/* 3c574.c: A PCMCIA ethernet driver for the 3com 3c574 "RoadRunner".

	Written 1993-1998 by
	Donald Becker, becker@cesdis.gsfc.nasa.gov, (driver core) and
	David Hinds, dhinds@allegro.stanford.edu (derived from his PC card code).

	This software may be used and distributed according to the terms of
	the GNU Public License, incorporated herein by reference.

	This driver derives from Donald Becker's 3c509 core, which has the
	following copyright:
	Copyright 1993 United States Government as represented by the
	Director, National Security Agency.

*/

/* Driver author info must always be in the binary.  Version too.. */
static const char *tc574_version =
"3c574_cs.c v1.08 9/24/98 Donald Becker/David Hinds, becker@cesdis.gsfc.nasa.gov.\n";

/*
				Theory of Operation

I. Board Compatibility

This device driver is designed for the 3Com 3c574 PC card Fast Ethernet
Adapter.

II. Board-specific settings

None -- PC cards are autoconfigured.

III. Driver operation

The 3c574 uses a Boomerang-style interface, without the bus-master capability.
See the Boomerang driver and documentation for most details.

IV. Notes and chip documentation.

Two added registers are used to enhance PIO performance, RunnerRdCtrl and
RunnerWrCtrl.  These are 11 bit down-counters that are preloaded with the
count of word (16 bits) reads or writes the driver is about to do to the Rx
or Tx FIFO.  The chip is then able to hide the internal-PCI-bus to PC-card
translation latency by buffering the I/O operations with an 8 word FIFO.
Note: No other chip accesses are permitted when this buffer is used.

A second enhancement is that both attribute and common memory space
0x0800-0x0fff can translated to the PIO FIFO.  Thus memory operations (faster
with *some* PCcard bridges) may be used instead of I/O operations.
This is enabled by setting the 0x10 bit in the PCMCIA LAN COR.

Some slow PC card bridges work better if they never see a WAIT signal.
This is configured by setting the 0x20 bit in the PCMCIA LAN COR.
Only do this after testing that it is reliable and improves performance.

The upper five bits of RunnerRdCtrl are used to window into PCcard
configuration space registers.  Window 0 is the regular Boomerang/Odie
register set, 1-5 are various PC card control registers, and 16-31 are
the (reversed!) CIS table.

A final note: writing the InteralConfig register in window 3 with an
invalid ramWidth is Very Bad.

V. References

http://cesdis.gsfc.nasa.gov/linux/misc/NWay.html
http://www.national.com/pf/DP/DP83840.html

Thanks to Terry Murphy of 3Com for providing development information for
earlier 3Com products.

No printed Roadrunner documentation is available.  This driver is the only
publically-available documentation about its operation (1/98).

*/

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

/* A few values that may be tweaked. */
#if LINUX_VERSION_CODE < 0x20115
MODULE_PARM(if_port, "i");
MODULE_PARM(irq_mask, "i");
MODULE_PARM(irq_list, "1-4i");
MODULE_PARM(max_interrupt_work, "i");
MODULE_PARM(use_fifo_buffer, "i");
MODULE_PARM(use_memory_ops, "i");
MODULE_PARM(no_wait, "i");
#endif

/* Now-standard PC card module parameters. */
static int if_port = 0;					/* Unused */
static u_int irq_mask = 0xdeb8;			/* IRQ3,4,5,7,9,10,11,12,14,15 */
static int irq_list[4] = { -1 };

/* Time in jiffies before concluding the transmitter is hung. */
#define TX_TIMEOUT  ((800*HZ)/1000)

/* Maximum events (Rx packets, etc.) to handle at each interrupt. */
static int max_interrupt_work = 20;

/* Performance features: best left disabled. */
/* Set to buffer all Tx/RxFIFO accesses. */
static int use_fifo_buffer = 0;
/* Set iff memory ops are faster than I/O ops. */
static int use_memory_ops = 0;
/* Set iff disabling the WAIT signal is reliable and faster. */
static int no_wait = 0;

/* To minimize the size of the driver source and make the driver more
   readable not all constants are symbolically defined.
   You'll need the manual if you want to understand driver details anyway. */
/* Offsets from base I/O address. */
#define EL3_DATA	0x00
#define EL3_CMD		0x0e
#define EL3_STATUS	0x0e

#define EL3WINDOW(win_num) outw(SelectWindow + (win_num), ioaddr + EL3_CMD)

/* The top five bits written to EL3_CMD are a command, the lower
   11 bits are the parameter, if applicable. */
enum el3_cmds {
	TotalReset = 0<<11, SelectWindow = 1<<11, StartCoax = 2<<11,
	RxDisable = 3<<11, RxEnable = 4<<11, RxReset = 5<<11, RxDiscard = 8<<11,
	TxEnable = 9<<11, TxDisable = 10<<11, TxReset = 11<<11,
	FakeIntr = 12<<11, AckIntr = 13<<11, SetIntrEnb = 14<<11,
	SetStatusEnb = 15<<11, SetRxFilter = 16<<11, SetRxThreshold = 17<<11,
	SetTxThreshold = 18<<11, SetTxStart = 19<<11, StatsEnable = 21<<11,
	StatsDisable = 22<<11, StopCoax = 23<<11,
};

enum elxl_status {
	IntLatch = 0x0001, AdapterFailure = 0x0002, TxComplete = 0x0004,
	TxAvailable = 0x0008, RxComplete = 0x0010, RxEarly = 0x0020,
	IntReq = 0x0040, StatsFull = 0x0080, CmdBusy = 0x1000 };

/* The SetRxFilter command accepts the following classes: */
enum RxFilter {
	RxStation = 1, RxMulticast = 2, RxBroadcast = 4, RxProm = 8
};

enum Window0 {
	Wn0EepromCmd = 10, Wn0EepromData = 12, /* EEPROM command/address, data. */
	IntrStatus=0x0E,		/* Valid in all windows. */
};
/* These assumes the larger EEPROM. */
enum Win0_EEPROM_cmds {
	EEPROM_Read = 0x200, EEPROM_WRITE = 0x100, EEPROM_ERASE = 0x300,
	EEPROM_EWENB = 0x30,		/* Enable erasing/writing for 10 msec. */
	EEPROM_EWDIS = 0x00,		/* Disable EWENB before 10 msec timeout. */
};

/* Register window 1 offsets, the window used in normal operation.
   On the "Odie" this window is always mapped at offsets 0x10-0x1f.
   Except for TxFree, which is overlapped by RunnerWrCtrl. */
