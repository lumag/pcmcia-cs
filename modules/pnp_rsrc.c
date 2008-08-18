#include <pcmcia/config.h>
#define __NO_VERSION__
#include <pcmcia/k_compat.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/malloc.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/pnp_bios.h>
#include <linux/pnp_resource.h>
#include <asm/irq.h>

/* From rsrc_mgr.c */
void request_io_region(u_long base, u_long num, char *name);

struct irq_entry {
    char *name;
    struct irq_entry *next;
};

static struct irq_entry *irq[NR_IRQS];

int proc_read_irq(char *buf, char **start, off_t pos,
		  int count, int *eof, void *data)
{
    int i;
    struct irq_entry *e;
    char *p = buf;
    for (i = 0; i < NR_IRQS; i++) {
	if (!irq[i]) continue;
	p += sprintf(p, "%3d:  ", i);
	for (e = irq[i]; e; e = e->next) {
	    strcpy(p, e->name);
	    if (e->next)
		strcat(p, ", ");
	    p += strlen(p);
	}
	strcat(p, "\n");
	p++;
    }
    return (p - buf);
}

void alloc_irq(int n, char *name)
{
    struct irq_entry **e = &irq[n];
    while (*e != NULL)
	e = &((*e)->next);
    *e = kmalloc(sizeof(*e), GFP_KERNEL);
    (*e)->name = name;
    (*e)->next = NULL;
}

void free_irqs(void)
{
    int n;
    struct irq_entry *e, *f;
    for (n = 0; n < NR_IRQS; n++)
	for (e = irq[n]; e; e = f) {
	    f = e->next;
	    kfree(e);
	}
}

/*======================================================================

    PCI Interrupt Routing Table parser

    This only needs to be done once per boot: we scan the BIOS for
    the routing table, and then look for devices that have interrupt
    assignments that the kernel doesn't know about.  If we find any,
    we update their pci_dev structures and write the PCI interrupt
    line registers.
    
======================================================================*/

#ifdef CONFIG_PCI

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

static void scan_pirq_table(void)
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

#endif /* CONFIG_PCI */

/*======================================================================

    PCI device resource enumeration

======================================================================*/

#ifdef CONFIG_PCI

static char *pci_names = NULL;

static void pci_claim_resources(void)
{
    struct pci_dev *dev;
    int r;
    unsigned long flags;
#if (LINUX_VERSION_CODE < VERSION(2,3,13))
    unsigned long a;
    u32 b, sz, idx;
    u16 cmd, tmp;
#endif
    char *name;
    
    for (r = 0, dev=pci_devices; dev; dev=dev->next, r++) ;
    name = pci_names = kmalloc(r*12, GFP_KERNEL);
    
    save_flags(flags);
    cli();
    for (dev=pci_devices; dev; dev=dev->next, name += 12) {
	sprintf(name, "pci %02x:%02x.%1x", dev->bus->number,
		PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn));
	if (dev->irq)
	    alloc_irq(dev->irq, name);
#if (LINUX_VERSION_CODE < VERSION(2,3,13))
	/* Disable IO and memory while we fiddle */
	pci_read_config_word(dev, PCI_COMMAND, &cmd);
	tmp = cmd & ~(PCI_COMMAND_IO | PCI_COMMAND_MEMORY);
	pci_write_config_word(dev, PCI_COMMAND, tmp);
	for (idx = 0; idx < 6; idx++) {
	    a = dev->base_address[idx];
	    r = PCI_BASE_ADDRESS_0 + 4*idx;
	    if (((a & PCI_BASE_ADDRESS_SPACE_IO) &&
		 !(a & PCI_BASE_ADDRESS_IO_MASK)) ||
		!(a & PCI_BASE_ADDRESS_MEM_MASK))
		continue;
	    pci_read_config_dword(dev, r, &b);
	    pci_write_config_dword(dev, r, ~0);
	    pci_read_config_dword(dev, r, &sz);
	    pci_write_config_dword(dev, r, b);
	    if (a & PCI_BASE_ADDRESS_SPACE_IO) {
		a &= PCI_BASE_ADDRESS_IO_MASK;
		sz = (~(sz & PCI_BASE_ADDRESS_IO_MASK))+1;
		sz &= 0xffff;
		request_io_region(a, sz, name);
	    } else {
		a &= PCI_BASE_ADDRESS_MEM_MASK;
		sz = (~(sz & PCI_BASE_ADDRESS_MEM_MASK))+1;
		request_mem_region(a, sz, name);
	    }
	}
	if (dev->rom_address & ~1) {
	    r = PCI_ROM_ADDRESS;
	    pci_read_config_dword(dev, r, &b);
	    pci_write_config_dword(dev, r, ~0);
	    pci_read_config_dword(dev, r, &sz);
	    pci_write_config_dword(dev, r, b);
	    sz = (~(sz & ~1))+1;
	    request_mem_region(dev->rom_address, sz, "pci");
	}
	pci_write_config_word(dev, PCI_COMMAND, cmd);
#endif
    }
    restore_flags(flags);
}

