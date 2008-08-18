/*======================================================================

    A general driver for accessing PCMCIA card memory via Bulk
    Memory Services.

    This driver provides the equivalent of /dev/mem for a PCMCIA
    card's attribute and common memory.  It includes character
    and block device support.

    memory_cs.c 1.53 1998/11/18 08:01:13

    The contents of this file are subject to the Mozilla Public
    License Version 1.0 (the "License"); you may not use this file
    except in compliance with the License. You may obtain a copy of
    the License at http://www.mozilla.org/MPL/

    Software distributed under the License is distributed on an "AS
    IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
    implied. See the License for the specific language governing
    rights and limitations under the License.

    The initial developer of the original code is David A. Hinds
    <dhinds@hyper.stanford.edu>.  Portions created by David A. Hinds
    are Copyright (C) 1998 David A. Hinds.  All Rights Reserved.
    
======================================================================*/

#include <pcmcia/config.h>
#include <pcmcia/k_compat.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/major.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <stdarg.h>

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/bulkmem.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/ds.h>
#include <pcmcia/memory.h>
#include <pcmcia/mem_op.h>

/* Major device #'s for memory device */
static int major_dev = 0;

/* Funky stuff for setting up a block device */
#define MAJOR_NR		major_dev
#define DEVICE_NAME		"memory"
#define DEVICE_REQUEST		do_memory_request
#define DEVICE_ON(device)
#define DEVICE_OFF(device)

#define DEVICE_NR(minor)	((minor)>>4)
#define IS_DIRECT(minor)	(((minor)&8)>>3)
#define REGION_AM(minor)	(((minor)&4)>>2)
#define REGION_NR(minor)	((minor)&7)
#define MINOR_NR(dev,dir,attr,rgn) \
(((dev)<<4)+((dir)<<3)+((attr)<<2)+(rgn))

#include BLK_DEV_HDR

#ifdef PCMCIA_DEBUG
static int pc_debug = PCMCIA_DEBUG;
MODULE_PARM(pc_debug, "i");
#define DEBUG(n, args...) if (pc_debug>(n)) printk(KERN_DEBUG args)
static char *version =
"memory_cs.c 1.53 1998/11/18 08:01:13 (David Hinds)";
#else
#define DEBUG(n, args...)
#endif

/*====================================================================*/

/* Parameters that can be set with 'insmod' */

/* 1 = do 16-bit transfers, 0 = do 8-bit transfers */
static int word_width = 1;

/* Speed of memory accesses, in ns */
static int mem_speed = 0;

MODULE_PARM(word_width, "i");
MODULE_PARM(mem_speed, "i");

/*====================================================================*/

/* Maximum number of separate memory devices we'll allow */
#define MAX_DEV		4

/* Maximum number of partitions per memory space */
#define MAX_PART	4

/* Maximum number of outstanding erase requests per socket */
#define MAX_ERASE	8

/* Sector size -- shouldn't need to change */
#define SECTOR_SIZE	512

/* Size of the PCMCIA address space: 26 bits = 64 MB */
#define HIGH_ADDR	0x4000000

static void memory_config(dev_link_t *link);
static void memory_release(u_long arg);
static int memory_event(event_t event, int priority,
			event_callback_args_t *args);

static dev_link_t *memory_attach(void);
static void memory_detach(dev_link_t *);

/* Each memory region corresponds to a minor device */
typedef struct minor_dev_t {		/* For normal regions */
    region_info_t	region;
    memory_handle_t	handle;
    int			open;
} minor_dev_t;

typedef struct direct_dev_t {		/* For direct access */
    int			flags;
    int			open;
    caddr_t		Base;
    u_int		Size;
    u_long		cardsize;
} direct_dev_t;

typedef struct memory_dev_t {
    dev_node_t		node;
    eraseq_handle_t	eraseq_handle;
    eraseq_entry_t	eraseq[MAX_ERASE];
    wait_queue_head_t	erase_pending;
    direct_dev_t	direct;
    minor_dev_t		minor[2*MAX_PART];
} memory_dev_t;

#define MEM_WRPROT	1

static dev_info_t dev_info = "memory_cs";
static dev_link_t *dev_table[MAX_DEV] = { NULL, /* ... */ };

static int memory_blocksizes[MINOR_NR(MAX_DEV, 0, 0, 0)] =
{ 0, /* ... */ };
    
/*====================================================================*/

static int memory_ioctl(struct inode *inode, struct file *file,
			u_int cmd, u_long arg);
static FS_SIZE_T memory_read FOPS(struct inode *inode,
				  struct file *file, char *buf,
				  U_FS_SIZE_T count, loff_t *ppos);