enum Window1 {
	TX_FIFO = 0x10,  RX_FIFO = 0x10,  RxErrors = 0x14,
	RxStatus = 0x18,  Timer=0x1A, TxStatus = 0x1B,
	TxFree = 0x0C, /* Remaining free bytes in Tx buffer. */
	RunnerRdCtrl = 0x16, RunnerWrCtrl = 0x1c,
};

enum Window3 {			/* Window 3: MAC/config bits. */
	Wn3_Config=0, Wn3_MAC_Ctrl=6, Wn3_Options=8,
};
union wn3_config {
	int i;
	struct w3_config_fields {
		unsigned int ram_size:3, ram_width:1, ram_speed:2, rom_size:2;
		int pad8:8;
		unsigned int ram_split:2, pad18:2, xcvr:3, pad21:1, autoselect:1;
		int pad24:7;
	} u;
};

enum Window4 {		/* Window 4: Xcvr/media bits. */
	Wn4_FIFODiag = 4, Wn4_NetDiag = 6, Wn4_PhysicalMgmt=8, Wn4_Media = 10,
};


#define MEDIA_TP	0x00C0	/* Enable link beat and jabber for 10baseT. */

struct el3_private {
	dev_node_t node;
	struct net_device_stats stats;
	u16 available_media;				/* From Wn3_Options. */
	u16 capabilities, softinfo1, softinfo2;		/* Various, from EEPROM. */
	u16 advertising;					/* NWay media advertisement */
	unsigned char phys[2];				/* MII device addresses. */
	unsigned int
	  is_full_duplex:1,
	  force_full_duplex:1,
	  autoselect:1, default_media:3;	/* Read from the EEPROM/Wn3_Config. */
};

static char *if_names[] = { "Auto", "10baseT", "10base2", "AUI", "MII" };

/* Set iff a MII transceiver on any interface requires mdio preamble.
   This only set with the original DP83840 on older 3c905 boards, so the extra
   code size of a per-interface flag is not worthwhile. */
static char mii_preamble_required = 0;

#ifdef PCMCIA_DEBUG
static int pc_debug = PCMCIA_DEBUG;
MODULE_PARM(pc_debug, "i");
#define DEBUG(n, args...) if (pc_debug>(n)) printk(KERN_DEBUG args)
static char *version =
"3c574_cs.c 1.000 1998/1/8 Donald Becker, becker@cesdis.gsfc.nasa.gov.\n";
#else
#define DEBUG(n, args...)
#endif

/* Index of functions. */

static void tc574_config(dev_link_t *link);
static void tc574_release(u_long arg);
static int tc574_event(event_t event, int priority,
					   event_callback_args_t *args);

static void mdio_sync(int ioaddr, int bits);
static int mdio_read(int ioaddr, int phy_id, int location);
static void mdio_write(int ioaddr, int phy_id, int location, int value);
static ushort read_eeprom(int ioaddr, int index);

static void tc574_reset(struct device *dev);
static int el3_config(struct device *dev, struct ifmap *map);
static int el3_open(struct device *dev);
static void tc574_tx_timeout(struct device *dev);
static int el3_start_xmit(struct sk_buff *skb, struct device *dev);
static void el3_interrupt IRQ(int irq, void *dev_id, struct pt_regs *regs);
static void update_stats(int addr, struct device *dev);
static struct net_device_stats *el3_get_stats(struct device *dev);
static int el3_rx(struct device *dev, int worklimit);
static int el3_close(struct device *dev);
#ifdef HAVE_PRIVATE_IOCTL
static int private_ioctl(struct device *dev, struct ifreq *rq, int cmd);
#endif
static void set_rx_mode(struct device *dev);

static dev_info_t dev_info = "3c574_cs";

static dev_link_t *tc574_attach(void);
static void tc574_detach(dev_link_t *);

static dev_link_t *dev_list = NULL;

static void cs_error(client_handle_t handle, int func, int ret)
{
#if CS_RELEASE_CODE < 0x2911
    CardServices(ReportError, dev_info, (void *)func, (void *)ret);
#else
	error_info_t err = { func, ret };
	CardServices(ReportError, handle, &err);
#endif
}

/*
   We never need to do anything when a tc574 device is "initialized"
   by the net software, because we only register already-found cards.
*/

static int tc574_init(struct device *dev)
{
	return 0;
}

/*
	tc574_attach() creates an "instance" of the driver, allocating
	local data structures for one device.  The device is registered
	with Card Services.
*/

static dev_link_t *tc574_attach(void)
{
	client_reg_t client_reg;
	dev_link_t *link;
	struct device *dev;
	int i, ret;

	DEBUG(0, "3c574_attach()\n");

	/* Create the PC card device object. */
	link = kmalloc(sizeof(struct dev_link_t), GFP_KERNEL);
	memset(link, 0, sizeof(struct dev_link_t));
	link->release.function = &tc574_release;
	link->release.data = (u_long)link;
	link->io.NumPorts1 = 32;
	link->io.Attributes1 = IO_DATA_PATH_WIDTH_16;
	link->io.IOAddrLines = 5;
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

	/* Create the network device object. */
	dev = kmalloc(sizeof(struct device), GFP_KERNEL);
	memset(dev, 0, sizeof(struct device));

	/* Make up a Odie-specific data structure. */
	dev->priv = kmalloc(sizeof(struct el3_private), GFP_KERNEL);
	memset(dev->priv, 0, sizeof(struct el3_private));

	/* The EL3-specific entries in the device structure. */
	dev->hard_start_xmit = &el3_start_xmit;
	dev->set_config = &el3_config;
	dev->get_stats = &el3_get_stats;
#ifdef HAVE_PRIVATE_IOCTL
	dev->do_ioctl = &private_ioctl;
#endif
	dev->set_multicast_list = &set_rx_mode;
	ether_setup(dev);
	dev->name = ((struct el3_private *)dev->priv)->node.dev_name;
	dev->init = &tc574_init;
	dev->open = &el3_open;
	dev->stop = &el3_close;
	dev->tbusy = 1;

	link->priv = dev;
#if CS_RELEASE_CODE > 0x2911
	link->irq.Instance = dev;
#endif

	/* Register with Card Services */
	link->next = dev_list;
	dev_list = link;
	client_reg.dev_info = &dev_info;
	client_reg.Attributes = INFO_IO_CLIENT | INFO_CARD_SHARE;
	client_reg.EventMask =
		CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
			CS_EVENT_RESET_PHYSICAL | CS_EVENT_CARD_RESET |
				CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
	client_reg.event_handler = &tc574_event;
	client_reg.Version = 0x0210;
	client_reg.event_callback_args.client_data = link;
	ret = CardServices(RegisterClient, &link->handle, &client_reg);
	if (ret != 0) {
		cs_error(link->handle, RegisterClient, ret);
		tc574_detach(link);
		return NULL;
	}

	return link;
} /* tc574_attach */

