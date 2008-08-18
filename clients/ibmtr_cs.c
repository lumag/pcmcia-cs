/*======================================================================

    A PCMCIA token-ring driver for IBM-based cards

    This driver supports the IBM PCMCIA Token-Ring Card.
    Written by Steve Kipisz, kipisz@vnet.ibm.com or
                             bungy@ibm.net

    Written 1995,1996.

    This code is based on pcnet_cs.c from David Hinds.

======================================================================*/

#include <pcmcia/config.h>
#include <pcmcia/k_compat.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/system.h>

#include <linux/netdevice.h>
#include <linux/trdevice.h>
#include <drivers/net/ibmtr.h>

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
"ibmtr_cs.c 1.10 1996/01/06 05:19:00 (Steve Kipisz)";
#else
#define DEBUG(n, args...)
#endif

/*====================================================================*/

/* Parameters that can be set with 'insmod' */

/* Bit map of interrupts to choose from */
static u_int irq_mask = 0xdeb8;
static int irq_list[4] = { -1 };

/* MMIO base address */
static u_long mmiobase = 0;

/* SRAM base address */
static u_long srambase = 0;

/* SRAM size 8,16,32,64 */
static u_long sramsize = 16;

/* Ringspeed 4,16 */
static int ringspeed = 16;

/* Ugh!  Let the user hardwire the hardware address for queer cards */
static int hw_addr[6] = { 0, /* ... */ };

MODULE_PARM(irq_mask, "i");
MODULE_PARM(irq_list, "1-4i");
MODULE_PARM(mmiobase, "i");
MODULE_PARM(srambase, "i");
MODULE_PARM(sramsize, "i");
MODULE_PARM(ringspeed, "i");
MODULE_PARM(hw_addr, "6i");

/*====================================================================*/

static void ibmtr_config(dev_link_t *link);
static void ibmtr_hw_setup(struct device *dev);
static void ibmtr_release(u_long arg);
static int ibmtr_event(event_t event, int priority,
                       event_callback_args_t *args);

static dev_info_t dev_info = "ibmtr_cs";

static dev_link_t *ibmtr_attach(void);
static void ibmtr_detach(dev_link_t *);

static dev_link_t *dev_list;

extern int trdev_init(struct device *dev);
extern void tok_interrupt(int irq, struct pt_regs *regs);
extern struct timer_list tr_timer;

/*====================================================================*/


typedef struct ibmtr_dev_t {
    struct device       dev;
    dev_node_t          node;
    window_handle_t     sram_win_handle;
} ibmtr_dev_t;

/*====================================================================*/

static void cs_error(client_handle_t handle, int func, int ret)
{
    error_info_t err = { func, ret };
    CardServices(ReportError, handle, &err);
}

/*======================================================================


    ibmtr_attach() creates an "instance" of the driver, allocating
    local data structures for one device.  The device is registered
    with Card Services.

======================================================================*/