static FS_SIZE_T memory_write FOPS(struct inode *inode,
				   struct file *file, const char *buf,
				   U_FS_SIZE_T count, loff_t *ppos);
static int memory_open(struct inode *inode, struct file *file);
static FS_RELEASE_T memory_close(struct inode *inode,
				 struct file *file);
static FS_RELEASE_T memory_blk_close(struct inode *inode,
				     struct file *file);

static struct file_operations memory_chr_fops = {
    NULL,		/* lseek */
    memory_read,	/* read */
    memory_write,      	/* write */
    NULL,		/* readdir */
    NULL,		/* select */
    memory_ioctl,	/* ioctl */
    NULL,		/* mmap */
    memory_open,	/* open */
    NULL_FLUSH		/* flush */
    memory_close,	/* release */
    NULL		/* fsync */
};

static struct file_operations memory_blk_fops = {
    NULL,		/* lseek */
    block_read,		/* read */
    block_write,	/* write */
    NULL,		/* readdir */
    NULL,		/* select */
    memory_ioctl,	/* ioctl */
    NULL,		/* mmap */
    memory_open,	/* open */
    NULL_FLUSH		/* flush */
    memory_blk_close,	/* release */
    block_fsync		/* fsync */
};

/*====================================================================*/

static void cs_error(client_handle_t handle, int func, int ret)
{
    error_info_t err = { func, ret };
    CardServices(ReportError, handle, &err);
}

/*======================================================================

    memory_attach() creates an "instance" of the driver, allocating
    local data structures for one device.  The device is registered
    with Card Services.

======================================================================*/

static dev_link_t *memory_attach(void)
{
    client_reg_t client_reg;
    dev_link_t *link;
    memory_dev_t *dev;
    eraseq_hdr_t eraseq_hdr;
    int i, ret;
    
    DEBUG(0, "memory_attach()\n");

    for (i = 0; i < MAX_DEV; i++)
	if (dev_table[i] == NULL) break;
    if (i == MAX_DEV) {
	printk(KERN_NOTICE "memory_cs: no devices available\n");
	return NULL;
    }
    
    /* Create new memory card device */
    link = kmalloc(sizeof(struct dev_link_t), GFP_KERNEL);
    dev_table[i] = link;
    memset(link, 0, sizeof(struct dev_link_t));
    link->release.function = &memory_release;
    link->release.data = (u_long)link;
    
    dev = kmalloc(sizeof(struct memory_dev_t), GFP_KERNEL);
    memset(dev, 0, sizeof(memory_dev_t));
    init_waitqueue_head(&dev->erase_pending);
    link->priv = dev;

    /* Register with Card Services */
    client_reg.dev_info = &dev_info;
    client_reg.Attributes = INFO_IO_CLIENT | INFO_CARD_SHARE;
    client_reg.EventMask =
	CS_EVENT_RESET_PHYSICAL | CS_EVENT_CARD_RESET |
	CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
	CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
    client_reg.event_handler = &memory_event;
    client_reg.Version = 0x0210;
    client_reg.event_callback_args.client_data = link;
    ret = CardServices(RegisterClient, &link->handle, &client_reg);
    if (ret != 0) {
	cs_error(link->handle, RegisterClient, ret);
	memory_detach(link);
	return NULL;
    }

    for (i = 0; i < MAX_ERASE; i++)
	dev->eraseq[i].State = ERASE_IDLE;
    eraseq_hdr.QueueEntryCnt = MAX_ERASE;
    eraseq_hdr.QueueEntryArray = dev->eraseq;
    dev->eraseq_handle = (void *)link->handle;
    ret = CardServices(RegisterEraseQueue, &dev->eraseq_handle, &eraseq_hdr);
    if (ret != 0) {
	cs_error(link->handle, RegisterEraseQueue, ret);
	dev->eraseq_handle = NULL;
	memory_detach(link);
	return NULL;
    }
    
    return link;
} /* memory_attach */

/*======================================================================

    This deletes a driver "instance".  The device is de-registered
    with Card Services.  If it has been released, all local data
    structures are freed.  Otherwise, the structures will be freed
    when the device is released.

======================================================================*/

