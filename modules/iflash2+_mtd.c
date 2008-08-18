/*======================================================================

    A simple MTD for Intel Series 2+ Flash devices

    Written by David Hinds, dhinds@allegro.stanford.edu

    For efficiency and simplicity, this driver is very block oriented.
    Reads and writes must not span erase block boundaries.  Erases
    are limited to one erase block per request.  This makes it much
    easier to manage multiple asynchronous erases efficiently.
    
======================================================================*/

#include <pcmcia/config.h>
#include <pcmcia/k_compat.h>

/* #define PCMCIA_DEBUG 1 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/major.h>
#include <linux/fs.h>
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
#include <pcmcia/mem_op.h>
#include "iflash.h"

#ifdef PCMCIA_DEBUG
static int pc_debug = PCMCIA_DEBUG;
MODULE_PARM(pc_debug, "i");
#define DEBUG(n, args...) if (pc_debug>(n)) printk(KERN_DEBUG args)
static char *version =
"iflash2+_mtd.c 1.36 1998/01/09 04:19:01 (David Hinds)";
#else
#define DEBUG(n, args...)
#endif

/*====================================================================*/

/* Parameters that can be set with 'insmod' */

static int vpp_timeout_period	= HZ;		/* in ticks */
static int vpp_settle		= HZ/10;	/* in ticks */
static int write_timeout	= HZ/10;	/* in ticks */
static int erase_timeout	= 100;		/* in ms */
static int erase_limit		= 10*HZ;	/* in ticks */
static int retry_limit		= 4;		/* write retries */
static u_int max_tries       	= 4096;		/* status polling */

MODULE_PARM(vpp_timeout_period, "i");
MODULE_PARM(vpp_settle, "i");
MODULE_PARM(write_timeout, "i");
MODULE_PARM(erase_timeout, "i");
MODULE_PARM(erase_limit, "i");
MODULE_PARM(retry_limit, "i");
MODULE_PARM(max_tries, "i");

/*====================================================================*/

/* Size of the memory window: 8K */
#define WINDOW_SIZE	0x2000

static void flash_config(dev_link_t *link);
static void flash_release(u_long arg);
static int flash_event(event_t event, int priority,
		       event_callback_args_t *args);

static dev_link_t *flash_attach(void);
static void flash_detach(dev_link_t *);

#define MAX_CELLS		32

/* A flash region is composed of one or more "cells", where we allow
   simultaneous erases if they are in different cells */
typedef struct flash_region_t {
    region_info_t	region;
    u_int		cell_size;
    struct flash_cell_t {
	u_int		state;
	u_int		erase_time;
	u_int		erase_addr;
	u_int		erase_retries;
    } cell[MAX_CELLS];
} flash_region_t;

typedef struct flash_dev_t {
    caddr_t		Base;
    window_handle_t	ESRwin;
    caddr_t		ESRbase;
    int			vpp_usage;
    u_int		vpp_start;
    struct timer_list	vpp_timeout;
    flash_region_t	*flash[2*CISTPL_MAX_DEVICES];
} flash_dev_t;

#define FLASH_PENDING		0x01
#define FLASH_ERASING		0x02
#define FLASH_ERASE_SUSPEND	0x04

static dev_info_t dev_info = "iflash2+_mtd";

static dev_link_t *dev_list = NULL;

/*====================================================================*/

static void cs_error(client_handle_t handle, int func, int ret)
{
    error_info_t err = { func, ret };
    CardServices(ReportError, handle, &err);
}

#ifdef PCMCIA_DEBUG
static inline u_long uticks(void)
{
    u_long count;
    outb_p(0x00, 0x43);
    count = inb_p(0x40);
    count |= inb(0x40) << 8;
    count = ((LATCH-1) - count) * 10000;
    count = (count + LATCH/2) / LATCH;
    count += jiffies * 10000;
    return count;
}
#endif

/*======================================================================

    Low level routines for programming the flash card.
    
======================================================================*/

static void log_esr(char *s, volatile u_short *esr)
{
    u_short CSR, GSR, BSR;

    writew(IF_READ_CSR, esr);
    CSR = readw(esr);
    writew(IF_READ_ESR, esr);
    BSR = readw(esr+2);
    GSR = readw(esr+4);
    printk(KERN_NOTICE "%s%s", s ? s : "", s ? ": " : "");
    printk("CSR = 0x%04x, BSR = 0x%04x, GSR = 0x%04x\n",
	   CSR, BSR, GSR);
}

static void abort_cmd(volatile u_short *esr)
{
    u_short i;

    writew(IF_ABORT, esr);
    writew(IF_READ_ESR, esr);
    for (i = 0; i < max_tries; i++)
	if ((readw(esr+4) & GSR_SLEEP) == GSR_SLEEP) break;
    if (i == max_tries)
	printk(KERN_NOTICE "iflash2+_mtd: abort cmd failed!\n");
    writew(IF_READ_ARRAY, esr);
    writew(IF_CLEAR_CSR, esr);
}