static dev_link_t *ibmtr_attach(void)
{
    client_reg_t client_reg;
    dev_link_t *link;
    ibmtr_dev_t *info;
    struct device *dev;
    struct tok_info *ti;
    int i, ret;

    DEBUG(0, "ibmtr_attach()\n");

    /* Create new token-ring device */
    link = kmalloc(sizeof(struct dev_link_t), GFP_KERNEL);
    memset(link, 0, sizeof(struct dev_link_t));
    link->release.function = &ibmtr_release;
    link->release.data = (u_long)link;
    link->io.Attributes1 = IO_DATA_PATH_WIDTH_8;
    link->io.NumPorts1 = 4;
    link->io.IOAddrLines = 16;
    link->irq.Attributes = IRQ_TYPE_EXCLUSIVE | IRQ_HANDLE_PRESENT;
    link->irq.IRQInfo1 = IRQ_INFO2_VALID|IRQ_LEVEL_ID;
    if (irq_list[0] == -1)
	link->irq.IRQInfo2 = irq_mask;
    else
	for (i = 0; i < 4; i++)
	    link->irq.IRQInfo2 |= 1 << irq_list[i];
    link->irq.Handler = &tok_interrupt;
    link->conf.Attributes = CONF_ENABLE_IRQ;
    link->conf.Vcc = 50;
    link->conf.IntType = INT_MEMORY_AND_IO;
    link->conf.Present = PRESENT_OPTION;

    info = kmalloc(sizeof(struct ibmtr_dev_t), GFP_KERNEL);
    memset(info, 0, sizeof(struct ibmtr_dev_t));
    link->irq.Instance = dev = &info->dev;

#if (LINUX_KERNEL_VERSION > VERSION(2,1,16))
    init_trdev(dev, 0);
#endif
    
    ti = kmalloc(sizeof(struct tok_info), GFP_KERNEL);
    memset(ti,0,sizeof(struct tok_info));
    dev->priv = ti;

    trdev_init(dev);
    dev->name = info->node.dev_name;
    dev->tbusy = 1;
    link->priv = info;

    /* Register with Card Services */
    link->next = dev_list;
    dev_list = link;
    client_reg.dev_info = &dev_info;
    client_reg.Attributes = INFO_IO_CLIENT | INFO_CARD_SHARE;
    client_reg.EventMask =
        CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
        CS_EVENT_RESET_PHYSICAL | CS_EVENT_CARD_RESET |
        CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
    client_reg.event_handler = &ibmtr_event;
    client_reg.Version = 0x0210;
    client_reg.event_callback_args.client_data = link;
    ret = CardServices(RegisterClient, &link->handle, &client_reg);
    if (ret != 0) {
        cs_error(link->handle, RegisterClient, ret);
        ibmtr_detach(link);
        return NULL;
    }

    return link;
} /* ibmtr_attach */

/*======================================================================

    This deletes a driver "instance".  The device is de-registered
    with Card Services.  If it has been released, all local data
    structures are freed.  Otherwise, the structures will be freed
    when the device is released.

======================================================================*/

static void ibmtr_detach(dev_link_t *link)
{
    dev_link_t **linkp;
    long flags;

    DEBUG(0, "ibmtr_detach(0x%p)\n", link);

    /* Locate device structure */
    for (linkp = &dev_list; *linkp; linkp = &(*linkp)->next)
        if (*linkp == link) break;
    if (*linkp == NULL)
        return;

    save_flags(flags);
    cli();
    if (tr_timer.next) del_timer(&tr_timer);
    if (link->state & DEV_RELEASE_PENDING) {
        del_timer(&link->release);
        link->state &= ~DEV_RELEASE_PENDING;
    }
    restore_flags(flags);

    if (link->state & DEV_CONFIG) {
        ibmtr_release((u_long)link);
        if (link->state & DEV_STALE_CONFIG) {
            link->state |= DEV_STALE_LINK;
            return;
        }
    }

    if (link->handle)
        CardServices(DeregisterClient, link->handle);

    /* Unlink device structure, free bits */
    *linkp = link->next;
    if (link->priv) {
        struct device *dev = link->priv;
        if (dev->priv)
            kfree_s(dev->priv, sizeof(struct tok_info));
        kfree_s(dev, sizeof(struct ibmtr_dev_t));
    }
    kfree_s(link, sizeof(struct dev_link_t));

} /* ibmtr_detach */

/*======================================================================

    ibmtr_config() is scheduled to run after a CARD_INSERTION event
    is received, to configure the PCMCIA socket, and to make the
    token-ring device available to the system.

======================================================================*/

#define CS_CHECK(fn, args...) \
while ((last_ret=CardServices(last_fn=(fn), args))!=0) goto cs_failed

