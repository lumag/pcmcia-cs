/*======================================================================

    A PCMCIA ethernet driver for the 3com 3c589 card.
    
    Copyright (C) 1998 David A. Hinds -- dhinds@hyper.stanford.edu

    3c589_cs.c 1.104 1998/04/19 11:51:03

    The network driver code is based on Donald Becker's 3c589 code:
    
    Written 1994 by Donald Becker.
    Copyright 1993 United States Government as represented by the
    Director, National Security Agency.  This software may be used and
    distributed according to the terms of the GNU Public License,
    incorporated herein by reference.
    Donald Becker may be reached at becker@cesdis1.gsfc.nasa.gov

======================================================================*/

#include <pcmcia/config.h>
#include <pcmcia/k_compat.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/in.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/bitops.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/if_arp.h>
#include <linux/ioport.h>

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/cisreg.h>
#include <pcmcia/ciscode.h>
#include <pcmcia/ds.h>

/* To minimize the size of the driver source I only define operating
   constants if they are used several times.  You'll need the manual
   if you want to understand driver details. */
/* Offsets from base I/O address. */
#define EL3_DATA	0x00
#define EL3_CMD		0x0e
#define EL3_STATUS	0x0e
#define ID_PORT		0x100
#define	EEPROM_READ	0x80

#define EL3WINDOW(win_num) outw(SelectWindow + (win_num), ioaddr + EL3_CMD)

/* The top five bits written to EL3_CMD are a command, the lower
   11 bits are the parameter, if applicable. */
enum c509cmd {
    TotalReset = 0<<11, SelectWindow = 1<<11, StartCoax = 2<<11,
    RxDisable = 3<<11, RxEnable = 4<<11, RxReset = 5<<11, RxDiscard = 8<<11,
    TxEnable = 9<<11, TxDisable = 10<<11, TxReset = 11<<11,
    FakeIntr = 12<<11, AckIntr = 13<<11, SetIntrEnb = 14<<11,
    SetStatusEnb = 15<<11, SetRxFilter = 16<<11, SetRxThreshold = 17<<11,
    SetTxThreshold = 18<<11, SetTxStart = 19<<11, StatsEnable = 21<<11,
    StatsDisable = 22<<11, StopCoax = 23<<11,
};

enum c509status {
    IntLatch = 0x0001, AdapterFailure = 0x0002, TxComplete = 0x0004,
    TxAvailable = 0x0008, RxComplete = 0x0010, RxEarly = 0x0020,
    IntReq = 0x0040, StatsFull = 0x0080, CmdBusy = 0x1000 };

/* The SetRxFilter command accepts the following classes: */
enum RxFilter {
    RxStation = 1, RxMulticast = 2, RxBroadcast = 4, RxProm = 8
};

/* Register window 1 offsets, the window used in normal operation. */
#define TX_FIFO		0x00
#define RX_FIFO		0x00
#define RX_STATUS 	0x08
#define TX_STATUS 	0x0B
#define TX_FREE		0x0C	/* Remaining free bytes in Tx buffer. */

#define WN0_IRQ		0x08	/* Window 0: Set IRQ line in bits 12-15. */
#define WN4_MEDIA	0x0A	/* Window 4: Various transcvr/media bits. */
#define MEDIA_TP	0x00C0	/* Enable link beat and jabber for 10baseT. */

struct el3_private {
    dev_node_t node;
    struct net_device_stats stats;
    int probe_port;
    int flipped;
};

static char *if_names[] = { "Auto", "10baseT", "10base2", "AUI" };

#ifdef PCMCIA_DEBUG
static int pc_debug = PCMCIA_DEBUG;
MODULE_PARM(pc_debug, "i");
#define DEBUG(n, args...) if (pc_debug>(n)) printk(KERN_DEBUG args)
static char *version =
"3c589_cs.c 1.109 1998/11/03 05:31:23 (David Hinds)";
#else
#define DEBUG(n, args...)
#endif

/*====================================================================*/

/* Parameters that can be set with 'insmod' */

/* Special hook for setting if_port when module is loaded */
static int if_port = 0;

/* Bit map of interrupts to choose from */
static u_int irq_mask = 0xdeb8;
static int irq_list[4] = { -1 };

MODULE_PARM(if_port, "i");
MODULE_PARM(irq_mask, "i");
MODULE_PARM(irq_list, "1-4i");

/*====================================================================*/

static void tc589_config(dev_link_t *link);
static void tc589_release(u_long arg);
static int tc589_event(event_t event, int priority,
		       event_callback_args_t *args);

static ushort read_eeprom(short ioaddr, int index);
static void tc589_reset(struct device *dev);
static void check_if_port(struct device *dev);
static int el3_config(struct device *dev, struct ifmap *map);
static int el3_open(struct device *dev);
static int el3_start_xmit(struct sk_buff *skb, struct device *dev);
static void el3_interrupt IRQ(int irq, void *dev_id, struct pt_regs *regs);
static void update_stats(int addr, struct device *dev);
static struct net_device_stats *el3_get_stats(struct device *dev);
static int el3_rx(struct device *dev);
static int el3_close(struct device *dev);