static int set_rdy_mode(volatile u_short *esr, u_short mode)
{
    u_short i;

    writew(IF_READ_ESR, esr);
    for (i = 0; i < max_tries; i++)
	if (!(readw(esr+2) & BSR_QUEUE_FULL)) break;
    if (i == max_tries) {
	printk(KERN_NOTICE "iflash2+_mtd: set_rdy_mode timed out!\n");
	log_esr(NULL, esr);
	return CS_GENERAL_FAILURE;
    }
    writew(IF_RDY_MODE, esr);
    writew(mode, esr);
    writew(IF_READ_ESR, esr);
    for (i = 0; i < max_tries; i++) {
	if ((readw(esr+4) & GSR_WR_READY) == GSR_WR_READY) break;
    }
    if (i == max_tries) {
	printk(KERN_NOTICE "iflash2+_mtd: set_rdy_mode timed out!\n");
	log_esr(NULL, esr);
	return CS_GENERAL_FAILURE;
    }
    if (readw(esr+4) & GSR_OP_ERR)
	return CS_GENERAL_FAILURE;
    return CS_SUCCESS;
}

static int check_write(struct wait_queue **queue, volatile u_short *esr)
{
    writew(IF_READ_ESR, esr);
    if ((readw(esr+2) & BSR_READY) != BSR_READY) {
	current->timeout = jiffies + write_timeout;
	while (current->timeout &&
	       ((readw(esr+2) & BSR_READY) != BSR_READY))
	    interruptible_sleep_on(queue);
	if ((readw(esr+2) & BSR_READY) != BSR_READY) {
	    printk(KERN_NOTICE "iflash2+_mtd: check_write: timed out!\n");
	    log_esr(NULL, esr);
	    return CS_GENERAL_FAILURE;
	}
    }
    if (readw(esr+2) & BSR_FAILED) {
	log_esr("write error", esr);
	return CS_WRITE_FAILURE;
    }
    else
	return CS_SUCCESS;
}

static int page_setup(struct wait_queue **queue, volatile u_short *esr,
		      volatile u_short *address,  u_short count)
{
    u_short nw;

    writew(IF_READ_ESR, esr);
    if ((readw(esr+4) & GSR_PAGE_AVAIL) != GSR_PAGE_AVAIL) {
	current->timeout = jiffies + write_timeout;
	while (current->timeout &&
	       ((readw(esr+4) & GSR_PAGE_AVAIL) != GSR_PAGE_AVAIL))
	    interruptible_sleep_on(queue);
	if ((readw(esr+4) & GSR_PAGE_AVAIL) != GSR_PAGE_AVAIL) {
	    printk(KERN_NOTICE "iflash2+_mtd: page_setup timed out\n");
	    log_esr(NULL, esr);
	    return CS_GENERAL_FAILURE;
	}
    }
    nw = count >> 1;
    if (nw == 0) {
	/* Special case of single byte write */
	writeb(LOW(IF_SINGLE_LOAD), address);
    } else {
	writew(IF_SEQ_LOAD, address);
	/* Tricky: split count into low bytes and high bytes */
	nw = (count-nw-1) | ((nw-1) << 8);
	if ((u_long)address & 2) {
	    writew(0, address);
	    writew(nw, address);
	} else {
	    writew(nw, address);
	    writew(0, address);
	}
    }
    return CS_SUCCESS;
}

static int page_write(volatile u_short *esr, volatile u_short *address,
		      u_short count)
{
    u_short nw;
    u_short i;

    writew(IF_READ_ESR, esr);
    for (i = 0; i < max_tries; i++)
	if (!(readw(esr+2) & BSR_QUEUE_FULL)) break;
    if (i == max_tries) {
	printk(KERN_NOTICE "iflash2+_mtd: page_write timed out\n");
	log_esr(NULL, esr);
	return CS_GENERAL_FAILURE;
    }
    nw = count >> 1;
    if (nw == 0) {
	writeb(LOW(IF_PAGE_WRITE), address);
	writeb(0, address);
	writeb(0, address);
    } else {
	writew(IF_PAGE_WRITE, address);
	/* Tricky: split count into low bytes and high bytes */
	nw = (count-nw-1) | ((nw-1) << 8);
	if ((u_long)address & 2) {
	    writew(0, address);
	    writew(nw, address);
	} else {
	    writew(nw, address);
	    writew(0, address);
	}
    }
    return CS_SUCCESS;
}

static void block_erase(volatile u_short *address)
{
    writew(IF_BLOCK_ERASE, address);
    writew(IF_CONFIRM, address);
}

static int check_erase(volatile u_short *address)
{
    u_short CSR;
    writew(IF_READ_CSR, address);
    CSR = readw(address);
    if ((CSR & CSR_WR_READY) != CSR_WR_READY)
	return CS_BUSY;
    else if (CSR & (CSR_ERA_ERR | CSR_VPP_LOW | CSR_WR_ERR)) {
	log_esr("erase error", address);
	return CS_WRITE_FAILURE;
    }
    else
	return CS_SUCCESS;
}