#endif /* CONFIG_PCI */

/*======================================================================

    PnP device resource enumeration
    
======================================================================*/

#define flip16(n)	le16_to_cpu(n)
#define flip32(n)	le32_to_cpu(n)

static struct pnp_dev_node_info node_info;

static void pnp_scan_node(char *name, u8 *buf, int len)
{
    union pnp_resource *p = (union pnp_resource *)buf;
    int tag = 0, sz;
    
    while (((u8 *)p < buf+len) && (tag != PNP_RES_SMTAG_END)) {
	if (p->lg.tag & PNP_RES_LARGE_ITEM) {
	    
	    union pnp_large_resource *r = &p->lg.d;
	    tag = p->lg.tag & ~PNP_RES_LARGE_ITEM;
	    sz = flip16(p->lg.sz) + 3;
	    switch (tag) {
	    case PNP_RES_LGTAG_MEM:
		if (r->mem.len && (r->mem.min == r->mem.max))
		    request_mem_region(flip16(r->mem.min)<<8,
				       flip16(r->mem.len), name);
		break;
	    case PNP_RES_LGTAG_MEM32:
		if (r->mem32.len && (r->mem32.min == r->mem32.max))
		    request_mem_region(flip32(r->mem32.min),
				       flip32(r->mem32.len), name);
		break;
	    case PNP_RES_LGTAG_MEM32_FIXED:
		if (r->mem32_fixed.len)
		    request_mem_region(flip32(r->mem32_fixed.base),
				       flip32(r->mem32_fixed.len), name);
		break;
	    }
	    
	} else {
	    
	    union pnp_small_resource *r = &p->sm.d;
	    tag = (p->sm.tag >> 3); sz = (p->sm.tag & 7) + 1;
	    switch (tag) {
	    case PNP_RES_SMTAG_IRQ:
		if (r->irq.mask && !(r->irq.mask & (r->irq.mask-1)))
		    alloc_irq(ffs(flip16(r->irq.mask))-1, name);
		break;
	    case PNP_RES_SMTAG_IO:
		if (r->io.len && (r->io.min == r->io.max))
		    request_io_region(flip16(r->io.min),
				      r->io.len, name);
		break;
	    case PNP_RES_SMTAG_IO_FIXED:
		if (r->io_fixed.len)
		    request_io_region(flip16(r->io_fixed.base),
				      r->io_fixed.len, name);
		break;
	    }
	    
	}
	(u8 *)p += sz;
    }
}

static char *pnp_names = NULL;

static void pnp_claim_resources(void)
{
    struct pnp_bios_node *node;
    char *name;
    u8 num;
    
    node = kmalloc(node_info.max_node_size, GFP_KERNEL);
    pnp_names = kmalloc(node_info.no_nodes*7, GFP_KERNEL);
    for (name = pnp_names, num = 0; num != 0xff; name += 7) {
	pnp_bios_get_dev_node(&num, 0, node);
	sprintf(name, "pnp %02x", node->handle);
	pnp_scan_node(name, node->data, node->size - sizeof(*node));
    }
}

/*====================================================================*/

void pnp_rsrc_init(void)
{
#ifdef CONFIG_PCI
    if (pcibios_present()) {
	scan_pirq_table();
	pci_claim_resources();
    }
#endif
    if (pnp_bios_present()) {
	if ((pnp_bios_dev_node_info(&node_info) == 0) &&
	    (node_info.no_nodes > 0))
	    pnp_claim_resources();
    }
}

void pnp_rsrc_done(void)
{
    if (pnp_names)
	kfree(pnp_names);
#ifdef CONFIG_PCI
    if (pci_names)
	kfree(pci_names);
#endif
    free_irqs();
}
