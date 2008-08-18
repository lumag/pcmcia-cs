/*======================================================================

    A driver for PCMCIA IDE/ATA disk cards

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
#include <linux/ioport.h>
#ifndef CONFIG_BLK_DEV_IDE
#define CONFIG_BLK_DEV_IDE
#endif
#include <linux/hdreg.h>
#include <linux/major.h>
#include <asm/io.h>
#include <asm/system.h>

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/ds.h>
#include <pcmcia/cisreg.h>

#ifdef PCMCIA_DEBUG
static int pc_debug = PCMCIA_DEBUG;
MODULE_PARM(pc_debug, "i");
#define DEBUG(n, args...) if (pc_debug>(n)) printk(KERN_DEBUG args)
static char *version =
"fixed_cs.c 1.20 1998/02/10 11:38:12 (David Hinds)";
#else
#define DEBUG(n, args...)
#endif

/*====================================================================*/

/* Parameters that can be set with 'insmod' */

/* Bit map of interrupts to choose from */
static u_int irq_mask = 0xdeb8;
static int irq_list[4] = { -1 };

MODULE_PARM(irq_mask, "i");
MODULE_PARM(irq_list, "1-4i");

/*====================================================================*/

static const char ide_major[4] = {
    IDE0_MAJOR, IDE1_MAJOR, IDE2_MAJOR, IDE3_MAJOR
};

typedef struct fixed_info_t {
    int		ndev;
    dev_node_t	node;
    int		hd;
} fixed_info_t;

static void fixed_config(dev_link_t *link);
static void fixed_release(u_long arg);
static int fixed_event(event_t event, int priority,
			event_callback_args_t *args);

static dev_info_t dev_info = "fixed_cs";

static dev_link_t *fixed_attach(void);
static void fixed_detach(dev_link_t *);

static dev_link_t *dev_list = NULL;

/*====================================================================*/

static void cs_error(client_handle_t handle, int func, int ret)
{
    error_info_t err = { func, ret };
    CardServices(ReportError, handle, &err);
}

/*======================================================================

    fixed_attach() creates an "instance" of the driver, allocating
    local data structures for one device.  The device is registered
    with Card Services.

======================================================================*/

static dev_link_t *fixed_attach(void)
{
    client_reg_t client_reg;
    dev_link_t *link;
    int i, ret;
    
    DEBUG(0, "fixed_attach()\n");

    /* Create new fixed device */
    link = kmalloc(sizeof(struct dev_link_t), GFP_KERNEL);
    memset(link, 0, sizeof(struct dev_link_t));
    link->release.function = &fixed_release;
    link->release.data = (u_long)link;
    link->io.Attributes1 = IO_DATA_PATH_WIDTH_AUTO;
    link->io.Attributes2 = IO_DATA_PATH_WIDTH_8;
    link->io.IOAddrLines = 3;
    link->irq.Attributes = IRQ_TYPE_EXCLUSIVE;
    link->irq.IRQInfo1 = IRQ_INFO2_VALID|IRQ_LEVEL_ID;
    if (irq_list[0] == -1)
	link->irq.IRQInfo2 = irq_mask;
    else
	for (i = 0; i < 4; i++)
	    link->irq.IRQInfo2 |= 1 << irq_list[i];
    link->conf.Attributes = CONF_ENABLE_IRQ;
    link->conf.Vcc = 50;
    link->conf.IntType = INT_MEMORY_AND_IO;
    link->priv = kmalloc(sizeof(struct fixed_info_t), GFP_KERNEL);
    memset(link->priv, 0, sizeof(struct fixed_info_t));
    
    /* Register with Card Services */
    link->next = dev_list;
    dev_list = link;
    client_reg.dev_info = &dev_info;
    client_reg.Attributes = INFO_IO_CLIENT | INFO_CARD_SHARE;
    client_reg.EventMask =
	CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
	CS_EVENT_RESET_PHYSICAL | CS_EVENT_CARD_RESET |
	CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
    client_reg.event_handler = &fixed_event;
    client_reg.Version = 0x0210;
    client_reg.event_callback_args.client_data = link;
    ret = CardServices(RegisterClient, &link->handle, &client_reg);
    if (ret != CS_SUCCESS) {
	cs_error(link->handle, RegisterClient, ret);
	fixed_detach(link);
	return NULL;
    }
    
    return link;
} /* fixed_attach */