static int suspend_erase(volatile u_short *esr)
{
    u_short i;

    DEBUG(1, "iflash2+_mtd: suspending erase...\n");
    writew(IF_ERASE_SUSPEND, esr);
    writew(IF_READ_ESR, esr);
    for (i = 0; i < max_tries; i++)
	if ((readw(esr+4) & GSR_WR_READY) == GSR_WR_READY) break;
    if (i == max_tries) {
	printk(KERN_NOTICE "iflash2+_mtd: suspend_erase timed out\n");
	log_esr(NULL, esr);
	return CS_GENERAL_FAILURE;
    }
    writew(IF_READ_ARRAY, esr);
    return CS_SUCCESS;
}

static void resume_erase(volatile u_short *esr)
{
    DEBUG(1, "iflash2+_mtd: resuming erase...\n");
    writew(IF_READ_ESR, esr);
    /* Only give resume signal if the erase is really suspended */
    if (readw(esr+4) & GSR_OP_SUSPEND)
	writew(IF_CONFIRM, esr);
}

static void reset_block(volatile u_short *esr)
{
    writew(IF_READ_ARRAY, esr);
    writew(IF_CLEAR_CSR, esr);
}

/*====================================================================*/

static void set_global_lock(window_handle_t win,
			    volatile caddr_t base, int set)
{
    mtd_mod_win_t mod;
    mod.Attributes = WIN_MEMORY_TYPE_AM;
    mod.AccessSpeed = 250;
    mod.CardOffset = 0x4000;
    MTDHelperEntry(MTDModifyWindow, win, &mod);
    writeb((set) ? WP_BLKEN : 0, base+CISREG_WP);
}

/*======================================================================

    Vpp management functions.  The vpp_setup() function checks to
    see if Vpp is available for the specified device.  If not, it
    turns on Vpp.  The vpp_shutdown() function is scheduled to turn
    Vpp off after an interval of inactivity.

    vpp_setup() assumes that it will be called at the top of a
    request handler, and that it can use the MTD_REQ_TIMEOUT flag
    to tell if it has already been called for this particular
    request, so that it can count Vpp users.

    A handler should call vpp_shutdown() once for each request that
    does a vpp_setup().
    
======================================================================*/

static int vpp_setup(dev_link_t *link, mtd_request_t *req)
{
    flash_dev_t *dev = (flash_dev_t *)link->priv;
    mtd_vpp_req_t vpp_req;

    /* First time for this request? */
    if (!(req->Function & MTD_REQ_TIMEOUT)) {
	dev->vpp_usage++;
	/* If no other users, kill shutdown timer and apply power */
	if (dev->vpp_usage == 1) {
	    if (dev->vpp_timeout.expires)
		del_timer(&dev->vpp_timeout);
	    else {
		DEBUG(1, "iflash2+_mtd: raising Vpp...\n");
		dev->vpp_start = jiffies;
		vpp_req.Vpp1 = vpp_req.Vpp2 = 120;
		MTDHelperEntry(MTDSetVpp, link->handle, &vpp_req);
	    }
	}
    }
    /* Wait for Vpp to settle if it was just applied */
    if (jiffies < dev->vpp_start + vpp_settle) {
	req->Status = MTD_WAITTIMER;
	req->Timeout = vpp_settle * 10;
	return 1;
    }
    return 0;
}

static void vpp_off(u_long arg)
{
    dev_link_t *link = (dev_link_t *)arg;
    flash_dev_t *dev;
    mtd_vpp_req_t req;

    DEBUG(1, "iflash2+_mtd: lowering Vpp...\n");
    dev = (flash_dev_t *)link->priv;
    dev->vpp_timeout.expires = 0;
    req.Vpp1 = req.Vpp2 = 0;
    MTDHelperEntry(MTDSetVpp, link->handle, &req);
}

static void vpp_shutdown(dev_link_t *link)
{
    flash_dev_t *dev;
    dev = (flash_dev_t *)link->priv;
    dev->vpp_usage--;
    if (dev->vpp_usage == 0) {
	dev->vpp_timeout.expires = RUN_AT(vpp_timeout_period);
	add_timer(&dev->vpp_timeout);
    }
}

/*======================================================================

    flash_attach() creates an "instance" of the driver, allocating
    local data structures for one device.  The device is registered
    with Card Services.

======================================================================*/