#ifdef NEW_MULTICAST
static void set_multicast_list(struct device *dev);
#else
static void set_multicast_list(struct device *dev, int num_addrs, void *addrs);
#endif

static dev_info_t dev_info = "3c589_cs";

static dev_link_t *tc589_attach(void);
static void tc589_detach(dev_link_t *);

static dev_link_t *dev_list = NULL;

/*====================================================================*/

static void cs_error(client_handle_t handle, int func, int ret)
{
    error_info_t err = { func, ret };
    CardServices(ReportError, handle, &err);
}

/*======================================================================

    We never need to do anything when a tc589 device is "initialized"
    by the net software, because we only register already-found cards.
    
======================================================================*/

static int tc589_init(struct device *dev)
{
    return 0;
}

/*======================================================================

    tc589_attach() creates an "instance" of the driver, allocating
    local data structures for one device.  The device is registered
    with Card Services.

======================================================================*/

static dev_link_t *tc589_attach(void)
{
    client_reg_t client_reg;
    dev_link_t *link;
    struct device *dev;
    int i, ret;

    DEBUG(0, "3c589_attach()\n");

    /* Create new ethernet device */
    link = kmalloc(sizeof(struct dev_link_t), GFP_KERNEL);
    memset(link, 0, sizeof(struct dev_link_t));
    link->release.function = &tc589_release;
    link->release.data = (u_long)link;
    link->io.NumPorts1 = 16;
    link->io.Attributes1 = IO_DATA_PATH_WIDTH_16;
    link->io.IOAddrLines = 4;
    link->irq.Attributes = IRQ_TYPE_EXCLUSIVE | IRQ_HANDLE_PRESENT;
    link->irq.IRQInfo1 = IRQ_INFO2_VALID|IRQ_LEVEL_ID;
    if (irq_list[0] == -1)
	link->irq.IRQInfo2 = irq_mask;
    else
	for (i = 0; i < 4; i++)
	    link->irq.IRQInfo2 |= 1 << irq_list[i];
    link->irq.Handler = &el3_interrupt;
    link->conf.Attributes = CONF_ENABLE_IRQ;
    link->conf.Vcc = 50;
    link->conf.IntType = INT_MEMORY_AND_IO;
    link->conf.ConfigIndex = 1;
    link->conf.Present = PRESENT_OPTION;
    
    dev = kmalloc(sizeof(struct device), GFP_KERNEL);
    memset(dev, 0, sizeof(struct device));
    
    /* Make up a EL3-specific-data structure. */
    dev->priv = kmalloc(sizeof(struct el3_private), GFP_KERNEL);
    memset(dev->priv, 0, sizeof(struct el3_private));
    
    /* The EL3-specific entries in the device structure. */
    dev->hard_start_xmit = &el3_start_xmit;
    dev->set_config = &el3_config;
    dev->get_stats = &el3_get_stats;
    dev->set_multicast_list = &set_multicast_list;
    ether_setup(dev);
    dev->name = ((struct el3_private *)dev->priv)->node.dev_name;
    dev->init = &tc589_init;
    dev->open = &el3_open;
    dev->stop = &el3_close;
    dev->tbusy = 1;
    link->priv = link->irq.Instance = dev;
    
    /* Register with Card Services */
    link->next = dev_list;
    dev_list = link;
    client_reg.dev_info = &dev_info;
    client_reg.Attributes = INFO_IO_CLIENT | INFO_CARD_SHARE;
    client_reg.EventMask =
	CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
	CS_EVENT_RESET_PHYSICAL | CS_EVENT_CARD_RESET |
	CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
    client_reg.event_handler = &tc589_event;
    client_reg.Version = 0x0210;
    client_reg.event_callback_args.client_data = link;
    ret = CardServices(RegisterClient, &link->handle, &client_reg);
    if (ret != 0) {
	cs_error(link->handle, RegisterClient, ret);
	tc589_detach(link);
	return NULL;
    }
    
    return link;
} /* tc589_attach */

/*======================================================================

    This deletes a driver "instance".  The device is de-registered
    with Card Services.  If it has been released, all local data
    structures are freed.  Otherwise, the structures will be freed
    when the device is released.

======================================================================*/

static void tc589_detach(dev_link_t *link)
{
    dev_link_t **linkp;
    long flags;
    
    DEBUG(0, "3c589_detach(0x%p)\n", link);
    
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
    
    if (link->state & DEV_CONFIG) {
	tc589_release((u_long)link);
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
	    kfree_s(dev->priv, sizeof(struct el3_private));
	kfree_s(link->priv, sizeof(struct device));
    }
    kfree_s(link, sizeof(struct dev_link_t));
    
} /* tc589_detach */

/*======================================================================

    tc589_config() is scheduled to run after a CARD_INSERTION event
    is received, to configure the PCMCIA socket, and to make the
    ethernet device available to the system.
    
======================================================================*/

#define CS_CHECK(fn, args...) \
while ((last_ret=CardServices(last_fn=(fn), args))!=0) goto cs_failed

