/*======================================================================

    A driver for Future Domain-compatible PCMCIA SCSI cards

    Written by David Hinds, dhinds@allegro.stanford.edu
    
======================================================================*/

#include <pcmcia/config.h>
#include <pcmcia/k_compat.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/malloc.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/ioport.h>
#if (LINUX_VERSION_CODE >= VERSION(1,3,98))
#include <scsi/scsi.h>
#else
#include <linux/scsi.h>
#endif
#include <linux/major.h>

#include BLK_DEV_HDR
#include "drivers/scsi/scsi.h"
#include "drivers/scsi/hosts.h"
#if (LINUX_VERSION_CODE >= VERSION(1,3,98))
#include <scsi/scsi_ioctl.h>
#else
#include "drivers/scsi/scsi_ioctl.h"
#endif
#include "drivers/scsi/fdomain.h"

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/ds.h>

#ifdef PCMCIA_DEBUG
static int pc_debug = PCMCIA_DEBUG;
MODULE_PARM(pc_debug, "i");
#define DEBUG(n, args...) if (pc_debug>(n)) printk(KERN_DEBUG args)
static char *version =
"fdomain_cs.c 1.25 1998/01/31 19:55:16 (David Hinds)";
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

typedef struct scsi_info_t {
    int		ndev;
    dev_node_t	node[8];
} scsi_info_t;

extern void fdomain_setup(char *str, int *ints);

static void fdomain_release(u_long arg);
static int fdomain_event(event_t event, int priority,
			event_callback_args_t *args);

static dev_link_t *fdomain_attach(void);
static void fdomain_detach(dev_link_t *);

static Scsi_Host_Template driver_template = FDOMAIN_16X0;

static dev_link_t *dev_list = NULL;

static dev_info_t dev_info = "fdomain_cs";

/*====================================================================*/

static void cs_error(client_handle_t handle, int func, int ret)
{
    error_info_t err = { func, ret };
    CardServices(ReportError, handle, &err);
}

/*====================================================================*/

static dev_link_t *fdomain_attach(void)
{
    client_reg_t client_reg;
    dev_link_t *link;
    int i, ret;
    
    DEBUG(0, "fdomain_attach()\n");

    /* Create new SCSI device */
    link = kmalloc(sizeof(struct dev_link_t), GFP_KERNEL);
    memset(link, 0, sizeof(struct dev_link_t));
    link->priv = kmalloc(sizeof(struct scsi_info_t), GFP_KERNEL);
    memset(link->priv, 0, sizeof(struct scsi_info_t));
    link->release.function = &fdomain_release;
    link->release.data = (u_long)link;

    link->io.NumPorts1 = 0x10;
    link->io.Attributes1 = IO_DATA_PATH_WIDTH_AUTO;
    link->io.IOAddrLines = 10;
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
    link->conf.Present = PRESENT_OPTION;

    /* Register with Card Services */
    link->next = dev_list;
    dev_list = link;
    client_reg.dev_info = &dev_info;
    client_reg.Attributes = INFO_IO_CLIENT | INFO_CARD_SHARE;
    client_reg.event_handler = &fdomain_event;
    client_reg.EventMask =
	CS_EVENT_RESET_REQUEST | CS_EVENT_CARD_RESET |
	CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
	CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
    client_reg.Version = 0x0210;
    client_reg.event_callback_args.client_data = link;
    ret = CardServices(RegisterClient, &link->handle, &client_reg);
    if (ret != 0) {
	cs_error(link->handle, RegisterClient, ret);
	fdomain_detach(link);
	return NULL;
    }
    
    return link;
} /* fdomain_attach */

/*====================================================================*/