/*======================================================================

    This deletes a driver "instance".  The device is de-registered
    with Card Services.  If it has been released, all local data
    structures are freed.  Otherwise, the structures will be freed
    when the device is released.

======================================================================*/

static void fixed_detach(dev_link_t *link)
{
    dev_link_t **linkp;
    long flags;
    int ret;

    DEBUG(0, "fixed_detach(0x%p)\n", link);
    
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
	fixed_release((u_long)link);
    
    if (link->handle) {
	ret = CardServices(DeregisterClient, link->handle);
	if (ret != CS_SUCCESS)
	    cs_error(link->handle, DeregisterClient, ret);
    }
    
    /* Unlink device structure, free bits */
    *linkp = link->next;
    kfree_s(link->priv, sizeof(fixed_info_t));
    kfree_s(link, sizeof(struct dev_link_t));
    
} /* fixed_detach */

/*======================================================================

    fixed_config() is scheduled to run after a CARD_INSERTION event
    is received, to configure the PCMCIA socket, and to make the
    fixed device available to the system.

======================================================================*/

#define CS_CHECK(fn, args...) \
while ((last_ret=CardServices(last_fn=(fn), args))!=0) goto cs_failed

void fixed_config(dev_link_t *link)
{
    client_handle_t handle;
    fixed_info_t *info;
    tuple_t tuple;
    u_short buf[128];
    cisparse_t parse;
    cistpl_cftable_entry_t *cfg = &parse.cftable_entry;
    int i, last_ret, last_fn, hd, io, ctl;

    sti();
    handle = link->handle;
    info = link->priv;
    
    DEBUG(0, "fixed_config(0x%p)\n", link);
    
    tuple.TupleData = (cisdata_t *)buf;
    tuple.TupleOffset = 0; tuple.TupleDataMax = 255;
    tuple.Attributes = 0;
    tuple.DesiredTuple = CISTPL_CONFIG;
    CS_CHECK(GetFirstTuple, handle, &tuple);
    CS_CHECK(GetTupleData, handle, &tuple);
    CS_CHECK(ParseTuple, handle, &tuple, &parse);
    link->conf.ConfigBase = parse.config.base;
    link->conf.Present = parse.config.rmask[0];
    
    /* Configure card */
    link->state |= DEV_CONFIG;

    io = ctl = 0;
    tuple.DesiredTuple = CISTPL_CFTABLE_ENTRY;
    tuple.Attributes = 0;
    i = CardServices(GetFirstTuple, handle, &tuple);
    while (i == CS_SUCCESS) {
	i = CardServices(GetTupleData, handle, &tuple);
	if (i != CS_SUCCESS) break;
	i = CardServices(ParseTuple, handle, &tuple, &parse);
	if ((i == CS_SUCCESS) &&
	    (cfg->vpp1.present & (1<<CISTPL_POWER_VNOM)) &&
	    (cfg->vpp1.param[CISTPL_POWER_VNOM] == 120))
	    link->conf.Vpp1 = link->conf.Vpp2 = 120;
	/* Sanity checks */
	if ((i == CS_SUCCESS) && (cfg->io.nwin > 0)) {
	    link->conf.ConfigIndex = cfg->index;
	    link->io.BasePort1 = cfg->io.win[0].base;
	    if (cfg->io.nwin == 2) {
		link->io.NumPorts1 = 8;
		link->io.BasePort2 = cfg->io.win[1].base;
		link->io.NumPorts2 = 1;
		i = CardServices(RequestIO, link->handle, &link->io);
		io = link->io.BasePort1;
		ctl = link->io.BasePort2;
	    }
	    else if (cfg->io.nwin == 1) {
		link->io.NumPorts1 = 16;
		link->io.NumPorts2 = 0;
		i = CardServices(RequestIO, link->handle, &link->io);
		io = link->io.BasePort1;
		ctl = link->io.BasePort1+0x0e;
	    }
	    else continue;
	    if (i == CS_SUCCESS) break;
	}
	i = CardServices(GetNextTuple, handle, &tuple);
    }
    if (i != CS_SUCCESS) {
	cs_error(link->handle, RequestIO, i);
	goto failed;
    }
    
    CS_CHECK(RequestIRQ, handle, &link->irq);
    CS_CHECK(RequestConfiguration, handle, &link->conf);
    
    release_region(link->io.BasePort1, link->io.NumPorts1);
    if (link->io.NumPorts2)
	release_region(link->io.BasePort2, link->io.NumPorts2);
    hd = ide_register(io, ctl, link->irq.AssignedIRQ);
    
    if (hd < 0) {
	printk(KERN_NOTICE "fixed_cs: ide_register() at 0x%3x &"
	       " 0x%3x, irq %u failed\n", io, ctl,
		   link->irq.AssignedIRQ);
	goto failed;
    }
    
    info->ndev = 1;
    sprintf(info->node.dev_name, "hd%c", 'a'+(hd*2));
    info->node.major = ide_major[hd];
    info->node.minor = 0;
    info->hd = hd;
    link->dev = &info->node;

    link->state &= ~DEV_CONFIG_PENDING;
    return;
    
cs_failed:
    cs_error(link->handle, last_fn, last_ret);
failed:
    fixed_release((u_long)link);

} /* fixed_config */