static void tc589_config(dev_link_t *link)
{
    client_handle_t handle;
    struct device *dev;
    tuple_t tuple;
    cisparse_t parse;
    u_short buf[32];
    int last_fn, last_ret, i, j, multi = 0;
    short ioaddr, *phys_addr;
    
    handle = link->handle;
    dev = link->priv;
    phys_addr = (short *)dev->dev_addr;

    DEBUG(0, "3c589_config(0x%p)\n", link);

    tuple.Attributes = 0;
    tuple.DesiredTuple = CISTPL_CONFIG;
    CS_CHECK(GetFirstTuple, handle, &tuple);
    tuple.TupleData = (cisdata_t *)buf;
    tuple.TupleDataMax = sizeof(buf);
    tuple.TupleOffset = 0;
    CS_CHECK(GetTupleData, handle, &tuple);
    CS_CHECK(ParseTuple, handle, &tuple, &parse);
    link->conf.ConfigBase = parse.config.base;
    link->conf.Present = parse.config.rmask[0];
    
    /* Is this a 3c562? */
    tuple.DesiredTuple = CISTPL_MANFID;
    tuple.Attributes = TUPLE_RETURN_COMMON;
    if ((CardServices(GetFirstTuple, handle, &tuple) == CS_SUCCESS) &&
	(CardServices(GetTupleData, handle, &tuple) == CS_SUCCESS)) {
	if (le16_to_cpu(buf[0]) != MANFID_3COM)
	    printk(KERN_INFO "3c589_cs: hmmm, is this really a "
		   "3Com card??\n");
	multi = (le16_to_cpu(buf[1]) == PRODID_3COM_3C562);
    }
    
    /* Configure card */
    link->state |= DEV_CONFIG;

    /* For the 3c562, the base address must be xx00-xx7f */
    for (i = j = 0; j < 0x400; j += 0x10) {
	if (multi && (j & 0x80)) continue;
	link->io.BasePort1 = j ^ 0x300;
	i = CardServices(RequestIO, link->handle, &link->io);
	if (i == CS_SUCCESS) break;
    }
    if (i != CS_SUCCESS) {
	cs_error(link->handle, RequestIO, i);
	goto failed;
    }
    CS_CHECK(RequestIRQ, link->handle, &link->irq);
    CS_CHECK(RequestConfiguration, link->handle, &link->conf);
	
    dev->irq = link->irq.AssignedIRQ;
    dev->base_addr = link->io.BasePort1;
    dev->tbusy = 0;
    if (register_netdev(dev) != 0) {
	printk(KERN_NOTICE "3c589_cs: register_netdev() failed\n");
	goto failed;
    }
    
    link->state &= ~DEV_CONFIG_PENDING;
    ioaddr = dev->base_addr;
    EL3WINDOW(0);

    /* The 3c589 has an extra EEPROM for configuration info, including
       the hardware address.  The 3c562 puts the address in the CIS. */
    tuple.DesiredTuple = 0x88;
    if (CardServices(GetFirstTuple, handle, &tuple) == CS_SUCCESS) {
	CardServices(GetTupleData, handle, &tuple);
	for (i = 0; i < 3; i++)
	    phys_addr[i] = htons(buf[i]);
    } else {
	for (i = 0; i < 3; i++)
	    phys_addr[i] = htons(read_eeprom(ioaddr, i));
	if (phys_addr[0] == 0x6060) {
	    printk(KERN_NOTICE "3c589_cs: IO port conflict at 0x%03lx"
		   "-0x%03lx\n", dev->base_addr, dev->base_addr+15);
	    goto failed;
	}
    }
    
    link->dev = &((struct el3_private *)dev->priv)->node;
    
    /* The address and resource configuration register aren't loaded from
       the EEPROM and *must* be set to 0 and IRQ3 for the PCMCIA version. */
    outw(0x3f00, ioaddr + 8);

    /* The if_port symbol can be set when the module is loaded */
    if ((if_port >= 0) && (if_port <= 3))
	dev->if_port = if_port;
    else
	printk(KERN_NOTICE "3c589_cs: invalid if_port requested\n");
    
    /* This is vital: the transceiver used must be set in the resource
       configuration register.  It took me many hours to discover this. */
    switch (dev->if_port) {
    case 0: case 1: outw(0, ioaddr + 6); break;
    case 2: outw(3<<14, ioaddr + 6); break;
    case 3: outw(1<<14, ioaddr + 6); break;
    }
    
    printk(KERN_INFO "%s: 3Com 3c%s, port %#3lx, irq %d, %s port, "
	   "hw_addr ", dev->name, (multi ? "562" : "589"),
	   dev->base_addr, dev->irq, if_names[dev->if_port]);
    for (i = 0; i < 6; i++)
	printk("%02X%s", dev->dev_addr[i], ((i<5) ? ":" : "\n"));
    return;

cs_failed:
    cs_error(link->handle, last_fn, last_ret);
failed:
    tc589_release((u_long)link);
    return;
    
} /* tc589_config */

