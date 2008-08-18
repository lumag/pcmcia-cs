/*======================================================================

    Kernel fixups for PCI device support
    
    pci_fixup.c 1.10 2000/03/08 00:29:36
    
    PCI bus fixups: various bits of code that don't really belong in
    the PCMCIA subsystem, but may or may not be available from the
    kernel, depending on kernel version.  The basic idea is to make
    2.0.* and 2.2.* kernels look like they have the 2.3.* features.

======================================================================*/

#include <pcmcia/config.h>
#define __NO_VERSION__
#include <pcmcia/k_compat.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/malloc.h>
#include <linux/pci.h>

/* We use these for setting up CardBus bridges */
#include "yenta.h"
#include "i82365.h"

#if (LINUX_VERSION_CODE < VERSION(2,3,24))

/* Default memory base addresses for CardBus controllers */
static u_int cb_mem_base[] = { 0x0, 0x68000000, 0xf8000000 };
MODULE_PARM(cb_mem_base, "i");

/* PCI bus number overrides for CardBus controllers */
#define INT_MODULE_PARM(n, v) static int n = v; MODULE_PARM(n, "i")
INT_MODULE_PARM(cb_bus_base, 0);
INT_MODULE_PARM(cb_bus_step, 2);

#endif

/* (exported) mask of interrupts reserved for PCI devices */
u32 pci_irq_mask = 0;

/*======================================================================

    Basic PCI services missing from older kernels: device lookup, etc

======================================================================*/

#if (LINUX_VERSION_CODE < VERSION(2,1,0))
struct pci_dev *pci_devices = NULL;
struct pci_bus pci_root = {
    parent:	NULL,
    children:	NULL,
    next:	NULL,
    self:	NULL,
    devices:	NULL,
    number:	0
};
#endif

#if (LINUX_VERSION_CODE < VERSION(2,1,93))

struct pci_dev *pci_find_slot(u_int bus, u_int devfn)
{
    struct pci_dev *dev;
    for (dev = pci_devices; dev; dev = dev->next)
	if ((dev->devfn == devfn) && (bus == dev->bus->number))
	    return dev;
#if (LINUX_VERSION_CODE > VERSION(2,1,0))
    return NULL;
#else
    {
	struct pci_bus *b;
	u8 hdr;
	u32 id, class;

	if (pcibios_read_config_byte(bus, devfn & ~7, PCI_HEADER_TYPE, &hdr))
	    return NULL;
	if (PCI_FUNC(devfn) && !(hdr & 0x80))
	    return NULL;
	pcibios_read_config_dword(bus, devfn, PCI_VENDOR_ID, &id);
	if ((id == 0) || (id == 0xffffffff))
	    return NULL;
	dev = kmalloc(sizeof *dev, GFP_ATOMIC);
	if (!dev)
	    return NULL;
	memset(dev, 0, sizeof *dev);
	dev->devfn = devfn;
	pcibios_read_config_byte(bus, devfn, PCI_INTERRUPT_LINE, &dev->irq);
	dev->vendor = id & 0xffff;
	dev->device = id >> 16;
	pcibios_read_config_dword(bus, devfn, PCI_CLASS_REVISION, &class);
	if (dev->irq == 255)
	    dev->irq = 0;
	dev->class = class >> 8;
	for (b = &pci_root; b; b = b->next)
	    if (b->number == bus) break;
	if (!b) {
	    b = kmalloc(sizeof *b, GFP_ATOMIC);
	    if (!b) {
		kfree(dev);
		return NULL;
	    }
	    memset(b, 0, sizeof *b);
	    b->number = bus;
	    b->next = pci_root.next;
	    pci_root.next = b;
	}
	dev->bus = b;
	return dev;
    }
#endif
}

struct pci_dev *pci_find_class(u_int class, struct pci_dev *from)
{
    static u16 index = 0;
    u8 bus, devfn;
    if (from == NULL)
	index = 0;
    if (pcibios_find_class(class, index++, &bus, &devfn) == 0)
	return pci_find_slot(bus, devfn);
    else
	return NULL;
}

#endif /* (LINUX_VERSION_CODE < VERSION(2,1,93)) */

/*======================================================================

    PCI Interrupt Routing Table parser

    This only needs to be done once per boot: we scan the BIOS for
    the routing table, and then look for devices that have interrupt
    assignments that the kernel doesn't know about.  If we find any,
    we update their pci_dev structures and write the PCI interrupt
    line registers.
    
======================================================================*/

#if (LINUX_VERSION_CODE < VERSION(2,3,24)) && defined(__i386__)