static void memory_detach(dev_link_t *link)
{
    memory_dev_t *dev;
    int nd;

    DEBUG(0, "memory_detach(0x%p)\n", link);
    
    /* Verify device address */
    for (nd = 0; nd < MAX_DEV; nd++)
	if (dev_table[nd] == link) break;
    if (nd == MAX_DEV)
	return;

    if (link->state & DEV_CONFIG) {
	memory_release((u_long)link);
	if (link->state & DEV_STALE_CONFIG) {
	    link->state |= DEV_STALE_LINK;
	    return;
	}
    }

    dev = (memory_dev_t *)link->priv;
    if (dev->eraseq_handle)
	CardServices(DeregisterEraseQueue, dev->eraseq_handle);
    if (link->handle)
	CardServices(DeregisterClient, link->handle);
    
    /* Unlink device structure, free bits */
    dev_table[nd] = NULL;
    kfree_s(dev, sizeof(struct memory_dev_t));
    kfree_s(link, sizeof(struct dev_link_t));
    
} /* memory_detach */

/*======================================================================

    Figure out the size of a simple SRAM card
    
======================================================================*/

static void get_size(dev_link_t *link, direct_dev_t *direct)
{
    modwin_t mod;
    memreq_t mem;
    u_char buf[26];
    int s, t, ret;

    mod.Attributes = WIN_ENABLE | WIN_MEMORY_TYPE_CM;
    mod.AccessSpeed = 0;
    ret = CardServices(ModifyWindow, link->win, &mod);
    if (ret != CS_SUCCESS)
	cs_error(link->handle, ModifyWindow, ret);

    /* Look for wrap-around or dead end */
    mem.Page = 0;
    for (s = 12; s < 26; s++) {
	mem.CardOffset = 1<<s;
	CardServices(MapMemPage, link->win, &mem);
	buf[s] = readb(direct->Base);
	writeb(~buf[s], direct->Base);
	for (t = 12; t < s; t++) {
	    mem.CardOffset = 1<<t;
	    CardServices(MapMemPage, link->win, &mem);
	    if (readb(direct->Base) != buf[t]) {
		writeb(buf[t], direct->Base);
		break;
	    }
	}
	if (t < s) break;
	mem.CardOffset = 1<<s;
	CardServices(MapMemPage, link->win, &mem);
	if (readb(direct->Base) != (0xff & ~buf[s])) break;
	writeb(buf[s], direct->Base);
    }

    /* Restore that last byte on wrap-around */
    if (t < s) {
	mem.CardOffset = 1<<t;
	CardServices(MapMemPage, link->win, &mem);
	writeb(buf[t], direct->Base);
    }

    direct->cardsize = (t > 15) ? (1<<t) : 0;
} /* get_size */

static void print_size(u_long sz)
{
    if (sz & 0x03ff)
	printk("%ld bytes", sz);
    else if (sz & 0x0fffff)
	printk("%ld kb", sz >> 10);
    else
	printk("%ld mb", sz >> 20);
}

/*======================================================================

    memory_config() is scheduled to run after a CARD_INSERTION event
    is received, to configure the PCMCIA socket, and to make the
    ethernet device available to the system.
    
======================================================================*/

#define CS_CHECK(fn, args...) \
while ((last_ret=CardServices(last_fn=(fn), args))!=0) goto cs_failed

static void memory_config(dev_link_t *link)
{
    memory_dev_t *dev;
    minor_dev_t *minor;
    region_info_t region;
    cs_status_t status;
    win_req_t req;
    int nd, i, last_ret, last_fn, attr, ret, nr[2];

    DEBUG(0, "memory_config(0x%p)\n", link);

    /* Configure card */
    link->state |= DEV_CONFIG;

    for (nd = 0; nd < MAX_DEV; nd++)
	if (dev_table[nd] == link) break;
    
    dev = (memory_dev_t *)link->priv;

    /* Allocate a small memory window for direct access */
    if (word_width)
	req.Attributes = WIN_DATA_WIDTH_16;
    else
	req.Attributes = WIN_DATA_WIDTH_8;
    req.Base = req.Size = 0;
    req.AccessSpeed = mem_speed;
    link->win = (window_handle_t)link->handle;
    CS_CHECK(RequestWindow, &link->win, &req);
    /* Get write protect status */
    CS_CHECK(GetStatus, link->handle, &status);
    
    dev->direct.Base = ioremap(req.Base, req.Size);
    dev->direct.Size = req.Size;
    dev->direct.cardsize = 0;

    for (attr = 0; attr < 2; attr++) {
	nr[attr] = 0;
	minor = dev->minor + attr*MAX_PART;
	region.Attributes =
	    (attr) ? REGION_TYPE_AM : REGION_TYPE_CM;
	ret = CardServices(GetFirstRegion, link->handle, &region);
	while (ret == CS_SUCCESS) {
	    minor->region = region;
	    minor++; nr[attr]++;
	    ret = CardServices(GetNextRegion, link->handle, &region);
	}
    }
    
    sprintf(dev->node.dev_name, "mem%d", nd);
    dev->node.major = major_dev;
    dev->node.minor = MINOR_NR(nd, 0, 0, 0);
    link->dev = &dev->node;
    link->state &= ~DEV_CONFIG_PENDING;
    
    printk(KERN_INFO "memory_cs: mem%d:", nd);
    if ((nr[0] == 0) && (nr[1] == 0)) {
	cisinfo_t cisinfo;
	if ((CardServices(ValidateCIS, link->handle, &cisinfo)
	     == CS_SUCCESS) && (cisinfo.Chains == 0)) {
	    get_size(link, &dev->direct);
	    printk(" anonymous: ");
	    if (dev->direct.cardsize == 0) {
		dev->direct.cardsize = HIGH_ADDR;
		printk("unknown size");
	    } else {
		print_size(dev->direct.cardsize);
	    }
	} else {
	    printk(" no regions found.");
	}
    } else {
	for (attr = 0; attr < 2; attr++) {
	    minor = dev->minor + attr*MAX_PART;
	    if (attr && nr[0] && nr[1])
		printk(",");
	    if (nr[attr])
		printk(" %s", attr ? "attribute" : "common");
	    for (i = 0; i < nr[attr]; i++) {
		printk(" ");
		print_size(minor[i].region.RegionSize);
	    }
	}
    }
    printk("\n");
    return;

cs_failed:
    cs_error(link->handle, last_fn, last_ret);
    memory_release((u_long)link);
    return;
} /* memory_config */