/*======================================================================

    After a card is removed, tc589_release() will unregister the net
    device, and release the PCMCIA configuration.  If the device is
    still open, this will be postponed until it is closed.
    
======================================================================*/

static void tc589_release(u_long arg)
{
    dev_link_t *link = (dev_link_t *)arg;
    struct device *dev = link->priv;

    DEBUG(0, "3c589_release(0x%p)\n", link);
    
    if (link->open) {
	DEBUG(1, "3c589_cs: release postponed, '%s' still open\n",
	      link->dev->dev_name);
	link->state |= DEV_STALE_CONFIG;
	return;
    }
    
    CardServices(ReleaseConfiguration, link->handle);
    CardServices(ReleaseIO, link->handle, &link->io);
    CardServices(ReleaseIRQ, link->handle, &link->irq);
    if (link->dev)
	unregister_netdev(dev);
    link->dev = NULL;
    
    link->state &= ~DEV_CONFIG;
    if (link->state & DEV_STALE_LINK)
	tc589_detach(link);
    
} /* tc589_release */

/*======================================================================

    The card status event handler.  Mostly, this schedules other
    stuff to run after an event is received.  A CARD_REMOVAL event
    also sets some flags to discourage the net drivers from trying
    to talk to the card any more.
    
======================================================================*/

static int tc589_event(event_t event, int priority,
		       event_callback_args_t *args)
{
    dev_link_t *link = args->client_data;
    struct device *dev = link->priv;
    
    DEBUG(1, "3c589_event(0x%06x)\n", event);
    
    switch (event) {
    case CS_EVENT_CARD_REMOVAL:
	link->state &= ~DEV_PRESENT;
	if (link->state & DEV_CONFIG) {
	    dev->tbusy = 1; dev->start = 0;
	    link->release.expires = RUN_AT(HZ/20);
	    add_timer(&link->release);
	}
	break;
    case CS_EVENT_CARD_INSERTION:
	link->state |= DEV_PRESENT | DEV_CONFIG_PENDING;
	tc589_config(link);
	break;
    case CS_EVENT_PM_SUSPEND:
	link->state |= DEV_SUSPEND;
	/* Fall through... */
    case CS_EVENT_RESET_PHYSICAL:
	if (link->state & DEV_CONFIG) {
	    if (link->open) {
		dev->tbusy = 1; dev->start = 0;
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
		tc589_reset(dev);
		dev->tbusy = 0; dev->start = 1;
	    }
	}
	break;
    }
    return 0;
} /* tc589_event */

/*====================================================================*/

/* Read a word from the EEPROM using the regular EEPROM access register.
   Assume that we are in register window zero.
 */
static ushort read_eeprom(short ioaddr, int index)
{
    outw(EEPROM_READ + index, ioaddr + 10);
    /* Pause for at least 162 us. for the read to take place. */
    udelay(300L);
    return inw(ioaddr + 12);
}

/* Reset and restore all of the 3c589 registers. */
static void tc589_reset(struct device *dev)
{
    ushort ioaddr = dev->base_addr;
    int i;
    
    EL3WINDOW(0);
    outw(0x0001, ioaddr + 4);			/* Activate board. */ 
    switch (dev->if_port) {			/* Set the xcvr */
    case 0: case 1: outw(0, ioaddr + 6); break;
    case 2: outw(3<<14, ioaddr + 6); break;
    case 3: outw(1<<14, ioaddr + 6); break;
    }
    outw(0x3f00, ioaddr + 8);			/* Set the IRQ line. */
    
    /* Set the station address in window 2. */
    EL3WINDOW(2);
    for (i = 0; i < 6; i++)
	outb(dev->dev_addr[i], ioaddr + i);
    
    if (dev->if_port == 2)
	/* Start the thinnet transceiver. We should really wait 50ms...*/
	outw(StartCoax, ioaddr + EL3_CMD);
    else if (dev->if_port < 2) {
	/* 10baseT interface, enabled link beat and jabber check. */
	EL3WINDOW(4);
	outw(inw(ioaddr + WN4_MEDIA) | MEDIA_TP, ioaddr + WN4_MEDIA);
    }

    /* Switch to the stats window, and clear all stats by reading. */
    outw(StatsDisable, ioaddr + EL3_CMD);
    EL3WINDOW(6);
    for (i = 0; i < 9; i++)
	inb(ioaddr+i);
    inw(ioaddr + 10);
    inw(ioaddr + 12);
    
    /* Switch to register set 1 for normal use. */
    EL3WINDOW(1);

    /* Accept b-cast and phys addr only. */
    outw(SetRxFilter | RxStation | RxBroadcast, ioaddr + EL3_CMD);
    outw(StatsEnable, ioaddr + EL3_CMD); /* Turn on statistics. */
    outw(RxEnable, ioaddr + EL3_CMD); /* Enable the receiver. */
    outw(TxEnable, ioaddr + EL3_CMD); /* Enable transmitter. */
    /* Allow status bits to be seen. */
    outw(SetStatusEnb | 0xff, ioaddr + EL3_CMD);
    /* Ack all pending events, and set active indicator mask. */
    outw(AckIntr | IntLatch | TxAvailable | RxEarly | IntReq,
	 ioaddr + EL3_CMD);
    outw(SetIntrEnb | IntLatch | TxAvailable | RxComplete | StatsFull,
	 ioaddr + EL3_CMD);
}