static dev_link_t *flash_attach(void)
{
    client_reg_t client_reg;
    dev_link_t *link;
    flash_dev_t *dev;
    int ret;
    
    DEBUG(0, "iflash2+_mtd: flash_attach()\n");

    /* Create new memory card device */
    link = kmalloc(sizeof(struct dev_link_t), GFP_KERNEL);
    memset(link, 0, sizeof(*link));
    link->release.function = &flash_release;
    link->release.data = (u_long)link;
    init_waitqueue(&link->pending);
    dev = kmalloc(sizeof(struct flash_dev_t), GFP_KERNEL);
    memset(dev, 0, sizeof(*dev));
    dev->vpp_timeout.function = vpp_off;
    dev->vpp_timeout.data = (u_long)link;
    link->priv = dev;

    /* Register with Card Services */
    link->next = dev_list;
    dev_list = link;
    client_reg.dev_info = &dev_info;
    client_reg.Attributes = INFO_MTD_CLIENT | INFO_CARD_SHARE;
    client_reg.EventMask =
	CS_EVENT_RESET_PHYSICAL | CS_EVENT_CARD_RESET |
	CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
	CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME |
	CS_EVENT_READY_CHANGE;
    client_reg.event_handler = &flash_event;
    client_reg.Version = 0x0210;
    client_reg.event_callback_args.client_data = link;
    ret = CardServices(RegisterClient, &link->handle, &client_reg);
    if (ret != 0) {
	cs_error(link->handle, RegisterClient, ret);
	flash_detach(link);
	return NULL;
    }
    
    return link;
} /* flash_attach */

/*======================================================================

    This deletes a driver "instance".  The device is de-registered
    with Card Services.  If it has been released, all local data
    structures are freed.  Otherwise, the structures will be freed
    when the device is released.

======================================================================*/

static void flash_detach(dev_link_t *link)
{
    dev_link_t **linkp;
    int ret;
    long flags;

    DEBUG(0, "iflash2+_mtd: flash_detach(0x%p)\n", link);
    
    /* Locate device structure */
    for (linkp = &dev_list; *linkp; linkp = &(*linkp)->next)
	if (*linkp == link) break;
    if (*linkp == NULL)
	return;

    save_flags(flags);
    cli();
    if (link->state & DEV_RELEASE_PENDING) {
	del_timer(&link->release);
	link->state &= ~DEV_RELEASE_PENDING;
    }
    restore_flags(flags);
    
    if (link->state & DEV_CONFIG)
	flash_release((u_long)link);

    if (link->handle) {
	ret = CardServices(DeregisterClient, link->handle);
	if (ret != CS_SUCCESS)
	    cs_error(link->handle, DeregisterClient, ret);
    }
    
    /* Unlink device structure, free bits */
    *linkp = link->next;
    kfree_s(link->priv, sizeof(struct flash_dev_t));
    kfree_s(link, sizeof(struct dev_link_t));
    
} /* flash_detach */

/*======================================================================

    flash_config() is scheduled to run after a CARD_INSERTION event
    is received, to bind the MTD to appropriate memory regions.
    
======================================================================*/

static void printk_size(u_int sz)
{
    if (sz & 0x3ff)
	printk("%u bytes", sz);
    else if (sz & 0x0fffff)
	printk("%u kb", sz >> 10);
    else
	printk("%u mb", sz >> 20);
}

static void flash_config(dev_link_t *link)
{
    flash_dev_t *dev;
    win_req_t req;
    mtd_reg_t reg;
    region_info_t region;
    int i, attr, ret;

    DEBUG(0, "iflash2+_mtd: flash_config(0x%p)\n", link);

    /* Allocate a 4K memory window */
    req.Attributes = WIN_DATA_WIDTH_16;
    req.Base = NULL; req.Size = WINDOW_SIZE;
    req.AccessSpeed = 0;
    link->win = (window_handle_t)link->handle;
    ret = MTDHelperEntry(MTDRequestWindow, &link->win, &req);
    if (ret != 0) {
	cs_error(link->handle, RequestWindow, ret);
	link->state &= ~DEV_CONFIG_PENDING;
	flash_release((u_long)link);
	return;
    }
    dev = link->priv;
    dev->Base = req.Base;

    /* Allocate a 4K memory window for ESR accesses*/
    req.Base = NULL; req.Size = WINDOW_SIZE;
    dev->ESRwin = (window_handle_t)link->handle;
    ret = MTDHelperEntry(MTDRequestWindow, &dev->ESRwin, &req);
    if (ret != 0) {
	cs_error(link->handle, RequestWindow, ret);
	link->state &= ~DEV_CONFIG_PENDING;
	flash_release((u_long)link);
	return;
    }
    dev->ESRbase = req.Base;

    link->state |= DEV_CONFIG;

    /* Grab info for all the memory regions we can access */
    i = 0;
    for (attr = 0; attr < 2; attr++) {
	region.Attributes = attr ? REGION_TYPE_AM : REGION_TYPE_CM;
	ret = CardServices(GetFirstRegion, link->handle, &region);
	while (ret == CS_SUCCESS) {
	    reg.Attributes = region.Attributes;
	    reg.Offset = region.CardOffset;
	    dev->flash[i] = kmalloc(sizeof(struct flash_region_t),
				    GFP_KERNEL);
	    reg.MediaID = (u_long)dev->flash[i];
	    ret = CardServices(RegisterMTD, link->handle, &reg);
	    if (ret != 0) {
		kfree_s(dev->flash[i], sizeof(struct flash_region_t));
		break;
	    }
	    printk(KERN_INFO "iflash2+_mtd: %s at 0x%x, ",
		   attr ? "attr" : "common", region.CardOffset);
	    printk_size(region.RegionSize);
	    printk(", %u ns\n", region.AccessSpeed);
	    memset(dev->flash[i], 0, sizeof(struct flash_region_t));
	    dev->flash[i]->region = region;
	    /* Distinguish between 4MB..20MB cards and 40MB cards */
	    if (region.RegionSize > 0x1400000)
		dev->flash[i]->cell_size = 0x800000; /* 8MB components */
	    else
		dev->flash[i]->cell_size = 0x400000; /* 4MB components */
	    i++;
	    ret = CardServices(GetNextRegion, link->handle, &region);
	}
    }
    dev->flash[i] = NULL;
    
} /* flash_config */