#pragma pack(1)

struct slot_entry {
    u8		bus, devfn;
    struct pirq_pin {
	u8	link;
	u16	irq_map;
    } pin[4];
    u8		slot;
    u8		reserved;
};

struct routing_table {
    u32		signature;
    u8		minor, major;
    u16		size;
    u8		bus, devfn;
    u16		pci_mask;
    u32		compat;
    u32		miniport;
    u8		reserved[11];
    u8		checksum;
    struct slot_entry entry[0];
};

#pragma pack()

/* Global variables for current routing table info */
static struct routing_table *pirq = NULL;
static struct pci_dev *pirq_router = NULL;
static u8 (*xlate_link)(struct pci_dev *, u8) = NULL;

/*
  The meaning of the link bytes in the routing table is vendor
  specific.  For the Intel 82371 family, the link byte points to a
  PCI configuration register that contains the ISA interrupt
  assignment.
*/

static u8 i82371_link(struct pci_dev *router, u8 link)
{
    u8 pirq;
    pci_read_config_byte(router, link, &pirq);
    return (pirq < 16) ? pirq : 0;
}

static u8 i82371_init(struct pci_dev *router, struct pirq_pin *pin)
{
    u16 map = pin->irq_map;
    u8 irq = 0;
    if (pirq->pci_mask)
	map &= pirq->pci_mask;
    if (map && (!(map & (map-1)))) {
	irq = ffs(map)-1;
	pci_write_config_byte(router, pin->link, irq);
    }
    return irq;
}

static u8 via_link(struct pci_dev *router, u8 link)
{
    u8 pirq = 0;
    if (link < 6)
	pci_read_config_byte(router, 0x55 + (link>>1), &pirq);
    return (link & 1) ? (pirq >> 4) : (pirq & 15);
}

/*
  A table of all the PCI interrupt routers for which we know how to
  interpret the link bytes.  For now, there isn't much to it.
*/

#ifndef PCI_DEVICE_ID_INTEL_82371FB_0
#define PCI_DEVICE_ID_INTEL_82371FB_0 0x122e
#endif
#ifndef PCI_DEVICE_ID_VIA_82C596
#define PCI_DEVICE_ID_VIA_82C596 0x0596
#endif
#ifndef PCI_DEVICE_ID_VIA_82C686
#define PCI_DEVICE_ID_VIA_82C686 0x0686
#endif

#define ID(a,b) PCI_VENDOR_ID_##a,PCI_DEVICE_ID_##a##_##b

struct {
    u16 vendor, device;
    u8 (*xlate_link)(struct pci_dev *, u8);
} irq_router[] = {
    { ID(INTEL, 82371FB_0), &i82371_link },
    { ID(INTEL, 82371SB_0), &i82371_link },
    { ID(INTEL, 82371AB_0), &i82371_link },
    { ID(VIA, 82C596), &via_link },
    { ID(VIA, 82C686), &via_link }
};
#define ROUTER_COUNT (sizeof(irq_router)/sizeof(irq_router[0]))

#ifndef __va
#define __va(x) (x)
#endif

void scan_pirq_table(void)
{
    struct routing_table *r;
    struct pci_dev *router, *dev;
    u8 pin, fn, *p;
    int i;
    struct slot_entry *e;

    /* Scan the BIOS for the routing table signature */
    for (p = (u8 *)__va(0xf0000); p < (u8 *)__va(0xfffff); p += 16)
	if ((p[0] == '$') && (p[1] == 'P') &&
	    (p[2] == 'I') && (p[3] == 'R')) break;
    if (p >= (u8 *)__va(0xfffff))
	return;
    
    pirq = r = (struct routing_table *)p;
    printk(KERN_INFO "PCI routing table version %d.%d at %#06x\n",
	   r->major, r->minor, (u32)r & 0xfffff);
    pci_irq_mask |= r->pci_mask;

    pirq_router = router = pci_find_slot(r->bus, r->devfn);
    if (router) {
	for (i = 0; i < ROUTER_COUNT; i++) {
	    if ((router->vendor == irq_router[i].vendor) &&
		(router->device == irq_router[i].device))
		break;
	    if (((r->compat & 0xffff) == irq_router[i].vendor) &&
		((r->compat >> 16) == irq_router[i].device))
		break;
	}
	if (i == ROUTER_COUNT)
	    printk(KERN_INFO "  unknown PCI interrupt router %04x:%04x\n",
		   router->vendor, router->device);
	else
	    xlate_link = irq_router[i].xlate_link;
    }

    for (e = r->entry; (u8 *)e < p+r->size; e++) {
	for (fn = 0; fn < 8; fn++) {
	    dev = pci_find_slot(e->bus, e->devfn | fn);
	    if ((dev == NULL) || (dev->irq != 0)) continue;
	    pci_read_config_byte(dev, PCI_INTERRUPT_PIN, &pin);
	    if ((pin == 0) || (pin == 255)) continue;
	    if (xlate_link) {
		dev->irq = xlate_link(router, e->pin[pin-1].link);
	    } else {
		/* Fallback: see if only one irq possible */
		int map = e->pin[pin-1].irq_map;
		if (map && (!(map & (map-1))))
		    dev->irq = ffs(map)-1;
	    }
	    if (dev->irq) {
		printk(KERN_INFO "  %02x:%02x.%1x -> irq %d\n",
		       e->bus, PCI_SLOT(dev->devfn),
		       PCI_FUNC(dev->devfn), dev->irq);
		pci_write_config_byte(dev, PCI_INTERRUPT_LINE,
				      dev->irq);
	    }
	}
    }
}

