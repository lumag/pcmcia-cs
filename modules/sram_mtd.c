/*======================================================================

    A simple MTD for accessing static RAM

    Written by David Hinds, dhinds@allegro.stanford.edu
    
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

#ifdef PCMCIA_DEBUG
static int pc_debug = PCMCIA_DEBUG;
MODULE_PARM(pc_debug, "i");
#define DEBUG(n, args...) if (pc_debug>(n)) printk(KERN_DEBUG args)
static char *version =
"sram_mtd.c 1.26 1998/01/09 04:19:01 (David Hinds)";
#else
#define DEBUG(n, args...)
#endif

/*====================================================================*/

/* Size of the memory window: 4K */
#define WINDOW_SIZE	0x1000

static void sram_config(dev_link_t *link);
static void sram_release(u_long arg);
static int sram_event(event_t event, int priority,
		       event_callback_args_t *args);

static dev_link_t *sram_attach(void);
static void sram_detach(dev_link_t *);

typedef struct sram_dev_t {
    caddr_t		Base;
    int			nregion;
    region_info_t	region[2*CISTPL_MAX_DEVICES];
} sram_dev_t;

static dev_info_t dev_info = "sram_mtd";

static dev_link_t *dev_list = NULL;

/*====================================================================*/

static void cs_error(client_handle_t handle, int func, int ret)
{
    error_info_t err = { func, ret };
    CardServices(ReportError, handle, &err);
}

/*======================================================================

    sram_attach() creates an "instance" of the driver, allocating
    local data structures for one device.  The device is registered
    with Card Services.

======================================================================*/

static dev_link_t *sram_attach(void)
{
    client_reg_t client_reg;
    dev_link_t *link;
    sram_dev_t *dev;
    int ret;
    
    DEBUG(0, "sram_attach()\n");

    /* Create new memory card device */
    link = kmalloc(sizeof(struct dev_link_t), GFP_KERNEL);
    memset(link, 0, sizeof(struct dev_link_t));
    link->release.function = &sram_release;
    link->release.data = (u_long)link;
    dev = kmalloc(sizeof(struct sram_dev_t), GFP_KERNEL);
    link->priv = dev;

    /* Register with Card Services */
    link->next = dev_list;
    dev_list = link;
    client_reg.dev_info = &dev_info;
    client_reg.Attributes = INFO_MTD_CLIENT | INFO_CARD_SHARE;
    client_reg.EventMask =
	CS_EVENT_RESET_PHYSICAL | CS_EVENT_CARD_RESET |
	CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
	CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
    client_reg.event_handler = &sram_event;
    client_reg.Version = 0x0210;
    client_reg.event_callback_args.client_data = link;
    ret = CardServices(RegisterClient, &link->handle, &client_reg);
    if (ret != 0) {
	cs_error(link->handle, RegisterClient, ret);
	sram_detach(link);
	return NULL;
    }

    return link;
} /* sram_attach */

/*======================================================================

    This deletes a driver "instance".  The device is de-registered
    with Card Services.  If it has been released, all local data
    structures are freed.  Otherwise, the structures will be freed
    when the device is released.

======================================================================*/

static void sram_detach(dev_link_t *link)
{
    dev_link_t **linkp;
    int ret;
    long flags;

    DEBUG(0, "sram_detach(0x%p)\n", link);
    
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
	sram_release((u_long)link);

    if (link->handle) {
	ret = CardServices(DeregisterClient, link->handle);
	if (ret != CS_SUCCESS)
	    cs_error(link->handle, DeregisterClient, ret);
    }
    
    /* Unlink device structure, free bits */
    *linkp = link->next;
    kfree_s(link->priv, sizeof(struct sram_dev_t));
    kfree_s(link, sizeof(struct dev_link_t));
    
} /* sram_detach */

/*======================================================================

    sram_config() is scheduled to run after a CARD_INSERTION event
    is received, to bind the MTD to appropriate memory regions.
    
======================================================================*/

static void printk_size(u_int sz)
{
    if (sz & 0x3ff)
	printk("%u bytes", sz);
    else if (sz & 0xfffff)
	printk("%u kb", sz >> 10);
    else
	printk("%u mb", sz >> 20);
}