static int el3_config(struct device *dev, struct ifmap *map)
{
    struct el3_private *lp = (struct el3_private *)dev->priv;
    if ((map->port != (u_char)(-1)) && (map->port != dev->if_port)) {
	if (map->port <= 3) {
	    lp->probe_port = 0;
	    dev->if_port = map->port;
	    printk(KERN_INFO "%s: switched to %s port\n",
		   dev->name, if_names[dev->if_port]);
	} else
	    return -EINVAL;
    }
    return 0;
}

static int el3_open(struct device *dev)
{
    struct el3_private *lp = (struct el3_private *)dev->priv;
    int ioaddr = dev->base_addr;
    dev_link_t *link;
    
    for (link = dev_list; link; link = link->next)
	if (link->priv == dev) break;
    if (!DEV_OK(link))
	return -ENODEV;

    link->open++;
    MOD_INC_USE_COUNT;

    EL3WINDOW(0);
    
    dev->interrupt = 0; dev->tbusy = 0; dev->start = 1;
    if (dev->if_port == 0) {
	lp->probe_port = 15; lp->flipped = 0;
    }
    tc589_reset(dev);

    DEBUG(2, "%s: opened, status %4.4x.\n",
	  dev->name, inw(ioaddr + EL3_STATUS));
    
    return 0;					/* Always succeed */
}

static int el3_start_xmit(struct sk_buff *skb, struct device *dev)
{
    struct el3_private *lp = (struct el3_private *)dev->priv;
    int ioaddr = dev->base_addr;

    /* Transmitter timeout, serious problems. */
    if (dev->tbusy) {
	int tickssofar = jiffies - dev->trans_start;
	if (tickssofar < 10)
	    return 1;
	printk(KERN_NOTICE "%s: transmit timed out, Tx_status %2.2x "
	       "status %4.4x Tx FIFO room %d.", dev->name,
	       inb(ioaddr + TX_STATUS), inw(ioaddr + EL3_STATUS),
	       inw(ioaddr + TX_FREE));
	lp->stats.tx_errors++;
	dev->trans_start = jiffies;
	/* Issue TX_RESET and TX_START commands. */
	outw(TxReset, ioaddr + EL3_CMD);
	outw(TxEnable, ioaddr + EL3_CMD);
	dev->tbusy = 0;
    }

#if (LINUX_VERSION_CODE < VERSION(2,1,25))
    if (skb == NULL) {
	dev_tint(dev);
	return 0;
    }
    if (skb->len <= 0)
	return 0;
#endif

    DEBUG(3, "%s: el3_start_xmit(length = %ld) called, "
	  "status %4.4x.\n", dev->name, (long)skb->len,
	  inw(ioaddr + EL3_STATUS));

#ifdef PCMCIA_DEBUG
    {	/* Error-checking code, delete someday. */
	ushort status = inw(ioaddr + EL3_STATUS);
	if (status & 0x0003 		/* IRQ line active, missed one. */
	    && inw(ioaddr + EL3_STATUS) & 3) { 		/* Make sure. */
	    printk(KERN_NOTICE "%s: missed interrupt, status then %04x"
		   " now %04x  Tx %2.2x Rx %4.4x.\n", dev->name, status,
		   inw(ioaddr + EL3_STATUS), inb(ioaddr + TX_STATUS),
		   inw(ioaddr + RX_STATUS));
	    if (status & 0x0002) /* Adaptor failure */
		tc589_reset(dev);
	    /* Fake interrupt trigger by masking, acknowledge interrupts. */
	    outw(SetStatusEnb | 0x00, ioaddr + EL3_CMD);
	    outw(AckIntr | IntLatch | TxAvailable | RxEarly | IntReq,
		 ioaddr + EL3_CMD);
	    outw(SetStatusEnb | 0xff, ioaddr + EL3_CMD);
	}
    }
#endif
    
    /* Avoid timer-based retransmission conflicts. */
    if (test_and_set_bit(0, (void*)&dev->tbusy) != 0)
	printk(KERN_NOTICE "%s: transmitter access conflict.\n",
	       dev->name);
    else {
#if (LINUX_VERSION_CODE >= VERSION(2,1,25))
	lp->stats.tx_bytes += skb->len;
#endif
	/* Put out the doubleword header... */
	outw(skb->len, ioaddr + TX_FIFO);
	outw(0x00, ioaddr + TX_FIFO);
	/* ... and the packet rounded to a doubleword. */
	outsl_ns(ioaddr + TX_FIFO, skb->data, (skb->len + 3) >> 2);
	
	dev->trans_start = jiffies;
	if (inw(ioaddr + TX_FREE) > 1536) {
	    dev->tbusy = 0;
	} else
	    /* Interrupt us when the FIFO has room for max-sized packet. */
	    outw(SetTxThreshold + 1536, ioaddr + EL3_CMD);
    }

    DEV_KFREE_SKB (skb);
    
    /* Clear the Tx status stack. */
    {
	short tx_status;
	int i = 4;
	
	while ((--i > 0) && (tx_status = inb(ioaddr + TX_STATUS)) > 0) {
	    if (tx_status & 0x38) lp->stats.tx_aborted_errors++;
	    if (tx_status & 0x30) outw(TxReset, ioaddr + EL3_CMD);
	    if (tx_status & 0x3C) outw(TxEnable, ioaddr + EL3_CMD);
	    outb(0x00, ioaddr + TX_STATUS); /* Pop the status stack. */
	}
    }

    if (lp->probe_port)
	check_if_port(dev);
    
    return 0;
}