/*

	This deletes a driver "instance".  The device is de-registered
	with Card Services.  If it has been released, all local data
	structures are freed.  Otherwise, the structures will be freed
	when the device is released.

*/

static void tc574_detach(dev_link_t *link)
{
	dev_link_t **linkp;
	long flags;

	DEBUG(0, "3c574_detach(0x%p)\n", link);

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
		tc574_release((u_long)link);
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

} /* tc574_detach */

/*

	tc574_config() is scheduled to run after a CARD_INSERTION event
	is received, to configure the PCMCIA socket, and to make the
	ethernet device available to the system.

*/

#define CS_CHECK(fn, args...) \
while ((last_ret=CardServices(last_fn=(fn), args))!=0) goto cs_failed

static void tc574_config(dev_link_t *link)
{
	client_handle_t handle;
	struct device *dev;
	struct el3_private *lp;
	tuple_t tuple;
	cisparse_t parse;
	u_short buf[32];
	int last_fn, last_ret, i, j;
	int ioaddr;
	u16 *phys_addr;
	char *cardname;

	handle = link->handle;
	dev = link->priv;
	phys_addr = (u16 *)dev->dev_addr;

	DEBUG(0, "3c574_config(0x%p)\n", link);

	tuple.Attributes = 0;
	tuple.DesiredTuple = CISTPL_CONFIG;
	CS_CHECK(GetFirstTuple, handle, &tuple);
	tuple.TupleData = (cisdata_t *)buf;
	tuple.TupleDataMax = 64;
	tuple.TupleOffset = 0;
	CS_CHECK(GetTupleData, handle, &tuple);
	CS_CHECK(ParseTuple, handle, &tuple, &parse);
	link->conf.ConfigBase = parse.config.base;
	link->conf.Present = parse.config.rmask[0];

	/* Configure card */
	link->state |= DEV_CONFIG;

	for (i = j = 0; j < 0x400; j += 0x20) {
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

	dev->mem_start = 0;
	if (use_memory_ops) {
		win_req_t req;
		memreq_t mem;
		req.Attributes = WIN_DATA_WIDTH_16|WIN_MEMORY_TYPE_CM|WIN_ENABLE;
		req.Base = 0;
		req.Size = 0x1000;
		req.AccessSpeed = 0;
		link->win = (window_handle_t)link->handle;
		i = CardServices(RequestWindow, &link->win, &req);
		if (i == CS_SUCCESS) {
			mem.Page = mem.CardOffset = 0;
			CardServices(MapMemPage, link->win, &mem);
			dev->mem_start = (long)(ioremap(req.Base, 0x1000)) + 0x800;
		} else
			cs_error(link->handle, RequestWindow, i);
	}

	dev->irq = link->irq.AssignedIRQ;
	dev->base_addr = link->io.BasePort1;

	dev->tbusy = 0;
	if (register_netdev(dev) != 0) {
		printk(KERN_NOTICE "3c574_cs: register_netdev() failed\n");
		goto failed;
	}

	link->state &= ~DEV_CONFIG_PENDING;

	ioaddr = dev->base_addr;
	lp = (struct el3_private *)dev->priv;
	link->dev = &lp->node;

	/* The 3c574 normally uses an EEPROM for configuration info, including
	   the hardware address.  The future products may include a modem chip
	   and put the address in the CIS. */
	tuple.DesiredTuple = 0x88;
	if (CardServices(GetFirstTuple, handle, &tuple) == CS_SUCCESS) {
		CardServices(GetTupleData, handle, &tuple);
		for (i = 0; i < 3; i++)
			phys_addr[i] = htons(buf[i]);
	} else {
		EL3WINDOW(0);
		for (i = 0; i < 3; i++)
			phys_addr[i] = htons(read_eeprom(ioaddr, i + 10));
		if (phys_addr[0] == 0x6060) {
			printk(KERN_NOTICE "3c574_cs: IO port conflict at 0x%03lx"
				   "-0x%03lx\n", dev->base_addr, dev->base_addr+15);
			goto failed;
		}
	}
	tuple.DesiredTuple = CISTPL_VERS_1;
	if (CardServices(GetFirstTuple, handle, &tuple) == CS_SUCCESS &&
		CardServices(GetTupleData, handle, &tuple) == CS_SUCCESS &&
		CardServices(ParseTuple, handle, &tuple, &parse) == CS_SUCCESS) {
		cardname = parse.version_1.str + parse.version_1.ofs[1];
	} else
		cardname = "3Com 3c574";

	/* Extract other info from the EEPROM. */
	lp->softinfo1 = read_eeprom(ioaddr, 13);
	lp->softinfo2 = read_eeprom(ioaddr, 15);
	lp->capabilities = read_eeprom(ioaddr, 16);

	if (lp->softinfo1 & 0x8000)
		lp->force_full_duplex = 1;

	printk(KERN_INFO "%s: %s at I/O %#3lx, IRQ %d, "
		   "MAC Address ", dev->name, cardname, dev->base_addr, dev->irq);

	for (i = 0; i < 6; i++)
		printk("%02X%s", dev->dev_addr[i], ((i<5) ? ":" : ".\n"));

	/* The if_port symbol can be set when the module is loaded,
	   but we always ignore it for now. */
	dev->if_port = 0;
	if (if_port == 1  ||  if_port == 4) {
		printk(KERN_INFO "%s: Media override to %s requested.\n",
			   dev->name, if_names[if_port]);
		dev->if_port = if_port;
	} else if (if_port != 0)
		printk(KERN_NOTICE "%s: Invalid if_port %d requested, ignored.\n",
			   dev->name, if_port);

	if (dev->mem_start)
		printk(KERN_INFO"%s:  Acceleration window at memory base %#lx.\n",
			   dev->name, dev->mem_start);
	else
		printk(KERN_INFO"%s:  No acceleration memory window.\n", dev->name);

	{
		char *ram_split[] = {"5:3", "3:1", "1:1", "3:5"};
		union wn3_config config;
		EL3WINDOW(3);
		lp->available_media = inw(ioaddr + Wn3_Options);
		/* Roadrunner only: Turn on the MII transceiver. */
		outw((lp->available_media & 0xff) | 0x8000, ioaddr + Wn3_Options);

		config.i = inl(ioaddr + Wn3_Config);
		DEBUG(1, "  Internal config register is %4.4x, transceivers %#x.\n",
			  config.i, lp->available_media);
		printk(KERN_INFO"  %dK FIFO split %s Rx:Tx, %sMII interface.\n",
			   8 << config.u.ram_size, ram_split[config.u.ram_split],
			   config.u.autoselect ? "autoselect " : "");
		lp->default_media = config.u.xcvr;
		lp->autoselect = config.u.autoselect;
	}

	{
		int phy, phy_idx = 0;
		EL3WINDOW(4);
		for (phy = 1; phy <= 32 && phy_idx < sizeof(lp->phys); phy++) {
			int mii_status;
			mdio_sync(ioaddr, 32);
			mii_status = mdio_read(ioaddr, phy & 0x1f, 1);
			if (mii_status != 0xffff) {
				lp->phys[phy_idx++] = phy & 0x1f;
				printk(KERN_INFO "  MII transceiver at index %d, status %x.\n",
					   phy, mii_status);
				if ((mii_status & 0x0040) == 0)
					mii_preamble_required = 1;
			}
		}
		if (phy_idx == 0) {
			printk(KERN_NOTICE"  ***WARNING*** No MII transceivers found!\n");
			lp->phys[0] = 24;
			lp->advertising = 0x01e1;	/* 10,100base-HD,FD, no T4. */
		} else {
			lp->advertising = mdio_read(ioaddr, lp->phys[0], 4);
			if (lp->force_full_duplex) {
				/* Only advertise the FD media types. */
				lp->advertising &= 0x015F;
				mdio_write(ioaddr, lp->phys[0], 4, lp->advertising);
			}
		}
	}

	return;

cs_failed:
	cs_error(link->handle, last_fn, last_ret);
failed:
	tc574_release((u_long)link);
	return;

} /* tc574_config */

/*
	After a card is removed, tc574_release() will unregister the net
	device, and release the PCMCIA configuration.  If the device is
	still open, this will be postponed until it is closed.

*/

static void tc574_release(u_long arg)
{
	dev_link_t *link = (dev_link_t *)arg;
	struct device *dev = link->priv;

	DEBUG(0, "3c574_release(0x%p)\n", link);

	if (link->open) {
		DEBUG(1, "3c574_cs: release postponed, '%s' still open\n",
			  link->dev->dev_name);
		link->state |= DEV_STALE_CONFIG;
		return;
	}

	CardServices(ReleaseConfiguration, link->handle);
	CardServices(ReleaseIO, link->handle, &link->io);
	CardServices(ReleaseIRQ, link->handle, &link->irq);
#if CS_RELEASE_CODE <= 0x2911
	if (dev->irq != 0)
		irq2dev_map[dev->irq] = NULL;
#endif
	if (link->win) {
		iounmap((void *)(dev->mem_start - 0x800));
		CardServices(ReleaseWindow, link->win);
	}
	if (link->dev)
		unregister_netdev(dev);
	link->dev = NULL;

	link->state &= ~DEV_CONFIG;
	if (link->state & DEV_STALE_LINK)
		tc574_detach(link);

} /* tc574_release */

/*

	The card status event handler.  Mostly, this schedules other
	stuff to run after an event is received.  A CARD_REMOVAL event
	also sets some flags to discourage the net drivers from trying
	to talk to the card any more.
*/

static int tc574_event(event_t event, int priority,
					   event_callback_args_t *args)
{
	dev_link_t *link = args->client_data;
	struct device *dev = link->priv;

	DEBUG(1, "3c574_event(0x%06x)\n", event);

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
		tc574_config(link);
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
				tc574_reset(dev);
				dev->tbusy = 0; dev->start = 1;
			}
		}
		break;
	}
	return 0;
} /* tc574_event */

