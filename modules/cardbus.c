/*======================================================================
  
    Cardbus device configuration
    
    Written by David Hinds, dhinds@hyper.stanford.edu
    
    These routines handle allocating resources for Cardbus cards, as
    well as setting up and shutting down Cardbus sockets.  They are
    called from cs.c in response to Request/ReleaseConfiguration and
    Request/ReleaseIO calls.
    
======================================================================*/

#include <pcmcia/config.h>
#define __NO_VERSION__
#include <pcmcia/k_compat.h>

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/malloc.h>
#include <linux/mm.h>
#include <linux/pci.h>
#include <linux/bios32.h>
#include <linux/ioport.h>

#define PCMCIA_DEBUG 2
static int pc_debug = 2;

#define IN_CARD_SERVICES
#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/ss.h>
#include <pcmcia/cs.h>
#include <pcmcia/bulkmem.h>
#include <pcmcia/cistpl.h>
#include "cs_internal.h"
#include "rsrc_mgr.h"

/*====================================================================*/

#define FIND_FIRST_BIT(n)	((n) - ((n) & ((n)-1)))

#define pci_readb		pcibios_read_config_byte
#define pci_writeb		pcibios_write_config_byte
#define pci_readw		pcibios_read_config_word
#define pci_writew		pcibios_write_config_word
#define pci_readl		pcibios_read_config_dword
#define pci_writel		pcibios_write_config_dword

#define CB_BAR(n)		(PCI_BASE_ADDRESS_0+(4*(n)))

typedef struct cb_config_t {
    u_char		irq;
    u_int		base[6];
    u_int		size[6];
} cb_config_t;

/*=====================================================================

    cb_enable() has the job of configuring a socket for a Cardbus
    card, and initializing the card's PCI configuration registers.

    It first sets up the Cardbus bridge windows, for IO and memory
    accesses.  Then, it initializes each card function's base address
    registers, interrupt line register, and command register.

    It is called as part of the RequestConfiguration card service.
    It should be called after a previous call to cb_config() (via the
    RequestIO service).
    
======================================================================*/

void cb_enable(socket_info_t *s)
{
    u_char i, j, bus = s->cap.cardbus;
    cb_config_t *c = s->cb_config;
    
    DEBUG(1, "cb_enable(bus %d)\n", bus);
    
    /* Configure bridge */
    for (i = 0; i < 3; i++) {
	cb_bridge_map m;
	switch (i) {
	case 0:
	    m.map = 0; m.flags = MAP_IOSPACE | MAP_ACTIVE;
	    m.start = s->io[0].BasePort;
	    m.stop = m.start + s->io[0].NumPorts - 1;
	    break;
	case 1:
	    m.map = m.flags = MAP_ACTIVE;
	    m.start = s->win[0].base;
	    m.stop = m.start + s->win[0].size - 1;
	    break;
	case 2:
	    m.map = 1; m.flags = MAP_PREFETCH | MAP_ACTIVE;
	    m.start = s->win[1].base;
	    m.stop = m.start + s->win[1].size - 1;
	    break;
	}
	if (m.start == 0) continue;
	DEBUG(1, "Bridge map %d (flags 0x%x): 0x%x-0x%x\n",
	      m.map, m.flags, m.start, m.stop);
	s->ss_entry(s->sock, SS_SetBridge, &m);
    }

    /* Set up base address registers */
    for (i = 0; i < s->functions; i++)
	for (j = 0; j < 6; j++) {
	    if (c[i].base[j] != 0)
		pci_writel(bus, i, CB_BAR(j), c[i].base[j]);
	}

    /* Set up PCI interrupt and command registers */
    for (i = 0; i < s->functions; i++)
	pci_writeb(bus, i, PCI_COMMAND, PCI_COMMAND_MASTER |
		   PCI_COMMAND_IO | PCI_COMMAND_MEMORY);
    if (s->irq.AssignedIRQ) {
	for (i = 0; i < s->functions; i++)
	    pci_writeb(bus, i, PCI_INTERRUPT_LINE,
		       s->irq.AssignedIRQ);
	s->socket.io_irq = s->irq.AssignedIRQ;
	s->ss_entry(s->sock, SS_SetSocket, &s->socket);
    }
    
}

/*======================================================================

    cb_disable() unconfigures a Cardbus card previously set up by
    cb_enable().

    It is called from the ReleaseConfiguration service.
    
======================================================================*/

void cb_disable(socket_info_t *s)
{
    u_char i;
    cb_bridge_map m = { 0, 0, 0, 0xffff };

    DEBUG(1, "cb_disable(bus %d)\n", s->cap.cardbus);
    
    /* Turn off bridge windows */
    for (i = 0; i < 3; i++) {
	switch (i) {
	case 0: m.map = 0; m.flags = MAP_IOSPACE; break;
	case 1: m.map = m.flags = 0; break;
	case 2: m.map = 1; m.flags = 0; break;
	}
	s->ss_entry(s->sock, SS_SetBridge, &m);
    }
}

/*=====================================================================

    cb_config() has the job of allocating all system resources that
    a Cardbus card requires.  Rather than using the CIS (which seems
    to not always be present), it treats the card as an ordinary PCI
    device, and probes the base address registers to determine each
    function's IO and memory space needs.

    It is called from the RequestIO card service.
    
======================================================================*/