static void ibmtr_config(dev_link_t *link)
{
    client_handle_t handle;
    tuple_t tuple;
    cisparse_t parse;
    win_req_t req;
    memreq_t mem;
    ibmtr_dev_t *info;
    struct device *dev;
    struct tok_info *ti;
    int i, last_ret, last_fn;
    u_char buf[64];
    unsigned char Shared_Ram_Base;

    handle = link->handle;
    info = link->priv;
    dev = &info->dev;
    ti = dev->priv;

    DEBUG(0, "ibmtr_config(0x%p)\n", link);

    tuple.Attributes = 0;
    tuple.TupleData = buf;
    tuple.TupleDataMax = 64;
    tuple.TupleOffset = 0;
    tuple.DesiredTuple = CISTPL_CONFIG;
    CS_CHECK(GetFirstTuple, handle, &tuple);
    CS_CHECK(GetTupleData, handle, &tuple);
    CS_CHECK(ParseTuple, handle, &tuple, &parse);
    link->conf.ConfigBase = parse.config.base;

    /* Configure card */
    link->state |= DEV_CONFIG;

    link->conf.ConfigIndex = 0x61;

    /* Determine if this is PRIMARY or ALTERNATE. */

    /* Try PRIMARY card at 0xA20-0xA23 */
    link->io.BasePort1 = 0xA20;
    i = CardServices(RequestIO, link->handle, &link->io);
    if (i == CS_SUCCESS) {
	memcpy(info->node.dev_name, "tr0\0", 4);
    } else {
	/* Couldn't get 0xA20-0xA23.  Try ALTERNATE at 0xA24-0xA27. */
	link->io.BasePort1 = 0xA24;
	CS_CHECK(RequestIO, link->handle, &link->io);
	memcpy(info->node.dev_name, "tr1\0", 4);
    }

    dev->base_addr = link->io.BasePort1;

    CS_CHECK(RequestIRQ, link->handle, &link->irq);
    dev->irq = link->irq.AssignedIRQ;
    ti->irq = link->irq.AssignedIRQ;
    ti->global_int_enable=GLOBAL_INT_ENABLE+((dev->irq==9) ? 2 : dev->irq);

    /* Allocate the MMIO memory window */
    req.Attributes = WIN_DATA_WIDTH_16|WIN_MEMORY_TYPE_CM|WIN_ENABLE;
    req.Attributes |= WIN_USE_WAIT;
    req.Base = mmiobase;
    req.Size = 0x2000;
    req.AccessSpeed = 0x81;
    link->win = (window_handle_t)link->handle;
    CS_CHECK(RequestWindow, &link->win, &req);

    mem.CardOffset = req.Base;
    mem.Page = 0;
    CS_CHECK(MapMemPage, link->win, &mem);
    ti->mmio = req.Base;

    /* Allocate the SRAM memory window */
    req.Attributes = WIN_DATA_WIDTH_16|WIN_MEMORY_TYPE_CM|WIN_ENABLE;
    req.Attributes |= WIN_USE_WAIT;
    req.Base = srambase;
    req.Size = sramsize * 1024;
    req.AccessSpeed = 0x81;
    info->sram_win_handle = (window_handle_t)link->handle;
    CS_CHECK(RequestWindow, &info->sram_win_handle, &req);

    mem.CardOffset = req.Base;
    mem.Page = 0;
    CS_CHECK(MapMemPage, info->sram_win_handle, &mem);
    Shared_Ram_Base = req.Base >> 12;
    /*  By setting the ti->sram to NULL, the RRR gets written by ibmtr.c */
    ti->sram = 0;
    ti->sram_base = Shared_Ram_Base;

    CS_CHECK(RequestConfiguration, link->handle, &link->conf);

    /*  Set up the Token-Ring Controller Configuration Register and
        turn on the card.  Check the "Local Area Network Credit Card
        Adapters Technical Reference"  SC30-3585 for this info.  */
    ibmtr_hw_setup(dev);

    dev->tbusy = 0;

#if (LINUX_KERNEL_VERSION <= VERSION(2,1,16))
    i = register_netdev(dev);
#else
    i = register_trdev(dev);
#endif
    if (i != 0) {
#if (LINUX_KERNEL_VERSION <= VERSION(2,1,16))
        printk(KERN_NOTICE "ibmtr_cs: register_netdev() failed\n");
#else
        printk(KERN_NOTICE "ibmtr_cs: register_trdev() failed\n");
#endif
	goto failed;
    }

    link->dev = &info->node;
    link->state &= ~DEV_CONFIG_PENDING;

    printk(KERN_INFO "%s: port %#3lx, irq %d,",
           dev->name, dev->base_addr, dev->irq);
    printk (" mmio %#5lx,", (u_long)ti->mmio);
    printk (" sram %#5lx,", (u_long)ti->sram_base << 12);
    printk ("\n" KERN_INFO "  hwaddr=");
    for (i = 0; i < TR_ALEN; i++)
        printk("%02X", dev->dev_addr[i]);
    printk("\n");
    return;

cs_failed:
    cs_error(link->handle, last_fn, last_ret);
failed:
    ibmtr_release((u_long)link);

} /* ibmtr_config */