/* Read a word from the EEPROM using the regular EEPROM access register.
   Assume that we are in register window zero.
 */
static ushort read_eeprom(int ioaddr, int index)
{
	int timer;
	outw(EEPROM_Read + index, ioaddr + Wn0EepromCmd);
	/* Pause for at least 162 usec for the read to take place. */
	for (timer = 1620; timer >= 0; timer--) {
		if ((inw(ioaddr + Wn0EepromCmd) & 0x8000) == 0)
			break;
	}
	return inw(ioaddr + Wn0EepromData);
}


/* MII transceiver control section.
   Read and write the MII registers using software-generated serial
   MDIO protocol.  See the MII specifications or DP83840A data sheet
   for details.
   The maxium data clock rate is 2.5 Mhz.  The timing is easily met by the
   slow PC card interface. */

#define MDIO_SHIFT_CLK	0x01
#define MDIO_DIR_WRITE	0x04
#define MDIO_DATA_WRITE0 (0x00 | MDIO_DIR_WRITE)
#define MDIO_DATA_WRITE1 (0x02 | MDIO_DIR_WRITE)
#define MDIO_DATA_READ	0x02
#define MDIO_ENB_IN		0x00

/* Generate the preamble required for initial synchronization and
   a few older transceivers. */
static void mdio_sync(int ioaddr, int bits)
{
	int mdio_addr = ioaddr + Wn4_PhysicalMgmt;

	/* Establish sync by sending at least 32 logic ones. */
	while (-- bits >= 0) {
		outw(MDIO_DATA_WRITE1, mdio_addr);
		outw(MDIO_DATA_WRITE1 | MDIO_SHIFT_CLK, mdio_addr);
	}
}

static int mdio_read(int ioaddr, int phy_id, int location)
{
	int i;
	int read_cmd = (0xf6 << 10) | (phy_id << 5) | location;
	unsigned int retval = 0;
	int mdio_addr = ioaddr + Wn4_PhysicalMgmt;

	if (mii_preamble_required)
		mdio_sync(ioaddr, 32);

	/* Shift the read command bits out. */
	for (i = 14; i >= 0; i--) {
		int dataval = (read_cmd&(1<<i)) ? MDIO_DATA_WRITE1 : MDIO_DATA_WRITE0;
		outw(dataval, mdio_addr);
		outw(dataval | MDIO_SHIFT_CLK, mdio_addr);
	}
	/* Read the two transition, 16 data, and wire-idle bits. */
	for (i = 19; i > 0; i--) {
		outw(MDIO_ENB_IN, mdio_addr);
		retval = (retval << 1) | ((inw(mdio_addr) & MDIO_DATA_READ) ? 1 : 0);
		outw(MDIO_ENB_IN | MDIO_SHIFT_CLK, mdio_addr);
	}
	return (retval>>1) & 0xffff;
}