/*======================================================================

    After a card is removed, memory_release() will unregister the 
    device, and release the PCMCIA configuration.  If the device is
    still open, this will be postponed until it is closed.
    
======================================================================*/

static void memory_release(u_long arg)
{
    dev_link_t *link = (dev_link_t *)arg;
    int nd;

    DEBUG(0, "memory_release(0x%p)\n", link);

    for (nd = 0; nd < MAX_DEV; nd++)
	if (dev_table[nd] == link) break;
    if (link->open) {
	DEBUG(0, "memory_cs: release postponed, 'mem%d'"
	      " still open\n", nd);
	link->state |= DEV_STALE_CONFIG;
	return;
    }

    link->dev = NULL;
    if (link->win) {
	memory_dev_t *dev = link->priv;
	iounmap(dev->direct.Base);
	CardServices(ReleaseWindow, link->win);
    }
    link->state &= ~DEV_CONFIG;
    
    if (link->state & DEV_STALE_LINK)
	memory_detach(link);
    
} /* memory_release */

/*======================================================================

    The card status event handler.  Mostly, this schedules other
    stuff to run after an event is received.  A CARD_REMOVAL event
    also sets some flags to discourage the driver from trying
    to talk to the card any more.
    
======================================================================*/

static int memory_event(event_t event, int priority,
		       event_callback_args_t *args)
{
    dev_link_t *link = args->client_data;
    memory_dev_t *dev;
    eraseq_entry_t *erase;

    DEBUG(1, "memory_event(0x%06x)\n", event);
    
    switch (event) {
    case CS_EVENT_CARD_REMOVAL:
	link->state &= ~DEV_PRESENT;
	if (link->state & DEV_CONFIG) {
	    link->release.expires = RUN_AT(HZ/20);
	    add_timer(&link->release);
	}
	break;
    case CS_EVENT_CARD_INSERTION:
	link->state |= DEV_PRESENT | DEV_CONFIG_PENDING;
	memory_config(link);
	break;
    case CS_EVENT_ERASE_COMPLETE:
	erase = (eraseq_entry_t *)(args->info);
	wake_up((wait_queue_head_t *)&erase->Optional);
	dev = (memory_dev_t *)(link->priv);
	wake_up_interruptible(&dev->erase_pending);
	break;
    case CS_EVENT_PM_SUSPEND:
	link->state |= DEV_SUSPEND;
	/* Fall through... */
    case CS_EVENT_RESET_PHYSICAL:
	/* get_lock(link); */
	break;
    case CS_EVENT_PM_RESUME:
	link->state &= ~DEV_SUSPEND;
	/* Fall through... */
    case CS_EVENT_CARD_RESET:
	/* free_lock(link); */
	break;
    }
    return 0;
} /* memory_event */

/*======================================================================

    This gets a memory handle for the region corresponding to the
    minor device number.
    
======================================================================*/