/*======================================================================

    After a card is removed, ibmtr_release() will unregister the net
    device, and release the PCMCIA configuration.  If the device is
    still open, this will be postponed until it is closed.

======================================================================*/

static void ibmtr_release(u_long arg)
{
    dev_link_t *link = (dev_link_t *)arg;
    ibmtr_dev_t *info = link->priv;

    DEBUG(0, "ibmtr_release(0x%p)\n", link);

    if (link->open) {
	DEBUG(1, "ibmtr_cs: release postponed, '%s' "
	      "still open\n", info->node.dev_name);
        link->state |= DEV_STALE_CONFIG;
        return;
    }

    if (link->dev)
#if (LINUX_KERNEL_VERSION <= VERSION(2,1,16))
        unregister_netdev(&info->dev);
#else
        unregister_trdev(&info->dev);
#endif
    link->dev = NULL;

    CardServices(ReleaseConfiguration, link->handle);
    CardServices(ReleaseIO, link->handle, &link->io);
    CardServices(ReleaseIRQ, link->handle, &link->irq);
    CardServices(ReleaseWindow, link->win);
    CardServices(ReleaseWindow, info->sram_win_handle);

    link->state &= ~(DEV_CONFIG | DEV_RELEASE_PENDING);
    if (link->state & DEV_STALE_LINK)
        ibmtr_detach(link);

} /* ibmtr_release */

/*======================================================================

    The card status event handler.  Mostly, this schedules other
    stuff to run after an event is received.  A CARD_REMOVAL event
    also sets some flags to discourage the net drivers from trying
    to talk to the card any more.

======================================================================*/

static int ibmtr_event(event_t event, int priority,
                       event_callback_args_t *args)
{
    dev_link_t *link = args->client_data;
    ibmtr_dev_t *info = link->priv;

    DEBUG(1, "ibmtr_event(0x%06x)\n", event);

    switch (event) {
    case CS_EVENT_CARD_REMOVAL:
        link->state &= ~DEV_PRESENT;
        if (link->state & DEV_CONFIG) {
            info->dev.tbusy = 1; info->dev.start = 0;
            link->release.expires = RUN_AT(HZ/20);
            link->state |= DEV_RELEASE_PENDING;
            add_timer(&link->release);
        }
        break;
    case CS_EVENT_CARD_INSERTION:
        link->state |= DEV_PRESENT;
        ibmtr_config(link);
        break;
    case CS_EVENT_PM_SUSPEND:
        link->state |= DEV_SUSPEND;
        /* Fall through... */
    case CS_EVENT_RESET_PHYSICAL:
        if (link->state & DEV_CONFIG) {
            if (link->open) {
                info->dev.tbusy = 1; info->dev.start = 0;
            }
            CardServices(ReleaseConfiguration, link->handle);
        }
        break;
    case CS_EVENT_PM_RESUME:
        link->state &= ~DEV_SUSPEND;
        /* Fall through... */
    case CS_EVENT_CARD_RESET:
        if (link->state & DEV_CONFIG) {
            CardServices(RequestConfiguration, link->handle, &link->conf);
            if (link->open) {
            /*  pcnet_reset_8390(&info->dev);
                NS8390_init(&info->dev, 1);  */
                ((&info->dev)->init)(&info->dev);
                info->dev.tbusy = 0; info->dev.start = 1;
            }
        }
        break;
    }
    return 0;
} /* ibmtr_event */