int cb_config(socket_info_t *s)
{
    u_short vend, v, dev;
    u_char i, j, fn, bus = s->cap.cardbus, *name;
    u_int sz, m, mask[3], num[3], base[3];
    cb_config_t *c;
    int irq, try, ret;
    
    pci_readw(bus, 0, PCI_VENDOR_ID, &vend);
    pci_readw(bus, 0, PCI_DEVICE_ID, &dev);
    DEBUG(1, "cb_config(bus %d): vendor 0x%04x, device 0x%04x\n",
	  bus, vend, dev);

    pci_readb(bus, 0, PCI_HEADER_TYPE, &fn);
    if (fn != 0) {
	/* Count functions */
	for (fn = 0; fn < 8; fn++) {
	    pci_readw(bus, fn, PCI_VENDOR_ID, &v);
	    if (v != vend) break;
	}
    } else fn = 1;
    s->functions = fn;
    
    c = kmalloc(fn * sizeof(struct cb_config_t), GFP_KERNEL);
    memset(c, fn * sizeof(struct cb_config_t), 0);
    s->cb_config = c;
    
    /* Determine IO and memory space needs */
    num[0] = num[1] = num[2] = 0;
    mask[0] = mask[1] = mask[2] = 0;
    for (i = 0; i < fn; i++) {
	for (j = 0; j < 6; j++) {
	    pci_writel(bus, i, CB_BAR(j), 0xffffffff);
	    pci_readl(bus, i, CB_BAR(j), &sz);
	    if (sz == 0) continue;
	    if (sz & PCI_BASE_ADDRESS_SPACE) {
		m = 0;
		sz &= PCI_BASE_ADDRESS_IO_MASK;
	    } else {
		m = (sz & PCI_BASE_ADDRESS_MEM_PREFETCH) ? 2 : 1;
		sz &= PCI_BASE_ADDRESS_MEM_MASK;
	    }
	    sz = FIND_FIRST_BIT(sz);
	    num[m] += sz; mask[m] |= sz;
	    c[i].size[j] = sz | m;
	}
    }

    /* Allocate system resources */
    name = "cb_enabler";
    s->io[0].NumPorts = num[0];
    s->io[0].BasePort = 0;
    if (num[0]) {
	if (find_io_region(&s->io[0].BasePort, num[0], name) != 0)
	    goto failed;
	base[0] = s->io[0].BasePort;
    }
    s->win[0].size = num[1];
    s->win[0].base = 0;
    if (num[1]) {
	if (find_mem_region(&s->win[0].base, num[1], name) != 0)
	    goto failed;
	base[1] = s->win[0].base;
    }
    s->win[1].size = num[2];
    s->win[1].base = 0;
    if (num[2]) {
	if (find_mem_region(&s->win[1].base, num[2], name) != 0)
	    goto failed;
	base[2] = s->win[1].base;
    }
    
    /* Set up base address registers */
    while (mask[0] | mask[1] | mask[2]) {
	num[0] = FIND_FIRST_BIT(mask[0]); mask[0] -= num[0];
	num[1] = FIND_FIRST_BIT(mask[1]); mask[1] -= num[1];
	num[2] = FIND_FIRST_BIT(mask[2]); mask[2] -= num[2];
	for (i = 0; i < fn; i++) {
	    for (j = 0; j < 6; j++) {
		sz = c[i].size[j];
		m = sz & 3; sz &= ~3;
		if (sz && (sz == num[m])) {
		    DEBUG(1, " fn %d bar %d: %s base 0x%x, sz 0x%x\n",
			  i, j, (m) ? "mem" : "io", base[m], sz);
		    c[i].base[j] = base[m];
		    base[m] += sz;
		}
	    }
	}
    }
    
    /* Allocate interrupt if needed */
    s->irq.AssignedIRQ = irq = 0; ret = -1;
    for (i = 0; i < fn; i++) {
	pci_readb(bus, i, PCI_INTERRUPT_PIN, &j);
	if (j == 0) continue;
	if (irq == 0) {
	    for (try = 0; try < 2; try++) {
		for (irq = 0; irq < 16; irq++)
		    if ((s->cap.irq_mask >> irq) & 1) {
			ret = try_irq(IRQ_TYPE_EXCLUSIVE, irq, try);
			if (ret == 0) break;
		    }
		if (ret == 0) break;
	    }
	    if (ret != 0) goto failed;
	    s->irq.AssignedIRQ = irq;
	}
    }
    c[0].irq = irq;
    
    return CS_SUCCESS;

failed:
    cb_release(s);
    return CS_OUT_OF_RESOURCE;
}

/*======================================================================

    cb_release() releases all the system resources (IO and memory
    space, and interrupt) committed for a Cardbus card by a prior call
    to cb_config().

    It is called from the ReleaseIO() service.
    
======================================================================*/

void cb_release(socket_info_t *s)
{
    DEBUG(1, "cb_release(bus %d)\n", s->cap.cardbus);
    
    if (s->win[0].size > 0)
	release_mem_region(s->win[0].base, s->win[0].size);
    if (s->win[1].size > 0)
	release_mem_region(s->win[1].base, s->win[1].size);
    if (s->io[0].NumPorts > 0)
	release_region(s->io[0].BasePort, s->io[0].NumPorts);
    s->io[0].NumPorts = 0;
    undo_irq(IRQ_TYPE_EXCLUSIVE, s->cb_config[0].irq);
    kfree(s->cb_config);
    s->cb_config = NULL;
}