static int memory_open(struct inode *inode, struct file *file)
{
    int minor = MINOR(inode->i_rdev);
    dev_link_t *link;
    memory_dev_t *dev;
    minor_dev_t *minor_dev;
    open_mem_t open;
    int ret;
    
    DEBUG(0, "memory_open(%d)\n", minor);

    link = dev_table[DEVICE_NR(minor)];
    if (!DEV_OK(link))
	return -ENODEV;
    
    dev = (memory_dev_t *)link->priv;

    if (IS_DIRECT(minor) || (dev->direct.cardsize > 0)) {
	if ((file->f_mode & 2) && (dev->direct.flags & MEM_WRPROT))
	    return -EROFS;
	dev->direct.open++;
	file->private_data = NULL;
    } else {
	minor_dev = &dev->minor[REGION_NR(minor)];
	if (minor_dev->region.RegionSize == 0)
	    return -ENODEV;
	if (minor_dev->handle == NULL) {
	    minor_dev->handle = (memory_handle_t)link->handle;
	    open.Attributes = minor_dev->region.Attributes;
	    open.Offset = minor_dev->region.CardOffset;
	    ret = CardServices(OpenMemory, &minor_dev->handle, &open);
	    if (ret != CS_SUCCESS)
		return -ENOMEM;
	}
	minor_dev->open++;
	file->private_data = minor_dev;
    }
    
    link->open++;
    MOD_INC_USE_COUNT;
    return 0;
} /* memory_open */

/*====================================================================*/

static FS_RELEASE_T memory_close(struct inode *inode,
				 struct file *file)
{
    dev_link_t *link;
    int minor = MINOR(inode->i_rdev);
    memory_dev_t *dev;
    minor_dev_t *minor_dev;
    
    DEBUG(0, "memory_close(%d)\n", minor);

    link = dev_table[DEVICE_NR(minor)];
    dev = (memory_dev_t *)link->priv;
    if (IS_DIRECT(minor) || (dev->direct.cardsize > 0)) {
	dev->direct.open--;
    } else {
	minor_dev = &dev->minor[REGION_NR(minor)];
	minor_dev->open--;
	if (minor_dev->open == 0) {
	    CardServices(CloseMemory, minor_dev->handle);
	    minor_dev->handle = NULL;
	}
    }

    link->open--;
    MOD_DEC_USE_COUNT;
    return (FS_RELEASE_T)0;
} /* memory_close */

static FS_RELEASE_T memory_blk_close(struct inode *inode,
				     struct file *file)
{
    fsync_dev(inode->i_rdev);
    INVALIDATE_INODES(inode->i_rdev);
    invalidate_buffers(inode->i_rdev);
    return memory_close(inode, file);
}

/*======================================================================

    Read for character-mode device
    
======================================================================*/

static FS_SIZE_T direct_read FOPS(struct inode *inode,
				  struct file *file, char *buf,
				  U_FS_SIZE_T count, loff_t *ppos)
{
    int minor = MINOR(F_INODE(file)->i_rdev);
    dev_link_t *link;
    memory_dev_t *dev;
    direct_dev_t *direct;
    U_FS_SIZE_T size, pos, read, from, nb;
    int ret;
    modwin_t mod;
    memreq_t mem;

    DEBUG(2, "direct_read(%d, 0x%lx, %ld)\n", minor,
	  (u_long)FPOS, (u_long)count);

    link = dev_table[DEVICE_NR(minor)];
    
    if (!DEV_OK(link))
	return -ENODEV;
    dev = (memory_dev_t *)link->priv;
    direct = &dev->direct;
    
    /* Boundary checks */
    if (count < 0)
	return -EINVAL;
    pos = FPOS;
    size = (IS_DIRECT(minor)) ? HIGH_ADDR : direct->cardsize;
    if (pos >= size)
	return 0;
    if (count > size - pos)
	count = size - pos;

    mod.Attributes = WIN_ENABLE;
    mod.Attributes |= (REGION_AM(minor)) ? WIN_MEMORY_TYPE_AM : 0;
    mod.AccessSpeed = 0;
    ret = CardServices(ModifyWindow, link->win, &mod);
    if (ret != CS_SUCCESS) {
	cs_error(link->handle, ModifyWindow, ret);
	return -EIO;
    }
    
    mem.CardOffset = pos & ~(direct->Size-1);
    mem.Page = 0;
    from = pos & (direct->Size-1);
    for (read = 0; count > 0; count -= nb, read += nb) {
	ret = CardServices(MapMemPage, link->win, &mem);
	if (ret != CS_SUCCESS) {
	    cs_error(link->handle, MapMemPage, ret);
	    return -EIO;
	}
	nb = (from+count > direct->Size) ? direct->Size-from : count;
	copy_pc_to_user(buf, direct->Base+from, nb);
	buf += nb;
        from = 0;
	mem.CardOffset += direct->Size;
    }

    FPOS += read;
    return read;
} /* direct_read */