static void fdomain_detach(dev_link_t *link)
{
    dev_link_t **linkp;

    DEBUG(0, "fdomain_detach(0x%p)\n", link);
    
    /* Locate device structure */
    for (linkp = &dev_list; *linkp; linkp = &(*linkp)->next)
	if (*linkp == link) break;
    if (*linkp == NULL)
	return;

    if (link->state & DEV_CONFIG) {
	fdomain_release((u_long)link);
	if (link->state & DEV_STALE_CONFIG) {
	    link->state |= DEV_STALE_LINK;
	    return;
	}
    }

    if (link->handle)
	CardServices(DeregisterClient, link->handle);
    
    /* Unlink device structure, free bits */
    *linkp = link->next;
    if (link->priv)
	kfree_s(link->priv, sizeof(struct scsi_info_t));
    kfree_s(link, sizeof(struct dev_link_t));
    
} /* fdomain_detach */

/*====================================================================*/

#define CS_CHECK(fn, args...) \
while ((last_ret=CardServices(last_fn=(fn), args))!=0) goto cs_failed

static void fdomain_config(dev_link_t *link)
{
    client_handle_t handle;
    scsi_info_t *info;
    tuple_t tuple;
    cisparse_t parse;
    int i, last_ret, last_fn, ints[3];
    u_char tuple_data[64];
#ifdef GET_SCSI_INFO
    Scsi_Device *dev;
    dev_node_t *node, **tail;
#if (LINUX_VERSION_CODE >= VERSION(2,1,75))
    struct Scsi_Host *host;
#endif
#endif
    
    handle = link->handle;
    info = link->priv;

    DEBUG(0, "fdomain_config(0x%p)\n", link);

    tuple.DesiredTuple = CISTPL_CONFIG;
    tuple.TupleData = tuple_data;
    tuple.TupleDataMax = 64;
    tuple.TupleOffset = 0;
    CS_CHECK(GetFirstTuple, handle, &tuple);
    CS_CHECK(GetTupleData, handle, &tuple);
    CS_CHECK(ParseTuple, handle, &tuple, &parse);
    link->conf.ConfigBase = parse.config.base;

    /* Configure card */
#if (LINUX_VERSION_CODE >= VERSION(2,1,23))
    driver_template.module = &__this_module;
#else
    driver_template.usage_count = &MOD_USE_COUNT;
#endif
    link->state |= DEV_CONFIG;
    
    tuple.DesiredTuple = CISTPL_CFTABLE_ENTRY;
    i = CardServices(GetFirstTuple, handle, &tuple);
    while (i == CS_SUCCESS) {
	i = CardServices(GetTupleData, handle, &tuple);
	if (i != CS_SUCCESS) break;
	i = CardServices(ParseTuple, handle, &tuple, &parse);
	if (i != CS_SUCCESS) break;
	link->conf.ConfigIndex = parse.cftable_entry.index;
	link->io.BasePort1 = parse.cftable_entry.io.win[0].base;
	i = CardServices(RequestIO, handle, &link->io);
	if (i != CS_IN_USE) break;
	i = CardServices(GetNextTuple, handle, &tuple);
    }
    if (i != CS_SUCCESS) {
	cs_error(handle, RequestIO, i);
	goto failed;
    }

    CS_CHECK(RequestIRQ, handle, &link->irq);
    CS_CHECK(RequestConfiguration, handle, &link->conf);
    
    /* A bad hack... */
    release_region(link->io.BasePort1, link->io.NumPorts1);

    /* Set configuration options for the fdomain driver */
    ints[0] = 2;
    ints[1] = link->io.BasePort1;
    ints[2] = link->irq.AssignedIRQ;
    fdomain_setup("PCMCIA setup", ints);
    
    scsi_register_module(MODULE_SCSI_HA, &driver_template);

#ifdef GET_SCSI_INFO
    tail = &link->dev;
    info->ndev = 0;
#if (LINUX_VERSION_CODE < VERSION(2,1,75))
    for (dev = scsi_devices; dev != NULL; dev = dev->next)
	if (dev->host->hostt == &driver_template) {
#else
    for (host = scsi_hostlist; host; host = host->next)
	if (host->hostt == &driver_template)
	    for (dev = host->host_queue; dev; dev = dev->next) {
#endif
	    u_long arg[2], id;
	    kernel_scsi_ioctl(dev, SCSI_IOCTL_GET_IDLUN, arg);
	    id = (arg[0]&0x0f) + ((arg[0]>>4)&0xf0) +
		((arg[0]>>8)&0xf00) + ((arg[0]>>12)&0xf000);
	    node = &info->node[info->ndev];
	    node->minor = 0;
	    switch (dev->type) {
	    case TYPE_TAPE:
		node->major = SCSI_TAPE_MAJOR;
		sprintf(node->dev_name, "st#%04lx", id);
		break;
	    case TYPE_DISK:
	    case TYPE_MOD:
		node->major = SCSI_DISK_MAJOR;
		sprintf(node->dev_name, "sd#%04lx", id);
		break;
	    case TYPE_ROM:
	    case TYPE_WORM:
		node->major = SCSI_CDROM_MAJOR;
		sprintf(node->dev_name, "sr#%04lx", id);
		break;
	    default:
		node->major = SCSI_GENERIC_MAJOR;
		sprintf(node->dev_name, "sg#%04lx", id);
		break;
	    }
	    *tail = node; tail = &node->next;
	    info->ndev++;
	}
    *tail = NULL;
    if (info->ndev == 0)
	printk(KERN_INFO "fdomain_cs: no SCSI devices found\n");
#else
    strcpy(info->node[0].dev_name, "n/a");
    link->dev = &info->node[0];
#endif /* GET_SCSI_INFO */
    
    link->state &= ~DEV_CONFIG_PENDING;
    return;
    
cs_failed:
    cs_error(link->handle, last_fn, last_ret);
failed:
    fdomain_release((u_long)link);
    return;
    
} /* fdomain_config */

/*====================================================================*/

static void fdomain_release(u_long arg)
{
    dev_link_t *link = (dev_link_t *)arg;

    DEBUG(0, "fdomain_release(0x%p)\n", link);

#if (LINUX_VERSION_CODE < VERSION(2,1,23))
    if (*driver_template.usage_count != 0) {
#else
    if (driver_template.module->usecount != 0) {
#endif
	DEBUG(1, "fdomain_cs: release postponed, "
	      "device still open\n");
	link->state |= DEV_STALE_CONFIG;
	return;
    }

    scsi_unregister_module(MODULE_SCSI_HA, &driver_template);
    link->dev = NULL;
    
    CardServices(ReleaseConfiguration, link->handle);
    CardServices(ReleaseIO, link->handle, &link->io);
    CardServices(ReleaseIRQ, link->handle, &link->irq);
    
    link->state &= ~DEV_CONFIG;
    if (link->state & DEV_STALE_LINK)
	fdomain_detach(link);
    
} /* fdomain_release */

/*====================================================================*/

static int fdomain_event(event_t event, int priority,
			event_callback_args_t *args)
{
    dev_link_t *link = args->client_data;

    DEBUG(1, "fdomain_event(0x%06x)\n", event);
    
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
	fdomain_config(link);
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
	if (link->state & DEV_CONFIG) {
	    CardServices(RequestConfiguration, link->handle, &link->conf);
#if (LINUX_VERSION_CODE < VERSION(2,1,18))
	    fdomain_16x0_reset(NULL);
#else
	    fdomain_16x0_reset(NULL, 0);
#endif
	}
	break;
    }
    return 0;
} /* fdomain_event */

/*====================================================================*/

int init_module(void) {
    servinfo_t serv;
    DEBUG(0, "%s\n", version);
    CardServices(GetCardServicesInfo, &serv);
    if (serv.Revision != CS_RELEASE_CODE) {
	printk(KERN_NOTICE "fdomain_cs: Card Services release "
	       "does not match!\n");
	return -1;
    }
    register_pcmcia_driver(&dev_info, &fdomain_attach, &fdomain_detach);
    return 0;
}

void cleanup_module(void) {
    DEBUG(0, "fdomain_cs: unloading\n");
    unregister_pcmcia_driver(&dev_info);
    while (dev_list != NULL)
	fdomain_detach(dev_list);
}