/* The EL3 interrupt handler. */
static void el3_interrupt IRQ(int irq, void *dev_id, struct pt_regs *regs)
{
    struct device *dev = (struct device *)DEV_ID;
    struct el3_private *lp;
    int ioaddr, status;
    int i = 0;
    
    if ((dev == NULL) || !dev->start)
	return;
    lp = (struct el3_private *)dev->priv;
    ioaddr = dev->base_addr;

#ifdef PCMCIA_DEBUG
    if (dev->interrupt) {
	printk(KERN_NOTICE "%s: re-entering the interrupt handler.\n",
	       dev->name);
	return;
    }
    dev->interrupt = 1;
    DEBUG(3, "%s: interrupt, status %4.4x.\n",
	  dev->name, inw(ioaddr + EL3_STATUS));
#endif
    
    while ((status = inw(ioaddr + EL3_STATUS)) &
	(IntLatch | RxComplete | StatsFull)) {
	if ((dev->start == 0) || ((status & 0xe000) != 0x2000)) {
	    DEBUG(1, "%s: interrupt from dead card\n", dev->name);
	    break;
	}
	
	if (status & RxComplete)
	    el3_rx(dev);
	
	if (status & TxAvailable) {
	    DEBUG(3, "    TX room bit was handled.\n");
	    /* There's room in the FIFO for a full-sized packet. */
	    outw(AckIntr | TxAvailable, ioaddr + EL3_CMD);
	    dev->tbusy = 0;
	    mark_bh(NET_BH);
	}
	
	if (status & (AdapterFailure | RxEarly | StatsFull)) {
	    /* Handle all uncommon interrupts. */
	    if (status & StatsFull)		/* Empty statistics. */
		update_stats(ioaddr, dev);
	    if (status & RxEarly) {		/* Rx early is unused. */
		el3_rx(dev);
		outw(AckIntr | RxEarly, ioaddr + EL3_CMD);
	    }
	    if (status & AdapterFailure) {
		/* Adapter failure requires Rx reset and reinit. */
		outw(RxReset, ioaddr + EL3_CMD);
#ifdef NEW_MULTICAST
		set_multicast_list(dev);
#endif
		outw(RxEnable, ioaddr + EL3_CMD); /* Re-enable the receiver. */
		outw(AckIntr | AdapterFailure, ioaddr + EL3_CMD);
	    }
	}
	
	if (++i > 10) {
	    printk(KERN_NOTICE "%s: infinite loop in interrupt, "
		   "status %4.4x.\n", dev->name, status);
	    /* Clear all interrupts */
	    outw(AckIntr | 0xFF, ioaddr + EL3_CMD);
	    break;
	}
	/* Acknowledge the IRQ. */
	outw(AckIntr | IntReq | IntLatch, ioaddr + EL3_CMD);

	if (lp->probe_port)
	    check_if_port(dev);
    }

#ifdef PCMCIA_DEBUG
    DEBUG(3, "%s: exiting interrupt, status %4.4x.\n",
	  dev->name, inw(ioaddr + EL3_STATUS));
    dev->interrupt = 0;
#endif
    return;
}

/* Check for a transceiver mismatch.  We should get carrier errors when
   there is a mismatch. */

static void check_if_port(struct device *dev)
{
    struct el3_private *lp = (struct el3_private *)dev->priv;
    int ioaddr = dev->base_addr;
    static volatile int in_check = 0;
    int carrier, tx_packets, rx_packets;
    long flags;

    /* We don't want this to get interrupted! */
    save_flags(flags);
    cli();
    if (in_check) {
	restore_flags(flags);
	return;
    }
    in_check = 1;
    /* Turn off statistics updates while reading. */
    outw(StatsDisable, ioaddr + EL3_CMD);
    /* Switch to the stats window, and read carrier errors. */
    EL3WINDOW(6);
    carrier = inb(ioaddr + 0);
    tx_packets = inb(ioaddr + 6);
    rx_packets = inb(ioaddr + 7);
    lp->stats.tx_carrier_errors += carrier;
    lp->stats.rx_packets += rx_packets;
    lp->stats.tx_packets += tx_packets;
    /* Back to window 1, and turn statistics back on. */
    EL3WINDOW(1);
    outw(StatsEnable, ioaddr + EL3_CMD);
    restore_flags(flags);
    
    if (carrier) {
	dev->if_port = 2;
	tc589_reset(dev);
    } else {
	if (rx_packets + tx_packets > 0) {
	    if (dev->if_port == 0) dev->if_port = 1;
	    printk(KERN_INFO "%s: autodetected %s\n",
		   dev->name, if_names[dev->if_port]);
	    lp->probe_port = 1;
	} else if (lp->probe_port == 1) {
	    printk(KERN_INFO "%s: network cable problem?\n",
		   dev->name);
	}
    }
    lp->probe_port--;
    in_check = 0;
}