static void mdio_write(int ioaddr, int phy_id, int location, int value)
{
	int write_cmd = 0x50020000 | (phy_id << 23) | (location << 18) | value;
	int mdio_addr = ioaddr + Wn4_PhysicalMgmt;
	int i;

	if (mii_preamble_required)
		mdio_sync(ioaddr, 32);

	/* Shift the command bits out. */
	for (i = 31; i >= 0; i--) {
		int dataval = (write_cmd&(1<<i)) ? MDIO_DATA_WRITE1 : MDIO_DATA_WRITE0;
		outw(dataval, mdio_addr);
		outw(dataval | MDIO_SHIFT_CLK, mdio_addr);
	}
	/* Leave the interface idle. */
	for (i = 1; i >= 0; i--) {
		outw(MDIO_ENB_IN, mdio_addr);
		outw(MDIO_ENB_IN | MDIO_SHIFT_CLK, mdio_addr);
	}

	return;
}


/* Reset and restore all of the 3c574 registers. */
static void tc574_reset(struct device *dev)
{
	struct el3_private *lp = (struct el3_private *)dev->priv;
	int ioaddr = dev->base_addr;
	int i = 1500;

	/* Typically 0 or 1 tick. */
	outw(TotalReset|0x10, ioaddr + EL3_CMD);
	while (inw(ioaddr + EL3_STATUS) & CmdBusy)
		if (--i < 0)  break;
	if (i < 0)
		printk(KERN_ERR "%s: Card reset did not complete, status 0x%4.4x.\n",
			   dev->name, inw(ioaddr + EL3_STATUS));

	/* Set the PIO ctrl bits in the PC card LAN COR using Runner window 1. */
	if (dev->mem_start || no_wait) {
		u8 lan_cor;
		outw(1<<11, ioaddr + RunnerRdCtrl);
		lan_cor = inw(ioaddr) & ~0x30;
		if (dev->mem_start)		/* Iff use_memory_ops worked! */
			lan_cor |= 0x10;
		if (no_wait)
			lan_cor |= 0x20;
		outw(lan_cor, ioaddr);
	}
	/* Clear any transactions in progress. */
	outw(0, ioaddr + RunnerWrCtrl);
	outw(0, ioaddr + RunnerRdCtrl);

	/* Set the station address and mask. */
	EL3WINDOW(2);
	for (i = 0; i < 6; i++)
		outb(dev->dev_addr[i], ioaddr + i);
	for (; i < 12; i+=2)
		outw(0, ioaddr + i);

	if (dev->if_port == 2)
		/* Start the thinnet transceiver. Note: Tx is not reliable for 50ms. */
		outw(StartCoax, ioaddr + EL3_CMD);
	else if (dev->if_port < 2) {
		/* 10baseT interface, enabled link beat and jabber check. */
		EL3WINDOW(4);
		outw(inw(ioaddr + Wn4_Media) | MEDIA_TP, ioaddr + Wn4_Media);
	}

	EL3WINDOW(3);
	/* Set the full-duplex bit. */
	outb((lp->force_full_duplex || lp->is_full_duplex ? 0x20 : 0) |
		 (dev->mtu > 1500 ? 0x40 : 0), ioaddr + Wn3_MAC_Ctrl);
	/* Roadrunner only: Turn on the MII transceiver. */
	outw((lp->available_media & 0xff) | 0x8000, ioaddr + Wn3_Options);

	/* Switch to the stats window, and clear all stats by reading. */
	outw(StatsDisable, ioaddr + EL3_CMD);
	EL3WINDOW(6);
	for (i = 0; i < 10; i++)
		inb(ioaddr + i);
	inw(ioaddr + 10);
	inw(ioaddr + 12);

	EL3WINDOW(4);
	/* New: On the Vortex/Odie we must also clear the BadSSD counter.. */
	inb(ioaddr + 12);
	/* .. enable any extra statistics bits.. */
	outw(0x0040, ioaddr + Wn4_NetDiag);
	/* .. re-sync MII and re-fill what NWay is advertising. */
	mdio_sync(ioaddr, 32);
	mdio_write(ioaddr, lp->phys[0], 4, lp->advertising);

	/* Switch to register set 1 for normal use, just for TxFree. */
	EL3WINDOW(1);

	set_rx_mode(dev);
	outw(StatsEnable, ioaddr + EL3_CMD); /* Turn on statistics. */
	outw(RxEnable, ioaddr + EL3_CMD); /* Enable the receiver. */
	outw(TxEnable, ioaddr + EL3_CMD); /* Enable transmitter. */
	/* Allow status bits to be seen. */
	outw(SetStatusEnb | 0xff, ioaddr + EL3_CMD);
	/* Ack all pending events, and set active indicator mask. */
	outw(AckIntr | IntLatch | TxAvailable | RxEarly | IntReq,
		 ioaddr + EL3_CMD);
	outw(SetIntrEnb | IntLatch | TxAvailable | RxComplete | StatsFull
		 | AdapterFailure /* Optional | TxComplete */, ioaddr + EL3_CMD);
}