#endif /* (LINUX_VERSION_CODE >= VERSION(2,1,103)) */

/*======================================================================

    PCI device enabler

    This is not at all generic... it is mostly a hack to correctly
    configure CardBus bridges.
    
======================================================================*/

#if (LINUX_VERSION_CODE < VERSION(2,3,24))

static int check_cb_mapping(u_int phys)
{
    /* A few sanity checks to validate the bridge mapping */
    char *virt = ioremap(phys, 0x1000);
    int ret = ((readb(virt+0x800+I365_IDENT) & 0x70) ||
	       (readb(virt+0x800+I365_CSC) &&
		readb(virt+0x800+I365_CSC) &&
		readb(virt+0x800+I365_CSC)));
    ret |= readl(virt+CB_SOCKET_FORCE);
    ret |= (readl(virt+CB_SOCKET_STATE) >> 16) != 0x3000;
    iounmap(virt);
    return ret;
}

static void setup_cb_bridge(struct pci_dev *dev)
{
    u8 bus, sub;
    u32 phys;
    int i;

    /* This is nasty, but where else can we put it? */
    if (PCI_FUNC(dev->devfn) == 0) {
	struct pci_dev *sib;
	sib = pci_find_slot(dev->bus->number, dev->devfn+1);
	if (sib) {
	    u8 a, b;
	    /* Check for bad PCI bus numbering */
	    pci_read_config_byte(dev, CB_CARDBUS_BUS, &a);
	    pci_read_config_byte(sib, CB_CARDBUS_BUS, &b);
	    if (a == b) {
		pci_write_config_byte(dev, CB_CARDBUS_BUS, 0);
		pci_write_config_byte(sib, CB_CARDBUS_BUS, 0);
	    }
	}
    }

    /* Assign PCI bus numbers, if needed */
    pci_read_config_byte(dev, CB_CARDBUS_BUS, &bus);
    pci_read_config_byte(dev, CB_SUBORD_BUS, &sub);
    if ((cb_bus_base > 0) || (bus == 0)) {
	if (cb_bus_base <= 0) cb_bus_base = 0x20;
	bus = cb_bus_base;
	sub = cb_bus_base+cb_bus_step;
	cb_bus_base += cb_bus_step+1;
	pci_write_config_byte(dev, CB_CARDBUS_BUS, bus);
	pci_write_config_byte(dev, CB_SUBORD_BUS, sub);
    }

#if (LINUX_VERSION_CODE >= VERSION(2,1,103))
    /* Create pci_bus structure for the CardBus, if needed */
    {
	struct pci_bus *child, *parent = dev->bus;
	for (child = parent->children; child; child = child->next)
	    if (child->number == bus) break;
	if (!child) {
	    child = kmalloc(sizeof(struct pci_bus), GFP_KERNEL);
	    memset(child, 0, sizeof(struct pci_bus));
	    child->self = dev;
	    child->primary = bus;
	    child->number = child->secondary = bus;
	    child->subordinate = sub;
	    child->parent = parent;
#if (LINUX_VERSION_CODE >= VERSION(2,3,15))
	    child->ops = parent->ops;
#endif
	    child->next = parent->children;
	    parent->children = child;
	}
    }
#endif

    /* Map the CardBus bridge registers, if needed */
    pci_write_config_dword(dev, CB_LEGACY_MODE_BASE, 0);
    pci_read_config_dword(dev, PCI_BASE_ADDRESS_0, &phys);
    if ((phys == 0) || (cb_mem_base[0] != 0)) {
	/* Make sure the bridge is awake so we can test it */
	pci_set_power_state(dev, 0);
	for (i = 0; i < sizeof(cb_mem_base)/sizeof(u_int); i++) {
	    phys = cb_mem_base[i];
	    if (phys == 0) continue;
	    pci_write_config_dword(dev, PCI_BASE_ADDRESS_0, phys);
	    if (check_cb_mapping(phys) == 0) break;
	}
	if (i == sizeof(cb_mem_base)/sizeof(u_int)) {
	    pci_write_config_dword(dev, PCI_BASE_ADDRESS_0, 0);
	} else {
	    cb_mem_base[0] = cb_mem_base[i] + 0x1000;
	}
    }
}