/*======================================================================

    After a card is removed, flash_release() will release the memory
    window allocated for this socket.
    
======================================================================*/

static void flash_release(u_long arg)
{
    dev_link_t *link = (dev_link_t *)arg;
    flash_dev_t *dev;
    int i;

    DEBUG(0, "iflash2+_mtd: flash_release(0x%p)\n", link);

    link->state &= ~DEV_CONFIG;
    if (link->win) {
	int ret = MTDHelperEntry(MTDReleaseWindow, link->win);
	if (ret != CS_SUCCESS)
	    cs_error(link->handle, ReleaseWindow, ret);
    }
    dev = link->priv;
    if (dev->ESRwin) {
	int ret = MTDHelperEntry(MTDReleaseWindow, dev->ESRwin);
	if (ret != CS_SUCCESS)
	    cs_error(link->handle, ReleaseWindow, ret);
    }
    if (dev->vpp_usage == 0)
	del_timer(&dev->vpp_timeout);
    vpp_off((u_long)link);
    for (i = 0; i < 2*CISTPL_MAX_DEVICES; i++)
	if (dev->flash[i])
	    kfree_s(dev->flash[i], sizeof(struct flash_region_t));
	else break;
    
    if (link->state & DEV_STALE_LINK)
	flash_detach(link);
    
} /* flash_release */

/*======================================================================

    The read request handler.  This handler supports suspending
    current erase requests.  Reading from a block that is currently
    erasing is undefined.
    
======================================================================*/

static int flash_read(dev_link_t *link, char *buf, mtd_request_t *req)
{
    flash_dev_t *dev = (flash_dev_t *)link->priv;
    flash_region_t *flash;
    region_info_t *region;
    mtd_mod_win_t mod;
    u_int from, length, nb, cell;
    int ret;
#ifdef PCMCIA_DEBUG
    u_long time;
#endif
    
    DEBUG(2, "iflash2+_mtd: flash_read(0x%p, 0x%lx, 0x%p, 0x%x, "
	  "0x%x)\n", link, req->MediaID, buf, req->SrcCardOffset,
	  req->TransferLength);

    flash = (flash_region_t *)(req->MediaID);
    region = &flash->region;
    if ((req->SrcCardOffset / region->BlockSize) !=
	((req->SrcCardOffset+req->TransferLength-1) / region->BlockSize))
	return CS_BAD_SIZE;
    if (region->Attributes & REGION_TYPE_AM)
	mod.Attributes = WIN_MEMORY_TYPE_AM;
    else
	mod.Attributes = WIN_MEMORY_TYPE_CM;
    mod.AccessSpeed = region->AccessSpeed;

    /* Suspend an in-progress block erase */
    cell = (req->SrcCardOffset - region->CardOffset) / flash->cell_size;
    if (flash->cell[cell].state & FLASH_ERASING) {
	if ((flash->cell[cell].erase_addr / region->BlockSize) ==
	    (req->SrcCardOffset / region->BlockSize)) {
	    DEBUG(1, "iflash2+_mtd: delaying read...\n");
	    req->Status = MTD_WAITREQ;
	    return CS_BUSY;
	}
	link->state |= DEV_BUSY;
	mod.CardOffset = flash->cell[cell].erase_addr;
	ret = MTDHelperEntry(MTDModifyWindow, dev->ESRwin, &mod);
	if (ret != CS_SUCCESS) goto done;
	ret = suspend_erase((u_short *)dev->ESRbase);
	if (ret != CS_SUCCESS) goto done;
	flash->cell[cell].state |= FLASH_ERASE_SUSPEND;
    }
    else
	link->state |= DEV_BUSY;

    mod.CardOffset = req->SrcCardOffset & ~(WINDOW_SIZE-1);
    from = req->SrcCardOffset & (WINDOW_SIZE-1);
    
    ret = CS_SUCCESS;
#ifdef PCMCIA_DEBUG
    time = uticks();
#endif
    for (length = req->TransferLength; length > 0; length -= nb) {
	
	ret = MTDHelperEntry(MTDModifyWindow, link->win, &mod);
	if (ret != CS_SUCCESS) goto done;
	nb = (from+length > WINDOW_SIZE) ? WINDOW_SIZE-from : length;

	if (req->Function & MTD_REQ_KERNEL)
	    copy_from_pc(buf, &dev->Base[from], nb);
	else
	    copy_pc_to_user(buf, &dev->Base[from], nb);
	
	buf += nb;
	from = 0;
	mod.CardOffset += WINDOW_SIZE;
    }
    
#ifdef PCMCIA_DEBUG
    time = uticks() - time;
    if (time < 10000000)
	DEBUG(3, "iflash2+_mtd: read complete, time = %ld,"
	      " avg = %ld ns/word, rate = %ld kb/sec\n", time,
	      time*2000/req->TransferLength,
	      req->TransferLength*977/time);
#endif
    
done:
    if (flash->cell[cell].state & FLASH_ERASE_SUSPEND) {
	resume_erase((u_short *)dev->ESRbase);
	flash->cell[cell].state &= ~FLASH_ERASE_SUSPEND;
    }
    link->state &= ~DEV_BUSY;
    return ret;
} /* flash_read */

