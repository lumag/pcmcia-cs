/*======================================================================

    Cardbus device enabler

    Written by David Hinds, dhinds@hyper.stanford.edu

    The general idea:

    A client driver registers using register_driver().  This module
    then creates a Card Services pseudo-client and registers it, and
    configures the socket if this is the first client.  It then
    invokes the appropriate PCI client routines in response to Card
    Services events.  

======================================================================*/

#include <pcmcia/config.h>
#include <pcmcia/k_compat.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/malloc.h>
#include <linux/string.h>
#include <linux/timer.h>

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
"cb_enabler.c 1.5 1998/02/04 17:02:17 (David Hinds)";
#else
#define DEBUG(n, args...)
#endif

/*====================================================================*/

/* Parameters that can be set with 'insmod' */

/*====================================================================*/

typedef struct driver_info_t {
    dev_link_t		*(*attach)(void);
    dev_info_t		dev_info;
    driver_operations	*ops;
    dev_link_t		*dev_list;
} driver_info_t;

static dev_link_t *cb_attach(int n);
#define MK_ENTRY(fn, n) \
static dev_link_t *fn(void) { return cb_attach(n); }

#define MAX_DRIVER	4

MK_ENTRY(attach_0, 0);
MK_ENTRY(attach_1, 1);
MK_ENTRY(attach_2, 2);
MK_ENTRY(attach_3, 3);

static driver_info_t driver[4] = {
    { attach_0 }, { attach_1 }, { attach_2 }, { attach_3 }
};

typedef struct bus_info_t {
    u_char		bus;
    int			ndev;
    struct bus_info_t	*next;
} bus_info_t;

static void cb_release(u_long arg);
static int cb_event(event_t event, int priority,
		    event_callback_args_t *args);

static void cb_detach(dev_link_t *);

static bus_info_t *bus_list = NULL;

/*====================================================================*/

static void cs_error(client_handle_t handle, int func, int ret)
{
    error_info_t err = { func, ret };
    CardServices(ReportError, handle, &err);
}

/*====================================================================*/

struct dev_link_t *cb_attach(int n)
{
    client_reg_t client_reg;
    dev_link_t *link;
    int ret;
    
    DEBUG(0, "cb_attach(%d)\n", n);

    link = kmalloc(sizeof(struct dev_link_t), GFP_KERNEL);
    memset(link, 0, sizeof(struct dev_link_t));
    link->release.function = &cb_release;
    link->release.data = (u_long)link;

    link->conf.Vcc = 33;
    link->conf.IntType = INT_MEMORY_AND_IO;

    /* Insert into instance chain for this driver */
    link->priv = &driver[n];
    link->next = driver[n].dev_list;
    driver[n].dev_list = link;
    
    /* Register with Card Services */
    client_reg.dev_info = &driver[n].dev_info;
    client_reg.Attributes = INFO_IO_CLIENT | INFO_CARD_SHARE;
    client_reg.event_handler = &cb_event;
    client_reg.EventMask =
	CS_EVENT_RESET_REQUEST | CS_EVENT_CARD_RESET |
	CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
	CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
    client_reg.Version = 0x0210;
    client_reg.event_callback_args.client_data = link;
    ret = CardServices(RegisterClient, &link->handle, &client_reg);
    if (ret != 0) {
	cs_error(link->handle, RegisterClient, ret);
	cb_detach(link);
	return NULL;
    }
    return link;
}

/*====================================================================*/

static void cb_detach(dev_link_t *link)
{
    driver_info_t *dev = link->priv;
    dev_link_t **linkp;

    DEBUG(0, "cb_detach(0x%p)\n", link);
    
    /* Locate device structure */
    for (linkp = &dev->dev_list; *linkp; linkp = &(*linkp)->next)
	if (*linkp == link) break;
    if (*linkp == NULL)
	return;

    if (link->state & DEV_CONFIG)
	cb_release((u_long)link);

    if (link->handle)
	CardServices(DeregisterClient, link->handle);
    
    *linkp = link->next;
    kfree_s(link, sizeof(struct dev_link_t));
}

/*====================================================================*/