static struct net_device_stats *el3_get_stats(struct device *dev)
{
    struct el3_private *lp = (struct el3_private *)dev->priv;
    unsigned long flags;
    dev_link_t *link;

    for (link = dev_list; link; link = link->next)
	if (link->priv == dev) break;
    if (DEV_OK(link)) {
	save_flags(flags);
	cli();
	update_stats(dev->base_addr, dev);
	restore_flags(flags);
    }
    return &lp->stats;
}

/*  Update statistics.  We change to register window 6, so this should be run
    single-threaded if the device is active. This is expected to be a rare
    operation, and it's simpler for the rest of the driver to assume that
    window 1 is always valid rather than use a special window-state variable.
 */
static void update_stats(int ioaddr, struct device *dev)
{
    struct el3_private *lp = (struct el3_private *)dev->priv;
    
    DEBUG(2, "%s: updating the statistics.\n", dev->name);
    /* Turn off statistics updates while reading. */
    outw(StatsDisable, ioaddr + EL3_CMD);
    /* Switch to the stats window, and read everything. */
    EL3WINDOW(6);
    lp->stats.tx_carrier_errors 	+= inb(ioaddr + 0);
    lp->stats.tx_heartbeat_errors	+= inb(ioaddr + 1);
    /* Multiple collisions. */	   	inb(ioaddr + 2);
    lp->stats.collisions		+= inb(ioaddr + 3);
    lp->stats.tx_window_errors		+= inb(ioaddr + 4);
    lp->stats.rx_fifo_errors		+= inb(ioaddr + 5);
    lp->stats.tx_packets		+= inb(ioaddr + 6);
    /* Rx packets   */			inb(ioaddr + 7);
    /* Tx deferrals */			inb(ioaddr + 8);
    inw(ioaddr + 10);	/* Total Rx and Tx octets. */
    inw(ioaddr + 12);
    
    /* Back to window 1, and turn statistics back on. */
    EL3WINDOW(1);
    outw(StatsEnable, ioaddr + EL3_CMD);
}

static int el3_rx(struct device *dev)
{
    struct el3_private *lp = (struct el3_private *)dev->priv;
    int ioaddr = dev->base_addr;
    int boguscnt = 10;
    short rx_status;
    
    DEBUG(3, "%s: in rx_packet(), status %4.4x, rx_status %4.4x.\n",
	  dev->name, inw(ioaddr+EL3_STATUS), inw(ioaddr+RX_STATUS));
    while ((rx_status = inw(ioaddr + RX_STATUS)) > 0) {
	if (rx_status & 0x4000) { /* Error, update stats. */
	    short error = rx_status & 0x3800;
	    lp->stats.rx_errors++;
	    switch (error) {
	    case 0x0000:		lp->stats.rx_over_errors++; break;
	    case 0x0800:		lp->stats.rx_length_errors++; break;
	    case 0x1000:		lp->stats.rx_frame_errors++; break;
	    case 0x1800:		lp->stats.rx_length_errors++; break;
	    case 0x2000:		lp->stats.rx_frame_errors++; break;
	    case 0x2800:		lp->stats.rx_crc_errors++; break;
	    }
	} else {
	    short pkt_len = rx_status & 0x7ff;
	    struct sk_buff *skb;
	    
	    skb = ALLOC_SKB(pkt_len+3);
	    
	    DEBUG(3, "    Receiving packet size %d status %4.4x.\n",
		  pkt_len, rx_status);
	    if (skb != NULL) {
		skb->dev = dev;
		
#define BLOCK_INPUT(buf, len) insl_ns(ioaddr+RX_FIFO, buf, (len+3)>>2)
		GET_PACKET(dev, skb, pkt_len);
		
		netif_rx(skb);
		outw(RxDiscard, ioaddr + EL3_CMD); /* Pop top Rx packet. */
		lp->stats.rx_packets++;
#if (LINUX_VERSION_CODE >= VERSION(2,1,25))
		lp->stats.rx_bytes += skb->len;
#endif
		continue;
	    } else
		DEBUG(1, "%s: couldn't allocate a sk_buff of"
		      " size %d.\n", dev->name, pkt_len);
	}
	lp->stats.rx_dropped++;
	outw(RxDiscard, ioaddr + EL3_CMD); /* Rx discard */
	while (--boguscnt > 0 && inw(ioaddr + EL3_STATUS) & 0x1000)
	    DEBUG(0, "    Waiting for 3c589 to discard packet,"
		  " status %x.\n", inw(ioaddr + EL3_STATUS));
	if (--boguscnt < 0)
	    break;
    }
    
    return 0;
}

