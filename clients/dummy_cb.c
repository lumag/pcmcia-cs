/*======================================================================

    A dummy driver for CardBus devices

    dummy_cb.c 1.5 2003/09/15 04:08:59

    The initial developer of the original code is David A. Hinds
    <dahinds@users.sourceforge.net>.  Portions created by David A. Hinds
    are Copyright (C) 2003 David A. Hinds.  All Rights Reserved.
  
======================================================================*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/tty.h>
#include <linux/pci.h>

#include <pcmcia/driver_ops.h>

/*====================================================================*/

/* Module parameters */

MODULE_AUTHOR("David Hinds <dahinds@users.sourceforge.net>");
MODULE_DESCRIPTION("Dummy CardBus driver");
MODULE_LICENSE("Dual MPL/GPL");

#define INT_MODULE_PARM(n, v) static int n = v; MODULE_PARM(n, "i")

#ifdef PCMCIA_DEBUG
INT_MODULE_PARM(pc_debug, PCMCIA_DEBUG);
#define DEBUG(n, args...) if (pc_debug>(n)) printk(KERN_DEBUG args)
static char *version =
"dummy_cb.c 1.5 2003/09/15 04:08:59 (David Hinds)";
#else
#define DEBUG(n, args...)
#endif

/*======================================================================

    dummy_attach() creates a device "instance", and dummy_detach()
    frees an instance.

======================================================================*/

static dev_node_t *dummy_attach(dev_locator_t *loc)
{
    u_char bus, devfn;
    static int instance = 0;
    dev_node_t *node;

    if (loc->bus != LOC_PCI) return NULL;
    bus = loc->b.pci.bus; devfn = loc->b.pci.devfn;
    printk(KERN_INFO "dummy_attach(device %02x:%02x.%d)\n",
	   bus, PCI_SLOT(devfn), PCI_FUNC(devfn));
    node = kmalloc(sizeof(dev_node_t), GFP_KERNEL);
    sprintf(node->dev_name, "dummy%d", instance++);
    node->major = node->minor = 0;
    node->next = NULL;
    MOD_INC_USE_COUNT;
    return node;
}

static void dummy_detach(dev_node_t *node)
{
    DEBUG(0, "dummy_detach(%s)\n", node->dev_name);
    kfree(node);
    MOD_DEC_USE_COUNT;
}

/*====================================================================*/

struct driver_operations dummy_ops = {
    "dummy_cb", dummy_attach, NULL, NULL, dummy_detach
};

static int __init init_dummy_cb(void)
{
    DEBUG(0, "%s\n", version);
    register_driver(&dummy_ops);
    return 0;
}

static void __exit exit_dummy_cb(void)
{
    DEBUG(0, "dummy_cb: unloading\n");
    unregister_driver(&dummy_ops);
}

module_init(init_dummy_cb);
module_exit(exit_dummy_cb);