static int el3_config(struct device *dev, struct ifmap *map)
{
	if ((map->port != (u_char)(-1)) && (map->port != dev->if_port)) {
		if (map->port <= 3) {
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

	dev->interrupt = 0; dev->tbusy = 0; dev->start = 1;
	{
		int mii_reg1, mii_reg5;
		EL3WINDOW(4);
		/* Read BMSR (reg1) only to clear old status. */
		mii_reg1 = mdio_read(ioaddr, lp->phys[0], 1);
		mii_reg5 = mdio_read(ioaddr, lp->phys[0], 5);
		if (mii_reg5 == 0xffff  ||  mii_reg5 == 0x0000)
			;					/* No MII device or no link partner report */
		else if ((mii_reg5 & 0x0100) != 0	/* 100baseTx-FD */
				 || (mii_reg5 & 0x00C0) == 0x0040) /* 10T-FD, but not 100-HD */
			lp->is_full_duplex = 1;
		printk(KERN_INFO "%s: MII #%d status %4.4x, link partner capability "
			   "%4.4x, setting %s-duplex.\n", dev->name, lp->phys[0],
			   mii_reg1, mii_reg5, lp->is_full_duplex ? "full" : "half");
	}

#if CS_RELEASE_CODE <= 0x2911
    irq2dev_map[dev->irq] = dev;
#endif

	tc574_reset(dev);

	DEBUG(2, "%s: opened, status %4.4x.\n",
		  dev->name, inw(ioaddr + EL3_STATUS));

	return 0;					/* Always succeed */
}

static void tc574_tx_timeout(struct device *dev)
{
	int ioaddr = dev->base_addr;

	printk(KERN_NOTICE "%s: Transmit timed out, Tx_status %2.2x "
		   "status %4.4x Tx FIFO room %d.\n", dev->name,
		   inb(ioaddr + TxStatus), inw(ioaddr + EL3_STATUS),
		   inw(ioaddr + TxFree));
	dev->trans_start = jiffies;
	/* Issue TX_RESET and TX_START commands. */
	outw(TxReset, ioaddr + EL3_CMD);
	outw(TxEnable, ioaddr + EL3_CMD);
	dev->tbusy = 0;
}

static int el3_start_xmit(struct sk_buff *skb, struct device *dev)
{
	struct el3_private *lp = (struct el3_private *)dev->priv;
	int ioaddr = dev->base_addr;
	long flags;

	/* Transmitter timeout, serious problems. */
	if (test_and_set_bit(0, (void*)&dev->tbusy) != 0) {
		if (jiffies - dev->trans_start >= TX_TIMEOUT) {
			tc574_tx_timeout(dev);
			lp->stats.tx_errors++;
		}
		return 1;
	}

	DEBUG(3, "%s: el3_start_xmit(length = %ld) called, "
		  "status %4.4x.\n", dev->name, (long)skb->len,
		  inw(ioaddr + EL3_STATUS));

#ifdef PCMCIA_DEBUG
	{							/* Error-checking code, delete someday. */
		ushort status = inw(ioaddr + EL3_STATUS);
		if (status & 0x0003		/* IRQ line active, missed one. */
			&& inw(ioaddr + EL3_STATUS) & 3) { /* Make sure. */
			printk(KERN_NOTICE "%s: missed interrupt, status then %04x"
				   " now %04x  Tx %2.2x Rx %4.4x.\n", dev->name, status,
				   inw(ioaddr + EL3_STATUS), inb(ioaddr + TxStatus),
				   inw(ioaddr + RxStatus));
			if (status & 0x0002) /* Adaptor failure */
				tc574_reset(dev);
			/* Fake interrupt trigger by masking, acknowledge interrupts. */
			outw(SetStatusEnb | 0x00, ioaddr + EL3_CMD);
			outw(AckIntr | IntLatch | TxAvailable | RxEarly | IntReq,
				 ioaddr + EL3_CMD);
			outw(SetStatusEnb | 0xff, ioaddr + EL3_CMD);
		}
	}
#endif

#if 0
	if (use_fifo_buffer) {
		/* Avoid other accesses to the chip while RunnerWrCtrl is non-zero. */
		save_flags(flags);
		cli();
		outw((((skb->len + 7)>>2)<<1), ioaddr + RunnerWrCtrl);
	}
	printk("TxFree %d, ", inw(ioaddr + TxFree));
	printk("tx length %lx, Runner Write control is %4.4x.\n",
		   skb->len, inw(ioaddr + RunnerWrCtrl));
#else
	save_flags(flags);
	if (use_fifo_buffer) {
		/* Avoid other accesses to the chip while RunnerWrCtrl is non-zero. */
		cli();
		outw((((skb->len + 7)>>2)<<1), ioaddr + RunnerWrCtrl);
	}
#endif

	/* Put out the doubleword header... */
	outl(skb->len, ioaddr + TX_FIFO);
	/* ... and the packet rounded to a doubleword. */
	if (dev->mem_start)
		memcpy((void*)dev->mem_start, skb->data, (skb->len + 3) & ~3);
	else
		outsl(ioaddr + TX_FIFO, skb->data, (skb->len + 3) >> 2);

#if 0
	if (use_fifo_buffer) {
		printk("Runner Write/Read control is %4.4x/%4.4x, TxFree %d.\n",
			   inw(ioaddr + RunnerWrCtrl), inw(ioaddr + RunnerRdCtrl),
			   inw(ioaddr + TxFree));
		restore_flags(flags);
	}
#else
	restore_flags(flags);
#endif

	dev->trans_start = jiffies;

	/* TxFree appears only in Window 1, not offset 0x1c. */
	if (inw(ioaddr + TxFree) > 1536) {
		dev->tbusy = 0;
	} else
		/* Interrupt us when the FIFO has room for max-sized packet. */
		outw(SetTxThreshold + 1536, ioaddr + EL3_CMD);

	DEV_KFREE_SKB (skb);

	/* Clear the Tx status stack. */
	{
		char tx_status;
		int i = 16;

		while ((--i > 0) && (tx_status = inb(ioaddr + TxStatus)) > 0) {
			if (tx_status & 0x38) lp->stats.tx_aborted_errors++;
			if (tx_status & 0x30) outw(TxReset, ioaddr + EL3_CMD);
			if (tx_status & 0x3C) outw(TxEnable, ioaddr + EL3_CMD);
			outb(0x00, ioaddr + TxStatus); /* Pop the status stack. */
		}
	}

	return 0;
}

/* The EL3 interrupt handler. */
static void el3_interrupt IRQ(int irq, void *dev_id, struct pt_regs *regs)
{
#if CS_RELEASE_CODE > 0x2911
	struct device *dev = (struct device *)dev_id;
#else
	struct device *dev = (struct device *)irq2dev_map[irq];
#endif
	struct el3_private *lp;
	int ioaddr, status;
	int work_budget = max_interrupt_work;

	if ((dev == NULL) || !dev->start)
		return;
	lp = (struct el3_private *)dev->priv;
	ioaddr = dev->base_addr;

#ifdef PCMCIA_DEBUG
	if (test_and_set_bit(0, (void*)&dev->interrupt)) {
		printk(KERN_NOTICE "%s: re-entering the interrupt handler.\n",
			   dev->name);
		return;
	}
	DEBUG(3, "%s: interrupt, status %4.4x.\n",
		  dev->name, inw(ioaddr + EL3_STATUS));
#endif

	while ((status = inw(ioaddr + EL3_STATUS)) &
		   (IntLatch | RxComplete | StatsFull)) {
		if ((dev->start == 0) || ((status & 0xe000) != 0x2000)) {
			DEBUG(1, "%s: Interrupt from dead card\n", dev->name);
			break;
		}

		if (status & RxComplete)
			work_budget = el3_rx(dev, work_budget);

		if (status & TxAvailable) {
			DEBUG(3, "    TX room bit was handled.\n");
			/* There's room in the FIFO for a full-sized packet. */
			outw(AckIntr | TxAvailable, ioaddr + EL3_CMD);
			dev->tbusy = 0;
			mark_bh(NET_BH);
		}

		if (status & TxComplete) {		/* Handle abnormal termination ASAP. */
			char tx_status;
			int i = 16;
			while ((--i > 0) && (tx_status = inb(ioaddr + TxStatus)) > 0) {
				if (tx_status & 0x38) lp->stats.tx_aborted_errors++;
				if (tx_status & 0x30) outw(TxReset, ioaddr + EL3_CMD);
				if (tx_status & 0x3C) outw(TxEnable, ioaddr + EL3_CMD);
				outb(0x00, ioaddr + TxStatus); /* Pop the status stack. */
			}
		}

		if (status & (AdapterFailure | RxEarly | StatsFull)) {
			/* Handle all uncommon interrupts. */
			if (status & StatsFull)		/* Empty statistics. */
				update_stats(ioaddr, dev);
			if (status & RxEarly) {	/* Rx early is unused. */
				work_budget = el3_rx(dev, work_budget);
				outw(AckIntr | RxEarly, ioaddr + EL3_CMD);
			}
			if (status & AdapterFailure) {
				int i;
				u16 fifo_diag;
				EL3WINDOW(4);
				fifo_diag = inw(ioaddr + Wn4_FIFODiag);
				EL3WINDOW(1);
				printk(KERN_NOTICE"%s: Host error, FIFO diagnostic register "
					   "%4.4x.\n", dev->name, fifo_diag);
				/* Adapter failure requires Rx reset and reinit. */
				if (fifo_diag & 0x0400) {
					outw(TxReset, ioaddr + EL3_CMD);
					for (i = 500; i >= 0 ; i--)
						if ((inw(ioaddr + EL3_STATUS) & CmdBusy) == 0) break;
				}
				outw(RxReset, ioaddr + EL3_CMD);
				for (i = 500; i >= 0 ; i--)
					if ((inw(ioaddr + EL3_STATUS) & CmdBusy) == 0) break;
				set_rx_mode(dev);
				outw(RxEnable, ioaddr + EL3_CMD);
				outw(TxEnable, ioaddr + EL3_CMD);
				outw(AckIntr | AdapterFailure, ioaddr + EL3_CMD);
			}
		}

		if (--work_budget < 0) {
			printk(KERN_NOTICE "%s: Too much work in interrupt, "
				   "status %4.4x.\n", dev->name, status);
			/* Clear all interrupts */
			outw(AckIntr | 0xFF, ioaddr + EL3_CMD);
			break;
		}
		/* Acknowledge the IRQ. */
		outw(AckIntr | IntReq | IntLatch, ioaddr + EL3_CMD);
	}

#ifdef PCMCIA_DEBUG
	DEBUG(3, "%s: exiting interrupt, status %4.4x.\n",
		  dev->name, inw(ioaddr + EL3_STATUS));
	dev->interrupt = 0;
#endif
	return;
}

static struct net_device_stats *el3_get_stats(struct device *dev)
{
	struct el3_private *lp = (struct el3_private *)dev->priv;

	if (dev->start)
		update_stats(dev->base_addr, dev);
	return &lp->stats;
}

/*  Update statistics.
	Suprisingly this need not be run single-threaded, but it effectively is.
	The counters clear when read, so the adds must merely be atomic.
 */
static void update_stats(int ioaddr, struct device *dev)
{
	struct el3_private *lp = (struct el3_private *)dev->priv;
	u8 upper_cnt;

	DEBUG(2, "%s: updating the statistics.\n", dev->name);

	if (inw(ioaddr+EL3_STATUS) == 0xffff) /* No card. */
		return;

	/* Unlike the 3c509 we need not turn off stats updates while reading. */
	/* Switch to the stats window, and read everything. */
	EL3WINDOW(6);
	lp->stats.tx_carrier_errors 	+= inb(ioaddr + 0);
	lp->stats.tx_heartbeat_errors	+= inb(ioaddr + 1);
	/* Multiple collisions. */	   	inb(ioaddr + 2);
	lp->stats.collisions		+= inb(ioaddr + 3);
	lp->stats.tx_window_errors	+= inb(ioaddr + 4);
	lp->stats.rx_fifo_errors	+= inb(ioaddr + 5);
	lp->stats.tx_packets		+= inb(ioaddr + 6);
	upper_cnt = inb(ioaddr + 9);
	lp->stats.tx_packets		+= (upper_cnt&0x30) << 4;
	/* Rx packets   */			inb(ioaddr + 7);
	/* Tx deferrals */			inb(ioaddr + 8);
#if (LINUX_VERSION_CODE >= VERSION(2,1,25))
	lp->stats.rx_bytes += inw(ioaddr + 10);
	lp->stats.tx_bytes += inw(ioaddr + 12);
#else
	inw(ioaddr + 10);
	inw(ioaddr + 12);
#endif

	/* With Vortex and later we must also clear the BadSSD counter. */
	EL3WINDOW(4);
	inb(ioaddr + 12);

	EL3WINDOW(1);
}

static int el3_rx(struct device *dev, int worklimit)
{
	struct el3_private *lp = (struct el3_private *)dev->priv;
	int ioaddr = dev->base_addr;
	short rx_status;

	DEBUG(3, "%s: in rx_packet(), status %4.4x, rx_status %4.4x.\n",
		  dev->name, inw(ioaddr+EL3_STATUS), inw(ioaddr+RxStatus));
	while ((rx_status = inw(ioaddr + RxStatus)) > 0
		   &&  --worklimit >= 0) {
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

			skb = dev_alloc_skb(pkt_len+5);

			DEBUG(3, "    Receiving packet size %d status %4.4x.\n",
				  pkt_len, rx_status);
			if (skb != NULL) {
				skb->dev = dev;

#if 1
				if (use_fifo_buffer)
					outw(((pkt_len + 3)>>2)<<1, ioaddr + RunnerRdCtrl);
#else
				if (use_fifo_buffer)
					outw((((pkt_len + 3)>>2)), ioaddr + RunnerRdCtrl);
				printk("%s: Start Rx %d -- RunnerRdCtrl is %4.4x.\n",
					   dev->name, pkt_len, inw(ioaddr + RunnerRdCtrl));
#endif
				skb_reserve(skb, 2);
				if (dev->mem_start) {
					memcpy((void*)dev->mem_start,
						   skb_put(skb, pkt_len), (pkt_len+3)&~3);
				} else {
					insl(ioaddr+RX_FIFO, skb_put(skb, pkt_len),
						 (pkt_len+3)>>2);
				}
				skb->protocol = eth_type_trans(skb, dev);

#if 0
				printk("%s: RunnerRdCtrl is now %4.4x.\n",
					   dev->name, inw(ioaddr + RunnerRdCtrl));
				outw(0, ioaddr + RunnerRdCtrl);
				printk("%s: Rx packet with data %2.2x:%2.2x:%2.2x:....\n",
					   dev->name, skb->head[0], skb->head[1], skb->head[2]);
#endif
				outw(RxDiscard, ioaddr + EL3_CMD); /* Pop top Rx packet. */

				netif_rx(skb);
				lp->stats.rx_packets++;
				continue;
			} else
				DEBUG(1, "%s: couldn't allocate a sk_buff of"
					  " size %d.\n", dev->name, pkt_len);
		}
		outw(RxDiscard, ioaddr + EL3_CMD); /* Rx discard */
		lp->stats.rx_dropped++;
		/* Wait a limited time to skip this packet. */
		{
			int i = 200;
			while (--i >= 0  &&  (inw(ioaddr + EL3_STATUS) & CmdBusy))
				;
		}
	}

	return worklimit;
}

#ifdef HAVE_PRIVATE_IOCTL
/* Provide ioctl() calls to examine the MII xcvr state. */
static int private_ioctl(struct device *dev, struct ifreq *rq, int cmd)
{
	struct el3_private *vp = (struct el3_private *)dev->priv;
	int ioaddr = dev->base_addr;
	u16 *data = (u16 *)&rq->ifr_data;
	int phy = vp->phys[0] & 0x1f;

	DEBUG(2, "%s: In ioct(%-.6s, %#4.4x) %4.4x %4.4x %4.4x %4.4x.\n",
		  dev->name, rq->ifr_ifrn.ifrn_name, cmd,
		  data[0], data[1], data[2], data[3]);

    switch(cmd) {
	case SIOCDEVPRIVATE:		/* Get the address of the PHY in use. */
		data[0] = phy;
	case SIOCDEVPRIVATE+1:		/* Read the specified MII register. */
		{
			int saved_window;
			long flags;

			save_flags(flags);
			cli();
			saved_window = inw(ioaddr + EL3_CMD) >> 13;
			EL3WINDOW(4);
			data[3] = mdio_read(ioaddr, data[0] & 0x1f, data[1] & 0x1f);
			EL3WINDOW(saved_window);
			restore_flags(flags);
			return 0;
		}
	case SIOCDEVPRIVATE+2:		/* Write the specified MII register */
		{
			int saved_window;
			long flags;

			if (!suser())
				return -EPERM;
			save_flags(flags);
			cli();
			saved_window = inw(ioaddr + EL3_CMD) >> 13;
			EL3WINDOW(4);
			mdio_write(ioaddr, data[0] & 0x1f, data[1] & 0x1f, data[2]);
			EL3WINDOW(saved_window);
			restore_flags(flags);
			return 0;
		}
	default:
		return -EOPNOTSUPP;
	}
}
#endif  /* HAVE_PRIVATE_IOCTL */

/* The Odie chip has a 64 bin multicast filter, but the bit layout is not
   documented.  Until it is we revert to receiving all multicast frames when
   any multicast reception is desired.
   Note: My other drivers emit a log message whenever promiscuous mode is
   entered to help detect password sniffers.  This is less desirable on
   typical PC card machines, so we omit the message.
   */

static void set_rx_mode(struct device *dev)
{
	int ioaddr = dev->base_addr;

	if (dev->flags & IFF_PROMISC)
		outw(SetRxFilter | RxStation | RxMulticast | RxBroadcast | RxProm,
			 ioaddr + EL3_CMD);
	else if (dev->mc_count || (dev->flags & IFF_ALLMULTI))
		outw(SetRxFilter|RxStation|RxMulticast|RxBroadcast, ioaddr + EL3_CMD);
	else
		outw(SetRxFilter | RxStation | RxBroadcast, ioaddr + EL3_CMD);
}

static int el3_close(struct device *dev)
{
	int ioaddr = dev->base_addr;
	dev_link_t *link;

	for (link = dev_list; link; link = link->next)
		if (link->priv == dev) break;
	if (link == NULL)
		return -ENODEV;

	DEBUG(2, "%s: shutting down ethercard.\n", dev->name);

	/* Turn off statistics ASAP.  We update lp->stats below. */
	outw(StatsDisable, ioaddr + EL3_CMD);

	/* Disable the receiver and transmitter. */
	outw(RxDisable, ioaddr + EL3_CMD);
	outw(TxDisable, ioaddr + EL3_CMD);

	if (dev->if_port == 2)
		/* Turn off thinnet power.  Green! */
		outw(StopCoax, ioaddr + EL3_CMD);
	else if (dev->if_port == 1) {
		/* Disable link beat and jabber, if_port may change ere next open(). */
		EL3WINDOW(4);
		outw(inw(ioaddr + Wn4_Media) & ~MEDIA_TP, ioaddr + Wn4_Media);
	}

	/* Note: Switching to window 0 may disable the IRQ. */
	EL3WINDOW(0);

	update_stats(ioaddr, dev);

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


int init_module(void)
{
	servinfo_t serv;

	/* Always emit the version, before any failure. */
	printk(KERN_INFO"%s", tc574_version);
	DEBUG(0, "%s\n", version);
	CardServices(GetCardServicesInfo, &serv);
	if (serv.Revision != CS_RELEASE_CODE) {
		printk(KERN_NOTICE "3c574_cs: Card Services release "
			   "does not match!\n");
		return -1;
	}
	register_pcmcia_driver(&dev_info, &tc574_attach, &tc574_detach);
	return 0;
}

void cleanup_module(void)
{
	DEBUG(0, "3c574_cs: unloading\n");
	unregister_pcmcia_driver(&dev_info);
	while (dev_list != NULL)
		tc574_detach(dev_list);
}

/*
 * Local variables:
 *  compile-command: "make 3c574_cs.o"
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  tab-width: 4
 * End:
 */
