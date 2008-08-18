/*======================================================================

    A driver for PCMCIA serial devices

    serial_cb.c 1.1 1999/02/13 06:47:03

    The contents of this file are subject to the Mozilla Public
    License Version 1.0 (the "License"); you may not use this file
    except in compliance with the License. You may obtain a copy of
    the License at http://www.mozilla.org/MPL/

    Software distributed under the License is distributed on an "AS
    IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
    implied. See the License for the specific language governing
    rights and limitations under the License.

    The initial developer of the original code is David A. Hinds
    <dhinds@hyper.stanford.edu>.  Portions created by David A. Hinds
    are Copyright (C) 1998 David A. Hinds.  All Rights Reserved.
    
======================================================================*/

#include <pcmcia/config.h>
#include <pcmcia/k_compat.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/malloc.h>
#include <linux/string.h>
#include <linux/tty.h>
#include <linux/serial.h>
#include <linux/major.h>

#include <pcmcia/driver_ops.h>

#ifdef PCMCIA_DEBUG
static int pc_debug = PCMCIA_DEBUG;
MODULE_PARM(pc_debug, "i");
#define DEBUG(n, args...) if (pc_debug>(n)) printk(KERN_DEBUG args)
static char *version =
"serial_cb.c 1.1 1999/02/13 06:47:03 (David Hinds)";
#else
#define DEBUG(n, args...)
#endif

/*======================================================================

    Card-specific configuration hacks

    Donald Becker's code to configure the Jack of Spades card

======================================================================*/

static void device_setup(u_char bus, u_char devfn, u_int ioaddr)
{
    u_int sub;
    
    pcibios_read_config_dword(bus, devfn, PCI_SUBSYSTEM_ID, &sub);
    if (sub == 0x800713a2) {
	DEBUG(0, "  83c175 NVCTL_m = 0x%4.4x.\n", inl(ioaddr+0x80));
	outl(0x4C00, ioaddr + 0x80);
	outl(0x4C80, ioaddr + 0x80);
	DEBUG(0, "  modem registers are %2.2x %2.2x %2.2x "
	      "%2.2x %2.2x %2.2x %2.2x %2.2x  %2.2x.\n",
	      inb(ioaddr + 0), inb(ioaddr + 1), inb(ioaddr + 2),
	      inb(ioaddr + 3), inb(ioaddr + 4), inb(ioaddr + 5),
	      inb(ioaddr + 6), inb(ioaddr + 7), inb(ioaddr + 8));
    }
}

/*======================================================================

    serial_attach() creates a serial device "instance" and registers
    it with the kernel serial driver, and serial_detach() unregisters
    an instance.

======================================================================*/

static dev_node_t *serial_attach(dev_locator_t *loc)
{
    u_int io;
    u_char bus, devfn, irq;
    int line;
    struct serial_struct serial;
    
    if (loc->bus != LOC_PCI) return NULL;
    bus = loc->b.pci.bus; devfn = loc->b.pci.devfn;
    printk(KERN_INFO "serial_attach(bus %d, fn %d)\n", bus, devfn);
    pcibios_read_config_dword(bus, devfn, PCI_BASE_ADDRESS_0, &io);
    pcibios_read_config_byte(bus, devfn, PCI_INTERRUPT_LINE, &irq);
    device_setup(bus, devfn, io);
    serial.port = io; serial.irq = irq;
    serial.flags = ASYNC_SKIP_TEST | ASYNC_SHARE_IRQ;
    line = register_serial(&serial);
    if (line < 0) {
	printk(KERN_NOTICE "serial_cb: register_serial() at 0x%04x, "
	       "irq %d failed\n", serial.port, serial.irq);
	return NULL;
    } else {
	dev_node_t *node = kmalloc(sizeof(dev_node_t), GFP_KERNEL);
	sprintf(node->dev_name, "ttyS%d", line);
	node->major = TTY_MAJOR; node->minor = 0x40 + line;
	node->next = NULL;
	MOD_INC_USE_COUNT;
	return node;
    }
}

static void serial_detach(dev_node_t *node)
{
    DEBUG(0, "serial_detach(line %d)\n", node->minor - 0x40);
    unregister_serial(node->minor - 0x40);
    kfree(node);
    MOD_DEC_USE_COUNT;
}

/*====================================================================*/

struct driver_operations serial_ops = {
    "serial_cb", serial_attach, NULL, NULL, serial_detach
};

int init_module(void)
{
    DEBUG(0, "%s\n", version);
    register_driver(&serial_ops);
    return 0;
}

void cleanup_module(void)
{
    DEBUG(0, "serial_cs: unloading\n");
    unregister_driver(&serial_ops);
}