static FS_SIZE_T memory_read FOPS(struct inode *inode,
				  struct file *file, char *buf,
				  U_FS_SIZE_T count, loff_t *ppos)
{
    minor_dev_t *minor;
    mem_op_t req;
    int ret;

    minor = file->private_data;
    if (minor == NULL)
	return direct_read FOPS(inode, file, buf, count, ppos);
    
    DEBUG(2, "memory_read(0x%p, 0x%lx, %ld)\n", minor->handle,
	  (u_long)FPOS, (u_long)count);
    
    req.Attributes = MEM_OP_BUFFER_USER;
    req.Offset = FPOS;
    req.Count = count;
    ret = CardServices(ReadMemory, minor->handle, &req, buf);
    if (ret == CS_SUCCESS) {
	FPOS += count;
	return count;
    } else if (ret == CS_BAD_OFFSET)
	return 0;
    else
	return -EIO;
} /* memory_read */

/*======================================================================

    Erase a memory region.  This is used by the write routine for
    suitably aligned and sized blocks.  It is also used for the
    MEMERASE ioctl().
    
======================================================================*/

static int memory_erase(int minor, u_long f_pos, U_FS_SIZE_T count)
{
    dev_link_t *link = dev_table[DEVICE_NR(minor)];
    memory_dev_t *dev = link->priv;
    minor_dev_t *minor_dev = &dev->minor[REGION_NR(minor)];
    int i, ret;

    /* Find a free erase slot, or wait for one to become available */
    for (;;) {
	for (i = 0; i < MAX_ERASE; i++)
	    if (!ERASE_IN_PROGRESS(dev->eraseq[i].State)) break;
	if (i < MAX_ERASE) break;
	DEBUG(2, "waiting for erase slot...\n");
	interruptible_sleep_on(&dev->erase_pending);
	if (signal_pending(current))
	    return -ERESTARTSYS;
    }

    /* Queue a new request */
    dev->eraseq[i].State = ERASE_QUEUED;
    dev->eraseq[i].Handle = minor_dev->handle;
    dev->eraseq[i].Offset = f_pos;
    dev->eraseq[i].Size = count;
    ret = CardServices(CheckEraseQueue, dev->eraseq_handle);
    if (ret != CS_SUCCESS) {
	cs_error(link->handle, CheckEraseQueue, ret);
	return -EIO;
    }

    /* Wait for request to complete */
    init_waitqueue_head((wait_queue_head_t *)&dev->eraseq[i].Optional);
    if (ERASE_IN_PROGRESS(dev->eraseq[i].State))
	sleep_on((wait_queue_head_t *)&dev->eraseq[i].Optional);
    if (dev->eraseq[i].State != ERASE_PASSED)
	return -EIO;
    return 0;
}

/*======================================================================

    Write for character-mode device
    
======================================================================*/

static FS_SIZE_T direct_write FOPS(struct inode *inode,
				   struct file *file, const char *buf,
				   U_FS_SIZE_T count, loff_t *ppos)
{
    int minor = MINOR(F_INODE(file)->i_rdev);
    dev_link_t *link;
    memory_dev_t *dev;
    direct_dev_t *direct;
    U_FS_SIZE_T size, pos, wrote, to, nb;
    int ret;
    modwin_t mod;
    memreq_t mem;
    
    DEBUG(2, "direct_write(%d, 0x%lx, %ld)\n", minor,
	  (u_long)FPOS, (u_long)count);
    
    link = dev_table[DEVICE_NR(minor)];
    
    if (!DEV_OK(link))
	return -ENODEV;
    
    dev = (memory_dev_t *)link->priv;
    direct = &dev->direct;
    
    /* Check for write protect */
    if (direct->flags & MEM_WRPROT)
	return -EROFS;

    /* Boundary checks */
    if (count < 0)
	return -EINVAL;
    size = (IS_DIRECT(minor)) ? HIGH_ADDR : direct->cardsize;
    pos = FPOS;
    if (pos >= size)
        return -ENOSPC;
    if (count > size - pos)
	count = size - pos;

    mod.Attributes = WIN_ENABLE;
    mod.Attributes |= (REGION_AM(minor)) ? WIN_MEMORY_TYPE_AM : 0;
    mod.AccessSpeed = 0;
    ret = CardServices(ModifyWindow, link->win, &mod);
    if (ret != CS_SUCCESS) {
	cs_error(link->handle, ModifyWindow, ret);
	return -EIO;
    }
    
    mem.CardOffset = pos & ~(direct->Size-1);
    mem.Page = 0;
    to = pos & (direct->Size-1);
    for (wrote = 0; count > 0; count -= nb, wrote += nb) {
	ret = CardServices(MapMemPage, link->win, &mem);
	if (ret != CS_SUCCESS) {
	    cs_error(link->handle, MapMemPage, ret);
	    return -EIO;
	}
	nb = (to+count > direct->Size) ? direct->Size-to : count;
	copy_user_to_pc(direct->Base+to, buf, nb);
	buf += nb;
        to = 0;
	mem.CardOffset += direct->Size;
    }

    FPOS += wrote;
    return wrote;
} /* direct_write */