/*======================================================================

    After a card is removed, fixed_release() will unregister the net
    device, and release the PCMCIA configuration.  If the device is
    still open, this will be postponed until it is closed.
    
======================================================================*/

void fixed_release(u_long arg)
{
    dev_link_t *link = (dev_link_t *)arg;
    fixed_info_t *info = link->priv;
    
    sti();
    
    DEBUG(0, "fixed_release(0x%p)\n", link);

    if (info->ndev)
	ide_unregister(info->hd);
    info->ndev = 0;
    link->dev = NULL;
    
    CardServices(ReleaseConfiguration, link->handle);
    CardServices(ReleaseIO, link->handle, &link->io);
    CardServices(ReleaseIRQ, link->handle, &link->irq);
    
    link->state &= ~DEV_CONFIG;

} /* fixed_release */

/*======================================================================

    The card status event handler.  Mostly, this schedules other
    stuff to run after an event is received.  A CARD_REMOVAL event
    also sets some flags to discourage the fixed drivers from
    talking to the ports.
    
======================================================================*/

int fixed_event(event_t event, int priority,
		event_callback_args_t *args)
{
    dev_link_t *link = args->client_data;

    DEBUG(1, "fixed_event(0x%06x)\n", event);
    
    switch (event) {
    case CS_EVENT_CARD_REMOVAL:
	link->state &= ~DEV_PRESENT;
	if (link->state & DEV_CONFIG) {
	    link->release.expires = RUN_AT(HZ/20);
	    link->state |= DEV_RELEASE_PENDING;
	    add_timer(&link->release);
	}
	break;
    case CS_EVENT_CARD_INSERTION:
	link->state |= DEV_PRESENT | DEV_CONFIG_PENDING;
	fixed_config(link);
	break;
    case CS_EVENT_PM_SUSPEND:
	link->state |= DEV_SUSPEND;
	/* Fall through... */
    case CS_EVENT_RESET_PHYSICAL:
	if (link->state & DEV_CONFIG)
	    CardServices(ReleaseConfiguration, link->handle);
	break;
    case CS_EVENT_PM_RESUME:
	link->state &= ~DEV_SUSPEND;
	/* Fall through... */
    case CS_EVENT_CARD_RESET:
	if (DEV_OK(link))
	    CardServices(RequestConfiguration, link->handle, &link->conf);
	break;
    }
    return 0;
} /* fixed_event */

/*====================================================================*/

int init_module(void)
{
    servinfo_t serv;
    DEBUG(0, "%s\n", version);
    CardServices(GetCardServicesInfo, &serv);
    if (serv.Revision != CS_RELEASE_CODE) {
	printk(KERN_NOTICE "fixed_cs: Card Services release "
	       "does not match!\n");
	return -1;
    }
    register_pcmcia_driver(&dev_info, &fixed_attach, &fixed_detach);
    return 0;
}

void cleanup_module(void)
{
    DEBUG(0, "fixed_cs: unloading\n");
    unregister_pcmcia_driver(&dev_info);
    while (dev_list != NULL)
	fixed_detach(dev_list);
}