/*======================================================================

    basic_write() handles a write that fits completely within a
    memory window that has already been set up.  It does a series
    of pipelined page buffer writes.
    
======================================================================*/

static int basic_write(struct wait_queue **queue, char *esr, char *dest,
		       char *buf, u_int nb, u_int is_krnl)
{
    u_short npb;
    int ret;

    /* Enable interrupts on write complete */
    ret = set_rdy_mode((u_short *)esr, IF_RDY_PULSE_WRITE);
    if (ret != CS_SUCCESS) return ret;
    
    /* Fix for mis-aligned writes */
    if ((u_long)dest & 1) {
	DEBUG(2, "iflash2+_mtd: odd address fixup at 0x%p\n", dest);
	ret = page_setup(queue, (u_short *)esr, (u_short *)dest, 1);
	if (ret != CS_SUCCESS) return ret;
	if (is_krnl)
	    writeb(*buf, dest);
	else {
	    char c;
	    get_user(c, buf);
	    writeb(c, dest);
	}
	ret = page_write((u_short *)esr, (u_short *)dest, 1);
	if (ret != CS_SUCCESS) return ret;
	dest++; buf++; nb--;
    }
    
    for (; nb > 0; nb -= npb) {
	    
	/* npb = # of bytes to write to page buffer */
	npb = (nb > 512) ? 512 : nb;
	/* sleep until page buffer is free */
	ret = page_setup(queue, (u_short *)esr, (u_short *)dest, npb);
	if (ret != CS_SUCCESS) return ret;
	
	if (is_krnl)
	    copy_to_pc(dest, buf, npb);
	else
	    copy_user_to_pc(dest, buf, npb);
	
	ret = page_write((u_short *)esr, (u_short *)dest, npb);
	if (ret != CS_SUCCESS) return ret;
	
	dest += npb;
	buf += npb;
    }
    
    /* sleep until block is ready */
    return check_write(queue, (u_short *)esr);
}
    
/*======================================================================

    The write request handler.  The Series 2+ cards support automatic
    erase suspend for writes.
    
======================================================================*/

