#include <pcmcia/config.h>
#define __NO_VERSION__
#include <pcmcia/k_compat.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>

/*======================================================================

    PCI Interrupt Routing Table parser

    This only needs to be done once per boot: we scan the BIOS for
    the routing table, and then look for devices that have interrupt
    assignments that the kernel doesn't know about.  If we find any,
    we update their pci_dev structures and write the PCI interrupt
    line registers.
    
======================================================================*/

#pragma pack(1)

struct slot_entry {
    u8		bus, devfn;
    struct {
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

/* We only bother doing this for relatively new kernels */
#if (LINUX_VERSION_CODE >= VERSION(2,1,103))

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

/*
  A table of all the PCI interrupt routers for which we know how to
  interpret the link bytes.  For now, there isn't much to it.
*/

struct {
    u16 vendor, device;
    u8 (*xlate_link)(struct pci_dev *, u8);
} irq_router[] = {
    { PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82371FB_0, &i82371_link },
    { PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82371SB_0, &i82371_link },
    { PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82371AB_0, &i82371_link },
};
#define ROUTER_COUNT (sizeof(irq_router)/sizeof(irq_router[0]))

void scan_pirq_table(void)
{
    struct routing_table *r;
    struct pci_dev *router, *dev;
    u8 (*xlate_link)(struct pci_dev *, u8) = NULL;
    u8 pin, fn, *p;
    int i;
    struct slot_entry *e;

    /* Scan the BIOS for the routing table signature */
    for (p = (u8 *)__va(0xf0000); p < (u8 *)__va(0xfffff); p += 16)
	if ((p[0] == '$') && (p[1] == 'P') &&
	    (p[2] == 'I') && (p[3] == 'R')) break;
    if (p >= (u8 *)__va(0xfffff))
	return;
    
    r = (struct routing_table *)p;
    printk(KERN_INFO "PCI routing table version %d.%d at %#06x\n",
	   r->major, r->minor, (u32)r & 0xfffff);

    router = pci_find_slot(r->bus, r->devfn);
    if (router) {
	for (i = 0; i < ROUTER_COUNT; i++)
	    if ((router->vendor == irq_router[i].vendor) &&
		(router->device == irq_router[i].device))
		break;
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
	    if (pin == 0) continue;
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

#else

void scan_pirq_table(void) { }

#endif /* (LINUX_VERSION_CODE >= VERSION(2,1,103)) */