static void sram_config(dev_link_t *link)
{
    sram_dev_t *dev;
    win_req_t req;
    mtd_reg_t reg;
    region_info_t region;
    int i, attr, ret;

    DEBUG(0, "sram_config(0x%p)\n", link);

    /* Allocate a 4K memory window */
    req.Attributes = WIN_DATA_WIDTH_16;
    req.Base = NULL; req.Size = WINDOW_SIZE;
    req.AccessSpeed = 0;
    link->win = (window_handle_t)link->handle;
    ret = MTDHelperEntry(MTDRequestWindow, &link->win, &req);
    if (ret != 0) {
	cs_error(link->handle, RequestWindow, ret);
	link->state &= ~DEV_CONFIG_PENDING;
	sram_release((u_long)link);
	return;
    }

    link->state |= DEV_CONFIG;

    /* Grab info for all the memory regions we can access */
    dev = link->priv;
    dev->Base = req.Base;
    i = 0;
    for (attr = 0; attr < 2; attr++) {
	region.Attributes = attr ? REGION_TYPE_AM : REGION_TYPE_CM;
	ret = CardServices(GetFirstRegion, link->handle, &region);
	while (ret == CS_SUCCESS) {
	    reg.Attributes = region.Attributes;
	    reg.Offset = region.CardOffset;
	    reg.MediaID = (u_long)&dev->region[i];
	    ret = CardServices(RegisterMTD, link->handle, &reg);
	    if (ret != 0) break;		
	    printk(KERN_INFO "sram_mtd: %s at 0x%x, ",
		   attr ? "attr" : "common", region.CardOffset);
	    printk_size(region.RegionSize);
	    printk(", %d ns\n", region.AccessSpeed);
	    dev->region[i] = region; i++;
	    ret = CardServices(GetNextRegion, link->handle, &region);
	}
    }
    dev->nregion = i;
    
} /* sram_config */

/*======================================================================

    After a card is removed, sram_release() will release the memory
    window allocated for this socket.
    
======================================================================*/

static void sram_release(u_long arg)
{
    dev_link_t *link = (dev_link_t *)arg;
    sram_dev_t *dev;

    DEBUG(0, "sram_release(0x%p)\n", link);

    if (link->win) {
	int ret = MTDHelperEntry(MTDReleaseWindow, link->win);
	if (ret != CS_SUCCESS)
	    cs_error(link->handle, ReleaseWindow, ret);
    }
    dev = link->priv;
    link->state &= ~DEV_CONFIG;
    
    if (link->state & DEV_STALE_LINK)
	sram_detach(link);
    
} /* sram_release */

/*====================================================================*/

static int sram_read(dev_link_t *link, char *buf, mtd_request_t *req)
{
    sram_dev_t *dev = (sram_dev_t *)link->priv;
    region_info_t *region;
    mtd_mod_win_t mod;
    u_int from, length, nb;
    int ret;
    
    DEBUG(1, "sram_read(0x%p, 0x%lx, 0x%p, 0x%x, 0x%x)\n", link,
	  req->MediaID, buf, req->SrcCardOffset, req->TransferLength);

    region = (region_info_t *)(req->MediaID);
    if (region->Attributes & REGION_TYPE_AM)
	mod.Attributes = WIN_MEMORY_TYPE_AM;
    else
	mod.Attributes = WIN_MEMORY_TYPE_CM;
    mod.AccessSpeed = region->AccessSpeed;

    mod.CardOffset = req->SrcCardOffset & ~(WINDOW_SIZE-1);
    from = req->SrcCardOffset & (WINDOW_SIZE-1);
    for (length = req->TransferLength; length > 0; length -= nb) {
	ret = MTDHelperEntry(MTDModifyWindow, link->win, &mod);
	if (ret != CS_SUCCESS) {
	    cs_error(link->handle, MapMemPage, ret);
	    return ret;
	}
	nb = (from+length > WINDOW_SIZE) ? WINDOW_SIZE-from : length;
	
	if (req->Function & MTD_REQ_KERNEL)
	    copy_from_pc(buf, &dev->Base[from], nb);
	else
	    copy_pc_to_user(buf, dev->Base+from, nb);
	buf += nb;
	
	from = 0;
	mod.CardOffset += WINDOW_SIZE;
    }
    return CS_SUCCESS;
} /* sram_read */

/*====================================================================*/