/*====================================================================*/

static void ibmtr_hw_setup(struct device *dev)
{
    struct tok_info *ti;
    int i, j;
    unsigned char  temp;

    ti = dev->priv;
    i = ((int)ti->mmio >> 16) & 0x0F;
    outb(i, dev->base_addr);
    i = 0x10 | (((int)ti->mmio >> 12) & 0x0E);
    outb(i, dev->base_addr);
    i = 0x26;
    outb(i, dev->base_addr);
    i = 0x34;             /*  Assume 16k for now  */
    if (ringspeed == 16)
	i |= 2;
    if (dev->base_addr == 0xA24)     /* Alternate */
	i |= 1;
    outb(i, dev->base_addr);
    outb(0x40, dev->base_addr);

    /* Get hw address of token ring card */
    j=0;
    for (i=0; i<0x18; i=i+2) {
	temp = readb((ulong)AIP + (ulong)i + ti->mmio) & 0x0f;
	/* Tech ref states must do this */
	ti->hw_address[j]=temp;
	if(j&1)
	    dev->dev_addr[(j/2)]=ti->hw_address[j]+(ti->hw_address[j-1]<<4);
	++j;
    }

    /* Check if we should override the device address. */
    if (hw_addr[0] != 0) {
	for (i=0; i<TR_ALEN; i++)
	    dev->dev_addr[i] = hw_addr[0];
    }
    
    /* get Adapter type:  'F' = Adapter/A, 'E' = 16/4 Adapter II,...*/
    ti->adapter_type = readb(ti->mmio + AIPADAPTYPE);
    
    /* get Data Rate:  F=4Mb, E=16Mb, D=4Mb & 16Mb ?? */
    ti->data_rate = readb(ti->mmio + AIPDATARATE);

    /* Get Early Token Release support?: F=no, E=4Mb, D=16Mb, C=4&16Mb */
    ti->token_release = readb(ti->mmio + AIPEARLYTOKEN);
    
    /* How much shared RAM is on adapter ? */
    ti->avail_shared_ram = 64;    /* for now */
    
    /* We need to set or do a bunch of work here based on previous results.. */
    /* Support paging?  What sizes?:  F=no, E=16k, D=32k, C=16 & 32k */
    ti->shared_ram_paging = readb(ti->mmio + AIPSHRAMPAGE);
    
    /* Available DHB  4Mb size:   F=2048, E=4096, D=4464 */
    ti->dhb_size4mb = readb(ti->mmio + AIP4MBDHB);
    
    /* Available DHB 16Mb size:  F=2048, E=4096, D=8192, C=16384, B=17960 */
    ti->dhb_size16mb = readb(ti->mmio + AIP16MBDHB);
    
    /* For now, no shared ram paging */
    /* determine how much of total RAM is mapped into PC space */
    ti->mapped_ram_size =
	1<<(((readb(ti->mmio+ ACA_OFFSET + ACA_RW + RRR_ODD))>>2)+4);
    ti->page_mask=0;
}


int init_module(void)
{
    servinfo_t serv;
    DEBUG(0, "%s\n", version);
    CardServices(GetCardServicesInfo, &serv);
    if (serv.Revision != CS_RELEASE_CODE) {
        printk(KERN_NOTICE "ibmtr_cs: Card Services release "
	       "does not match!\n");
        return -1;
    }
    register_pcmcia_driver(&dev_info, &ibmtr_attach, &ibmtr_detach);
    return 0;
}

void cleanup_module(void)
{
    DEBUG(0, "ibmtr_cs: unloading\n");
    unregister_pcmcia_driver(&dev_info);
    while (dev_list != NULL)
        ibmtr_detach(dev_list);
}