static FS_SIZE_T memory_write FOPS(struct inode *inode,
				   struct file *file, const char *buf,
				   U_FS_SIZE_T count, loff_t *ppos)
{
    minor_dev_t *minor;
    mem_op_t req;
    int ret;

    minor = file->private_data;
    if (minor == NULL)
	return direct_write FOPS(inode, file, buf, count, ppos);
    
    DEBUG(2, "memory_write(0x%p, 0x%lx, %ld)\n", minor->handle,
	  (u_long)FPOS, (u_long)count);
    if ((minor->region.BlockSize > 1) &&
	((FPOS & (minor->region.BlockSize-1)) == 0) &&
	((count & (minor->region.BlockSize-1)) == 0)) {
	ret = memory_erase(MINOR(F_INODE(file)->i_rdev), FPOS, count);
	if (ret != 0)
	    return ret;
    }
    
    req.Attributes = 0;
    req.Offset = FPOS;
    req.Count = count;
    ret = CardServices(WriteMemory, minor->handle, &req, buf);
    if (ret == CS_SUCCESS) {
	FPOS += count;
	return count;
    } else
	return -EIO;
} /* memory_write */

/*======================================================================

    IOCTL calls for getting device parameters.

======================================================================*/

static int memory_ioctl(struct inode *inode, struct file *file,
			u_int cmd, u_long arg)
{
    int minor = MINOR(inode->i_rdev);
    dev_link_t *link;
    memory_dev_t *dev;
    minor_dev_t *minor_dev;
    erase_info_t erase;
    u_int size;
    int ret = 0;

    link = dev_table[DEVICE_NR(minor)];
    
    if (!DEV_OK(link))
	return -ENODEV;
    dev = (memory_dev_t *)link->priv;
    minor_dev = &dev->minor[REGION_NR(minor)];

    size = (cmd & IOCSIZE_MASK) >> IOCSIZE_SHIFT;
    if (cmd & IOC_IN) {
	ret = verify_area(VERIFY_READ, (char *)arg, size);
	if (ret) return ret;
    }
    if (cmd & IOC_OUT) {
	ret = verify_area(VERIFY_WRITE, (char *)arg, size);
	if (ret) return ret;
    }

    switch (cmd) {
    case BLKGETSIZE:
	if (!IS_DIRECT(minor))
	    put_user(dev->direct.cardsize/SECTOR_SIZE, (long *)arg);
	else
	    put_user(minor_dev->region.RegionSize/SECTOR_SIZE,
		     (long *)arg);
	break;
    case BLKFLSBUF:
	if (!suser()) return -EACCES;
	if (!(inode->i_rdev)) return -EINVAL;
	fsync_dev(inode->i_rdev);
	invalidate_buffers(inode->i_rdev);
	break;
    case MEMGETINFO:
	if (!IS_DIRECT(minor)) {
	    copy_to_user((region_info_t *)arg, &minor_dev->region,
			 sizeof(struct region_info_t));
	} else ret = -EINVAL;
	break;
    case MEMERASE:
	if (!IS_DIRECT(minor)) {
	    copy_from_user(&erase, (erase_info_t *)arg,
			   sizeof(struct erase_info_t));
	    ret = memory_erase(minor, erase.Offset, erase.Size);
	} else ret = -EINVAL;
	break;
    default:
	ret = -EINVAL;
    }
    
    return ret;
} /* memory_ioctl */

/*======================================================================

    Handler for block device requests
    
======================================================================*/