#define CMD_DFLT (PCI_COMMAND_IO | PCI_COMMAND_MEMORY | \
		  PCI_COMMAND_MASTER | PCI_COMMAND_WAIT)

#ifdef __i386__
static void setup_cb_bridge_irq(struct pci_dev *dev)
{
    struct slot_entry *e;
    u8 pin;
    u32 phys;
    char *virt;

    pci_read_config_byte(dev, PCI_INTERRUPT_PIN, &pin);
    pci_read_config_dword(dev, PCI_BASE_ADDRESS_0, &phys);
    if (!pin || !phys)
	return;
    virt = ioremap(phys, 0x1000);
    if (virt) {
	/* Disable any pending interrupt sources */
	writel(0, virt+CB_SOCKET_MASK);
	writel(-1, virt+CB_SOCKET_EVENT);
	iounmap(virt);
    }
    for (e = pirq->entry; (u8 *)e < (u8 *)pirq + pirq->size; e++) {
	if ((e->bus != dev->bus->number) ||
	    (e->devfn != (dev->devfn & ~7)))
	    continue;
	dev->irq = i82371_init(pirq_router, &e->pin[pin-1]);
	pci_write_config_byte(dev, PCI_INTERRUPT_LINE, dev->irq);
	break;
    }
}
#endif

int pci_enable_device(struct pci_dev *dev)
{
    pci_write_config_word(dev, PCI_COMMAND, CMD_DFLT);
    if ((dev->class >> 8) == PCI_CLASS_BRIDGE_CARDBUS) {
	setup_cb_bridge(dev);
    }
#ifdef __i386__
    /* In certain cases, if the interrupt can be deduced, but was
       unrouted when the pirq table was scanned, we'll try to set it
       up now. */
    if (!dev->irq && pirq && (xlate_link == &i82371_link) &&
	((dev->class >> 8) == PCI_CLASS_BRIDGE_CARDBUS)) {
	setup_cb_bridge_irq(dev);
    }
#endif
    return 0;
}

int pci_set_power_state(struct pci_dev *dev, int state)
{
    u8 a, b, pmcs;
    pci_read_config_byte(dev, PCI_STATUS, &a);
    if (a & PCI_STATUS_CAPLIST) {
	pci_read_config_byte(dev, PCI_CB_CAPABILITY_POINTER, &b);
	while (b != 0) {
	    pci_read_config_byte(dev, b+PCI_CAPABILITY_ID, &a);
	    if (a == PCI_CAPABILITY_PM) {
		pmcs = b + PCI_PM_CONTROL_STATUS;
		/* Make sure we're in D0 state */
		pci_write_config_word(dev, pmcs, PCI_PMCS_PWR_STATE_D0);
		break;
	    }
	    pci_read_config_byte(dev, b+PCI_NEXT_CAPABILITY, &b);
	}
    }
    return 0;
}

#endif /* (LINUX_VERSION_CODE < VERSION(2,3,24)) */

/*======================================================================

    General setup and cleanup entry points

======================================================================*/

void pci_fixup_init(void)
{
    struct pci_dev *p;

#if (LINUX_VERSION_CODE < VERSION(2,3,24)) && defined(__i386__)
    scan_pirq_table();
#endif

    pci_for_each_dev(p)
	pci_irq_mask |= (1<<p->irq);

#ifdef __alpha__
#define PIC 0x4d0
    pci_irq_mask |= inb(PIC) | (inb(PIC+1) << 8);
#endif
}

void pci_fixup_done(void)
{
#if (LINUX_VERSION_CODE < VERSION(2,1,0))
    struct pci_dev *d, *dn;
    struct pci_bus *b, *bn;
    for (d = pci_devices; d; d = dn) {
	dn = d->next;
	kfree(d);
    }
    for (b = pci_root.next; b; b = bn) {
	bn = b->next;
	kfree(b);
    }
#endif
}