static void cb_config(dev_link_t *link)
{
    client_handle_t handle = link->handle;
    driver_info_t *drv = link->priv;
    dev_locator_t loc;
    bus_info_t *b;
    config_info_t config;
    u_char bus, devfn;
    int i;
    
    DEBUG(0, "cb_config(0x%p)\n", link);

    /* Get PCI bus info */
    CardServices(GetConfigurationInfo, handle, &config);
    bus = config.Option; devfn = config.Function;

    /* Is this a new bus? */
    for (b = bus_list; b != NULL; b = b->next)
	if (bus == b->bus) break;
    if (!b) {
	b = kmalloc(sizeof(bus_info_t), GFP_KERNEL);
	b->bus = bus;
	b->next = bus_list;
	b->ndev = 0;
	bus_list = b;

	/* Special hook: CS know what to do... */
	i = CardServices(RequestIO, handle, NULL);
	if (i != CS_SUCCESS) {
	    cs_error(handle, RequestIO, i);
	    return;
	}
	i = CardServices(RequestConfiguration, handle, NULL);
	if (i != CS_SUCCESS) {
	    cs_error(handle, RequestConfiguration, i);
	    return;
	}
    }
    link->win = (void *)b;
    loc.bus = LOC_PCI;
    loc.b.pci.bus = bus;
    loc.b.pci.devfn = devfn;
    link->dev = drv->ops->attach(&loc);
    b->ndev++;
    
    link->state &= ~DEV_CONFIG_PENDING;
    link->state |= DEV_CONFIG;
}

/*====================================================================*/

static void cb_release(u_long arg)
{
    dev_link_t *link = (dev_link_t *)arg;
    driver_info_t *drv = link->priv;
    bus_info_t *b = (void *)link->win;

    DEBUG(0, "cb_release(0x%p)\n", link);

    if (link->dev != NULL) {
	drv->ops->detach(link->dev);
	link->dev = NULL;
    }
    if (link->state & DEV_CONFIG) {
	b->ndev--;
	if (b->ndev == 0) {
	    CardServices(ReleaseConfiguration, link->handle);
	    CardServices(ReleaseIO, link->handle, NULL);
	}
    }
    link->state &= ~DEV_CONFIG;
}

/*====================================================================*/

static int cb_event(event_t event, int priority,
		    event_callback_args_t *args)
{
    dev_link_t *link = args->client_data;
    driver_info_t *drv = link->priv;
    bus_info_t *b = (void *)link->win;
    
    DEBUG(0, "cb_event(0x%06x)\n", event);
    
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
	cb_config(link);
	break;
    case CS_EVENT_PM_SUSPEND:
	link->state |= DEV_SUSPEND;
	/* Fall through... */
    case CS_EVENT_RESET_PHYSICAL:
	if (link->state & DEV_CONFIG) {
	    if (drv->ops->suspend != NULL)
		drv->ops->suspend(link->dev);
	    b->ndev--;
	    if (b->ndev == 0)
		CardServices(ReleaseConfiguration, link->handle);
	}
	break;
    case CS_EVENT_PM_RESUME:
	link->state &= ~DEV_SUSPEND;
	/* Fall through... */
    case CS_EVENT_CARD_RESET:
	if (link->state & DEV_CONFIG) {
	    b->ndev++;
	    if (b->ndev == 1)
		CardServices(RequestConfiguration, link->handle, NULL);
	    if (drv->ops->resume != NULL)
		drv->ops->resume(link->dev);
	}
	break;
    }
    return 0;
}

/*====================================================================*/

int register_driver(struct driver_operations *ops)
{
    int i;
    
    DEBUG(0, "register_driver('%s')\n", ops->name);

    for (i = 0; i < MAX_DRIVER; i++)
	if (driver[i].ops == NULL) break;
    if (i == MAX_DRIVER)
	return -1;

    MOD_INC_USE_COUNT;
    driver[i].ops = ops;
    strcpy(driver[i].dev_info, ops->name);
    register_pcmcia_driver(&driver[i].dev_info, driver[i].attach,
			   &cb_detach);
    return 0;
}

void unregister_driver(struct driver_operations *ops)
{
    int i;

    DEBUG(0, "unregister_driver('%s')\n", ops->name);
    for (i = 0; i < MAX_DRIVER; i++)
	if (driver[i].ops == ops) break;
    if (i < MAX_DRIVER) {
	unregister_pcmcia_driver(&driver[i].dev_info);
	driver[i].ops = NULL;
	MOD_DEC_USE_COUNT;
    }
}

/*====================================================================*/

int init_module(void) {
    servinfo_t serv;
    DEBUG(0, "%s\n", version);
    CardServices(GetCardServicesInfo, &serv);
    if (serv.Revision != CS_RELEASE_CODE) {
	printk(KERN_NOTICE "cb_enabler: Card Services release "
	       "does not match!\n");
	return -1;
    }
    return 0;
}

void cleanup_module(void) {
    DEBUG(0, "cb_enabler: unloading\n");
}