static void do_direct_request(dev_link_t *link)
{
    int addr, len, from, nb, ret;
    char *buf;
    memory_dev_t *dev;
    direct_dev_t *direct;
    modwin_t mod;
    memreq_t mem;
    
    dev = (memory_dev_t *)link->priv;
    direct = &dev->direct;
    
    addr = CURRENT->sector * SECTOR_SIZE;
    len = CURRENT->current_nr_sectors * SECTOR_SIZE;
    if ((addr + len) > direct->cardsize) {
	end_request(0);
	return;
    }
    
    mod.Attributes = WIN_ENABLE;
    mod.AccessSpeed = 0;
    ret = CardServices(ModifyWindow, link->win, &mod);
    if (ret != CS_SUCCESS) {
	cs_error(link->handle, ModifyWindow, ret);
	end_request(0);
	return;
    }

    buf = CURRENT->buffer;
    mem.Page = 0;
    mem.CardOffset = addr & ~(direct->Size-1);
    from = addr & (direct->Size-1);
    ret = 0;
    
    if ((CURRENT->cmd == READ) || (CURRENT->cmd == WRITE))
	for ( ; len > 0; len -= nb, buf += nb, from = 0) {
	    ret = CardServices(MapMemPage, link->win, &mem);
	    if (ret != CS_SUCCESS) break;
	    nb = (from+len > direct->Size) ? direct->Size-from : len;
	    if (CURRENT->cmd == READ)
		copy_from_pc(buf, &direct->Base[from], nb);
	    else
		copy_to_pc(&direct->Base[from], buf, nb);
	    mem.CardOffset += direct->Size;
	}
    else panic("pcmem_cs: unknown block command!\n");
    
    if (ret == CS_SUCCESS)
	end_request(1);
    else {
	cs_error(link->handle, MapMemPage, ret);
	end_request(0);
    }
} /* do_direct_request */

static void do_memory_request(void)
{
    int ret, minor;
    char *buf;
    mem_op_t req;
    dev_link_t *link;
    memory_dev_t *dev;
    minor_dev_t *minor_dev;
    
    sti();
    do {
	INIT_REQUEST;

	minor = MINOR(DEVICE(CURRENT));
	link = dev_table[DEVICE_NR(minor)];
	dev = (memory_dev_t *)link->priv;

	if (IS_DIRECT(minor) || (dev->direct.cardsize > 0)) {
	    do_direct_request(link);
	    continue;
	}
	
	minor_dev = &dev->minor[REGION_NR(minor)];

	req.Attributes = MEM_OP_BUFFER_KERNEL;
	req.Offset = CURRENT->sector * SECTOR_SIZE;
	req.Count = CURRENT->current_nr_sectors * SECTOR_SIZE;
	buf = CURRENT->buffer;
	ret = CS_SUCCESS;
	
	if (CURRENT->cmd == READ) {
	    ret = CardServices(ReadMemory, minor_dev->handle,
			       &req, buf);
	    if (ret != CS_SUCCESS)
		cs_error(link->handle, ReadMemory, ret);
	} else if (CURRENT->cmd == WRITE) {
	    ret = CardServices(WriteMemory, minor_dev->handle,
			       &req, buf);
	    if (ret != CS_SUCCESS)
		cs_error(link->handle, WriteMemory, ret);
	} else
	    panic("memory_cs: unknown block command!\n");
	
	if (ret == CS_SUCCESS)
	    end_request(1);
	else
	    end_request(0);
    } while (1);
} /* do_memory_request */

/*====================================================================*/

int init_module(void)
{
    servinfo_t serv;
    int i;
    
    DEBUG(0, "%s\n", version);
    
    CardServices(GetCardServicesInfo, &serv);
    if (serv.Revision != CS_RELEASE_CODE) {
	printk(KERN_NOTICE "memory_cs: Card Services release "
	       "does not match!\n");
	return -1;
    }
    
    register_pcmcia_driver(&dev_info, &memory_attach, &memory_detach);

    for (i = MAX_CHRDEV-1; i > 0; i--) {
	if (register_chrdev(i, "memory", &memory_chr_fops) == 0) {
	    if (register_blkdev(i, "memory", &memory_blk_fops) == 0)
		break;
	    else
		unregister_chrdev(i, "memory");
	}
    }
    if (i == 0)
	printk(KERN_NOTICE "memory_cs: unable to grab a device #\n");
    else
	major_dev = i;

    blk_dev[major_dev].request_fn = DEVICE_REQUEST;
    for (i = 0; i < MINOR_NR(MAX_DEV, 0, 0, 0); i++)
	memory_blocksizes[i] = 1024;
    blksize_size[major_dev] = memory_blocksizes;
    
    return 0;
}

void cleanup_module(void)
{
    int i;
    dev_link_t *link;

    DEBUG(0, "memory_cs: unloading\n");
    unregister_pcmcia_driver(&dev_info);
    if (major_dev != 0) {
	unregister_chrdev(major_dev, "memory");
	unregister_blkdev(major_dev, "memory");
    }
    for (i = 0; i < MAX_DEV; i++) {
	link = dev_table[i];
	if (link) {
	    if (link->state & DEV_CONFIG)
		memory_release((u_long)link);
	    memory_detach(link);
	}
    }
}