/* Set or clear the multicast filter for this adaptor.
   num_addrs == -1	Promiscuous mode, receive all packets
   num_addrs == 0	Normal mode, clear multicast list
   num_addrs > 0	Multicast mode, receive normal and MC packets, and do
			best-effort filtering.
 */
#ifdef NEW_MULTICAST
static void set_multicast_list(struct device *dev)
{
    short ioaddr = dev->base_addr;
    dev_link_t *link;
    for (link = dev_list; link; link = link->next)
	if (link->priv == dev) break;
    if (!(DEV_OK(link))) return;
#ifdef PCMCIA_DEBUG
    if (pc_debug > 2) {
	static int old = 0;
	if (old != dev->mc_count) {
	    old = dev->mc_count;
	    DEBUG(0, "%s: setting Rx mode to %d addresses.\n",
		  dev->name, old);
	}
    }
#endif
    if (dev->flags & IFF_PROMISC)
	outw(SetRxFilter | RxStation | RxMulticast | RxBroadcast | RxProm,
	     ioaddr + EL3_CMD);
    else if (dev->mc_count || (dev->flags & IFF_ALLMULTI))
	outw(SetRxFilter|RxStation|RxMulticast|RxBroadcast, ioaddr + EL3_CMD);
    else
	outw(SetRxFilter | RxStation | RxBroadcast, ioaddr + EL3_CMD);
}
#else
static void
set_multicast_list(struct device *dev, int num_addrs, void *addrs)
{
    short ioaddr = dev->base_addr;
#ifdef PCMCIA_DEBUG
    if (pc_debug > 2) {
	static int old = 0;
	if (old != num_addrs) {
	    old = num_addrs;
	    DEBUG(0, "%s: setting Rx mode to %d addresses.\n",
		  dev->name, num_addrs);
	}
    }
#endif
    if ((num_addrs > 0) || (num_addrs == -2))
	outw(SetRxFilter|RxStation|RxMulticast|RxBroadcast, ioaddr + EL3_CMD);
    else if (num_addrs < 0)
	outw(SetRxFilter | RxStation | RxMulticast | RxBroadcast | RxProm,
	     ioaddr + EL3_CMD);
    else
	outw(SetRxFilter | RxStation | RxBroadcast, ioaddr + EL3_CMD);
}
#endif

static int el3_close(struct device *dev)
{
    int ioaddr = dev->base_addr;
    dev_link_t *link;

    for (link = dev_list; link; link = link->next)
	if (link->priv == dev) break;
    if (link == NULL)
	return -ENODEV;
    
    DEBUG(2, "%s: shutting down ethercard.\n", dev->name);

    if (DEV_OK(link)) {
	/* Turn off statistics ASAP.  We update lp->stats below. */
	outw(StatsDisable, ioaddr + EL3_CMD);
	
	/* Disable the receiver and transmitter. */
	outw(RxDisable, ioaddr + EL3_CMD);
	outw(TxDisable, ioaddr + EL3_CMD);
	
	if (dev->if_port == 2)
	    /* Turn off thinnet power.  Green! */
	    outw(StopCoax, ioaddr + EL3_CMD);
	else if (dev->if_port == 1) {
	    /* Disable link beat and jabber */
	    EL3WINDOW(4);
	    outw(inw(ioaddr + WN4_MEDIA) & ~MEDIA_TP, ioaddr + WN4_MEDIA);
	}
	
	/* Switching back to window 0 disables the IRQ. */
	EL3WINDOW(0);
	/* But we explicitly zero the IRQ line select anyway. */
	outw(0x0f00, ioaddr + WN0_IRQ);
        
	/* Check if the card still exists */
	if ((inw(ioaddr+EL3_STATUS) & 0xe000) == 0x2000)
	    update_stats(ioaddr, dev);
    }

    link->open--;
    dev->start = 0;
    if (link->state & DEV_STALE_CONFIG) {
	link->release.expires = RUN_AT(HZ/20);
	link->state |= DEV_RELEASE_PENDING;
	add_timer(&link->release);
    }
    
    MOD_DEC_USE_COUNT;
    
    return 0;
}

/*====================================================================*/

int init_module(void)
{
    servinfo_t serv;
    DEBUG(0, "%s\n", version);
    CardServices(GetCardServicesInfo, &serv);
    if (serv.Revision != CS_RELEASE_CODE) {
	printk(KERN_NOTICE "3c589_cs: Card Services release "
	       "does not match!\n");
	return -1;
    }
    register_pcmcia_driver(&dev_info, &tc589_attach, &tc589_detach);
    return 0;
}

void cleanup_module(void)
{
    DEBUG(0, "3c589_cs: unloading\n");
    unregister_pcmcia_driver(&dev_info);
    while (dev_list != NULL)
	tc589_detach(dev_list);
}