static int flash_write(dev_link_t *link, char *buf, mtd_request_t *req)
{
    flash_dev_t *dev = (flash_dev_t *)link->priv;
    mtd_mod_win_t mod;
    mtd_rdy_req_t rdy;
    flash_region_t *flash;
    region_info_t *region;
    u_int from, length, nb, retry, cell;
    status_t status;
    int ret;
#ifdef PCMCIA_DEBUG
    u_long time;
#endif

    DEBUG(2, "iflash2+_mtd: flash_write(0x%p, 0x%lx, "
	  "0x%p, 0x%x, 0x%x)\n", link, req->MediaID, buf,
	  req->DestCardOffset, req->TransferLength);

    /* Check card write protect status */
    ret = CardServices(GetStatus, link->handle, &status);
    if (ret != CS_SUCCESS) {
	cs_error(link->handle, GetStatus, ret);
	return CS_GENERAL_FAILURE;
    }
    if (status.CardState & CS_EVENT_WRITE_PROTECT)
	return CS_WRITE_PROTECTED;

    flash = (flash_region_t *)(req->MediaID);
    region = &flash->region;
    if ((req->DestCardOffset / region->BlockSize) !=
	((req->DestCardOffset+req->TransferLength-1) / region->BlockSize))
	return CS_BAD_SIZE;
    
    if (vpp_setup(link, req) != 0)
	return CS_BUSY;

    /* Is this cell being erased or written? */
    cell = (req->DestCardOffset - region->CardOffset) / flash->cell_size;
    if (flash->cell[cell].state & FLASH_ERASING) {
	DEBUG(1, "iflash2+_mtd: delaying write...\n");
	req->Status = MTD_WAITREQ;
	return CS_BUSY;
    }
    link->state |= DEV_BUSY;
    
    if (region->Attributes & REGION_TYPE_AM)
	mod.Attributes = WIN_MEMORY_TYPE_AM;
    else
	mod.Attributes = WIN_MEMORY_TYPE_CM;
    mod.AccessSpeed = region->AccessSpeed;

    /* Set up window for ESR accesses */
    mod.CardOffset = req->DestCardOffset & ~(region->BlockSize-1);
    ret = MTDHelperEntry(MTDModifyWindow, dev->ESRwin, &mod);
    if (ret != CS_SUCCESS) goto done;

    rdy.Mask = CS_EVENT_READY_CHANGE;
    MTDHelperEntry(MTDRDYMask, link->handle, &rdy);

#ifdef PCMCIA_DEBUG
    time = uticks();
#endif
    mod.CardOffset = req->DestCardOffset & ~(WINDOW_SIZE-1);
    from = req->DestCardOffset & (WINDOW_SIZE-1);
    
    for (length = req->TransferLength ; length > 0; length -= nb) {
	ret = MTDHelperEntry(MTDModifyWindow, link->win, &mod);
	if (ret != CS_SUCCESS) goto done;

	nb = (from+length > WINDOW_SIZE) ? WINDOW_SIZE-from : length;

	for (retry = 0; retry < retry_limit; retry++) {
	    ret = basic_write(&link->pending, dev->ESRbase,
			      dev->Base+from, buf, nb,
			      (req->Function & MTD_REQ_KERNEL));
	    if (ret == CS_SUCCESS)
		break;
	    abort_cmd((u_short *)dev->ESRbase);
	}
	if (retry == retry_limit) {
	    printk(KERN_DEBUG "iflash2+_mtd: write failed: "
		   "too many retries!\n");
	    goto done;
	}
	
	buf += nb;
	from = 0;
	mod.CardOffset += WINDOW_SIZE;
    }

#ifdef PCMCIA_DEBUG
    time = uticks() - time;
    if (time < 10000000)
	DEBUG(3, "iflash2+_mtd: write complete, time = %ld,"
	      " avg = %ld us/word, rate = %ld kb/sec\n", time,
	      time*2/req->TransferLength,
	      req->TransferLength*977/time);
#endif
    
done:
    reset_block((u_short *)dev->ESRbase);
    rdy.Mask = 0;
    MTDHelperEntry(MTDRDYMask, link->handle, &rdy);
    link->state &= ~DEV_BUSY;
    /* Fire up the Vpp timer */
    vpp_shutdown(link);
    return ret;
} /* flash_write */

/*======================================================================

    The erase request handler.  This handler supports simultaneous
    erases in different device components.
    
======================================================================*/

static int flash_erase(dev_link_t *link, char *buf, mtd_request_t *req)
{
    flash_dev_t *dev = (flash_dev_t *)link->priv;
    status_t status;
    flash_region_t *flash;
    region_info_t *region;
    mtd_mod_win_t mod;
    int i, ret;

    DEBUG(2, "iflash2+_mtd: flash_erase(0x%p, 0x%lx, 0x%x, 0x%x)\n",
	  link, req->MediaID, req->DestCardOffset,
	  req->TransferLength);

    flash = (flash_region_t *)(req->MediaID);
    region = &flash->region;
    if (region->BlockSize != req->TransferLength)
	return CS_BAD_SIZE;
    
    i = (req->DestCardOffset-region->CardOffset)/flash->cell_size;
    
    if (!(req->Function & MTD_REQ_TIMEOUT)) {
	if (flash->cell[i].state & (FLASH_ERASING|FLASH_PENDING)) {
	    DEBUG(1, "iflash2+_mtd: delaying erase...\n");
	    req->Status = MTD_WAITREQ;
	    return CS_BUSY;
	}
	/* Check card write protect status */
	ret = CardServices(GetStatus, link->handle, &status);
	if (ret != CS_SUCCESS) {
	    cs_error(link->handle, GetStatus, ret);
	    return CS_GENERAL_FAILURE;
	}
	if (status.CardState & CS_EVENT_WRITE_PROTECT)
	    return CS_WRITE_PROTECTED;
	flash->cell[i].state |= FLASH_PENDING;
	/* Activate Vpp if necessary */
	if (vpp_setup(link, req) != 0)
	    return CS_BUSY;
    }

    if (region->Attributes & REGION_TYPE_AM)
	mod.Attributes = WIN_MEMORY_TYPE_AM;
    else
	mod.Attributes = WIN_MEMORY_TYPE_CM;
    mod.AccessSpeed = region->AccessSpeed;
    mod.CardOffset = req->DestCardOffset;
    ret = MTDHelperEntry(MTDModifyWindow, link->win, &mod);
    if (ret != CS_SUCCESS)
	goto done;
    
    if (flash->cell[i].state & FLASH_PENDING) {
	/* Start a new block erase */
	flash->cell[i].state &= ~FLASH_PENDING;
	flash->cell[i].state |= FLASH_ERASING;
	flash->cell[i].erase_addr = mod.CardOffset;
	flash->cell[i].erase_time = jiffies;
	flash->cell[i].erase_retries = 0;
	set_global_lock(dev->ESRwin, dev->ESRbase, 0);
	/* Disable busy signal during the erase */
	set_rdy_mode((u_short *)dev->Base, IF_RDY_DISABLE);
	block_erase((u_short *)dev->Base);
    } else {
	/* Check on an already started erase */
	ret = check_erase((u_short *)dev->Base);
	if (ret == CS_SUCCESS)
	    goto done;
	else if (ret != CS_BUSY) {
	    if (++flash->cell[i].erase_retries > retry_limit) {
		printk(KERN_NOTICE "iflash2+_mtd: erase failed: "
		       "too many retries!\n");
		goto done;
	    } else {
		flash->cell[i].erase_time = jiffies;
		abort_cmd((u_short *)dev->Base);
		block_erase((u_short *)dev->Base);
	    }
	}
    }

    /* If the request is not complete, has it taken too long? */
    if (jiffies > flash->cell[i].erase_time + erase_limit) {
	printk(KERN_NOTICE "iflash2+_mtd: erase timed out!\n");
	log_esr(NULL, (u_short *)dev->Base);
	abort_cmd((u_short *)dev->Base);
	ret = CS_GENERAL_FAILURE;
	goto done;
    }
    req->Status = MTD_WAITTIMER;
    req->Timeout = erase_timeout;
    return CS_BUSY;
    
done:
    DEBUG(2, "iflash2+_mtd: erase complete, time = %ld\n",
	  jiffies - flash->cell[i].erase_time);
    flash->cell[i].state &= ~(FLASH_ERASING|FLASH_PENDING);
    reset_block((u_short *)dev->ESRbase);
    set_global_lock(dev->ESRwin, dev->ESRbase, 1);
    vpp_shutdown(link);
    return ret;
} /* flash_erase */
    