static int sram_write(dev_link_t *link, char *buf, mtd_request_t *req)
{
    sram_dev_t *dev = (sram_dev_t *)link->priv;
    mtd_mod_win_t mod;
    region_info_t *region;
    u_int from, length, nb;
    status_t status;
    int ret;

    DEBUG(1, "sram_write(0x%p, 0x%lx, 0x%p, 0x%x, 0x%x)\n", link,
	  req->MediaID, buf, req->DestCardOffset, req->TransferLength);

    /* Check card write protect status */
    ret = CardServices(GetStatus, link->handle, &status);
    if (ret != 0) {
	cs_error(link->handle, GetStatus, ret);
	return CS_GENERAL_FAILURE;
    }
    if (status.CardState & CS_EVENT_WRITE_PROTECT)
	return CS_WRITE_PROTECTED;
    
    region = (region_info_t *)(req->MediaID);
    if (region->Attributes & REGION_TYPE_AM)
	mod.Attributes = WIN_MEMORY_TYPE_AM;
    else
	mod.Attributes = WIN_MEMORY_TYPE_CM;
    mod.AccessSpeed = region->AccessSpeed;
    
    mod.CardOffset = req->DestCardOffset & ~(WINDOW_SIZE-1);
    from = req->DestCardOffset & (WINDOW_SIZE-1);
    for (length = req->TransferLength ; length > 0; length -= nb) {
	ret = MTDHelperEntry(MTDModifyWindow, link->win, &mod);
	if (ret != CS_SUCCESS) {
	    cs_error(link->handle, MapMemPage, ret);
	    return ret;
	}
	nb = (from+length > WINDOW_SIZE) ? WINDOW_SIZE-from : length;

	if (req->Function & MTD_REQ_KERNEL)
	    copy_to_pc(dev->Base+from, buf, nb);
	else
	    copy_user_to_pc(dev->Base+from, buf, nb);
	buf += nb;
	
	from = 0;
	mod.CardOffset += WINDOW_SIZE;
    }
    return CS_SUCCESS;
} /* sram_write */

/*====================================================================*/

#if 0
static int sram_erase(dev_link_t *link, char *buf, mtd_request_t *req)
{
    DEBUG(1, "sram_erase(0x%p, 0x%lx, 0x%p, 0x%x, 0x%x)\n", link,
	  req->MediaID, buf, req->DestCardOffset, req->TransferLength);

    if (req->Function & MTD_REQ_TIMEOUT) {
	DEBUG(2, "sram_erase: complete\n");
	return CS_SUCCESS;
    } else {
	DEBUG(2, "sram_erase: starting\n");
	req->Status = MTD_WAITTIMER;
	req->Timeout = 10;
	return CS_BUSY;
    }
    
} /* sram_erase */
#endif

/*====================================================================*/

static int sram_request(dev_link_t *link, void *buf, mtd_request_t *req)
{
    int ret = 0;
    if (!(link->state & DEV_PRESENT))
	return CS_NO_CARD;
    switch (req->Function & MTD_REQ_ACTION) {
    case MTD_REQ_READ:
	ret = sram_read(link, buf, req);
	break;
    case MTD_REQ_WRITE:
	ret = sram_write(link, buf, req);
	break;
    case MTD_REQ_ERASE:
#if 0
	ret = sram_erase(link, buf, req);
#endif
	ret = CS_UNSUPPORTED_FUNCTION;
	break;
    case MTD_REQ_COPY:
	ret = CS_UNSUPPORTED_FUNCTION;
	break;
    }
    if (!(link->state & DEV_PRESENT))
	return CS_GENERAL_FAILURE;
    return ret;
} /* sram_request */

/*======================================================================

    The card status event handler.  Mostly, this schedules other
    stuff to run after an event is received.  A CARD_REMOVAL event
    also sets some flags to discourage the driver from trying to
    talk to the card any more.
    
======================================================================*/

static int sram_event(event_t event, int priority,
		      event_callback_args_t *args)
{
    dev_link_t *link = args->client_data;

    DEBUG(1, "sram_event(0x%06x)\n", event);
    
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
	sram_config(link);
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
	return sram_request(link, args->buffer, args->mtdrequest);
	break;
	
    }
    return CS_SUCCESS;
} /* sram_event */

/*====================================================================*/

int init_module(void)
{
    servinfo_t serv;
    
    DEBUG(0, "%s\n", version);
    
    CardServices(GetCardServicesInfo, &serv);
    if (serv.Revision != CS_RELEASE_CODE) {
	printk(KERN_NOTICE "sram_mtd: Card Services release "
	       "does not match!\n");
	return -1;
    }
    
    register_pcmcia_driver(&dev_info, &sram_attach, &sram_detach);

    return 0;
}

void cleanup_module(void)
{
    DEBUG(0, "sram_mtd: unloading\n");
    unregister_pcmcia_driver(&dev_info);
    while (dev_list != NULL)
	sram_detach(dev_list);
}