/*====================================================================*/

static int flash_request(dev_link_t *link, void *buf, mtd_request_t *req)
{
    int ret = 0;
    
    if (!(link->state & DEV_PRESENT))
	return CS_NO_CARD;

    if (link->state & DEV_BUSY) {
	/* We do this because the erase routine uses the TIMEOUT flag
	   to decide if this is a new request or a status check, so
	   we need to propagate it */
	if (req->Function & MTD_REQ_TIMEOUT) {
	    req->Timeout = erase_timeout;
	    req->Status = MTD_WAITTIMER;
	}
	else
	    req->Status = MTD_WAITREQ;
	return CS_BUSY;
    }
    
    switch (req->Function & MTD_REQ_ACTION) {
    case MTD_REQ_READ:
	ret = flash_read(link, buf, req);
	break;
    case MTD_REQ_WRITE:
	ret = flash_write(link, buf, req);
	break;
    case MTD_REQ_ERASE:
	ret = flash_erase(link, buf, req);
	break;
    case MTD_REQ_COPY:
	ret = CS_UNSUPPORTED_FUNCTION;
	break;
    }
    if (!(link->state & DEV_PRESENT))
	return CS_GENERAL_FAILURE;
    return ret;
} /* flash_request */

/*======================================================================

    The card status event handler.  Mostly, this schedules other
    stuff to run after an event is received.  A CARD_REMOVAL event
    also sets some flags to discourage the driver from trying to
    talk to the card any more.
    
======================================================================*/

static int flash_event(event_t event, int priority,
		       event_callback_args_t *args)
{
    dev_link_t *link = args->client_data;

    DEBUG(1, "iflash2+_mtd: flash_event(0x%06x)\n", event);
    
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
	flash_config(link);
	break;

    case CS_EVENT_READY_CHANGE:
	wake_up_interruptible(&link->pending);
	break;
	
    case CS_EVENT_PM_SUSPEND:
	link->state |= DEV_SUSPEND;
	/* Fall through... */
    case CS_EVENT_RESET_PHYSICAL:
	break;
	
    case CS_EVENT_PM_RESUME:
	link->state &= ~DEV_SUSPEND;
	/* Fall through... */
    case CS_EVENT_CARD_RESET:
	break;
	
    case CS_EVENT_MTD_REQUEST:
	return flash_request(link, args->buffer, args->mtdrequest);
	break;
	
    }
    return CS_SUCCESS;
} /* flash_event */

/*====================================================================*/

int init_module(void)
{
    servinfo_t serv;
    
    DEBUG(0, "%s\n", version);
    
    CardServices(GetCardServicesInfo, &serv);
    if (serv.Revision != CS_RELEASE_CODE) {
	printk(KERN_NOTICE "iflash2+_mtd: Card Services release "
	       "does not match!\n");
	return -1;
    }
    
    register_pcmcia_driver(&dev_info, &flash_attach, &flash_detach);

    return 0;
}

void cleanup_module(void)
{
    DEBUG(0, "iflash2+_mtd: unloading\n");
    unregister_pcmcia_driver(&dev_info);
    while (dev_list != NULL)
	flash_detach(dev_list);
}
