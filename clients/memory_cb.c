/*======================================================================

    A direct memory interface driver for CardBus cards

    memory_cb.c 1.4 1998/11/05 07:13:23

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
#include <linux/timer.h>
#include <linux/ioport.h>
#include <linux/major.h>

#include <pcmcia/driver_ops.h>
#include <pcmcia/mem_op.h>

#ifdef PCMCIA_DEBUG
static int pc_debug = PCMCIA_DEBUG;
MODULE_PARM(pc_debug, "i");
#define DEBUG(n, args...) if (pc_debug>(n)) printk(KERN_DEBUG args)
static char *version =
"memory_cb.c 1.4 1998/11/05 07:13:23 (David Hinds)";
#else
#define DEBUG(n, args...)
#endif

/*====================================================================*/

/* Parameters that can be set with 'insmod' */

/*====================================================================*/

typedef struct memory_dev_t {
    dev_node_t	node;
    u_char	bus, devfn;
    u_int	open, stopped;
    u_int	base[8];
    u_int	size[8];
    u_char	*virt[8];
} memory_dev_t;

#define MAX_DEV 8
static memory_dev_t *dev_table[MAX_DEV] = { 0 };

static int major_dev = 0;

/*====================================================================*/

#define FIND_FIRST_BIT(n)	((n) - ((n) & ((n)-1)))
#define CB_BAR(n)		(PCI_BASE_ADDRESS_0+(4*(n)))
#define CB_ROM_BASE		0x0030

static dev_node_t *memory_attach(dev_locator_t *loc)
{
    u_char bus, devfn, cmd;
    memory_dev_t *dev;
    int i, br;
    
    if (loc->bus != LOC_PCI) return NULL;
    bus = loc->b.pci.bus; devfn = loc->b.pci.devfn;
    printk(KERN_INFO "memory_attach(bus %d, function %d)\n",
	   bus, devfn);

    for (i = 0; i < MAX_DEV; i++)
	if (dev_table[i] == NULL) break;
    if (i == MAX_DEV) return NULL;
    dev_table[i] = dev = kmalloc(sizeof(memory_dev_t), GFP_KERNEL);
    memset(dev, 0, sizeof(memory_dev_t));
    dev->bus = bus; dev->devfn = devfn;
    sprintf(dev->node.dev_name, "cbmem%d", i);
    dev->node.major = major_dev;
    dev->node.minor = i<<3;
    
    dev->size[0] = 0x100;
    printk(KERN_INFO "memory_cb: cbmem%d: 0 [256 b]", i);
    pcibios_read_config_byte(bus, devfn, PCI_COMMAND, &cmd);
    pcibios_write_config_byte(bus, devfn, PCI_COMMAND, 0);
    for (i = 1; i < 8; i++) {
	br = (i == 7) ? CB_ROM_BASE : CB_BAR(i-1);
	pcibios_read_config_dword(bus, devfn, br, &dev->base[i]);
	pcibios_write_config_dword(bus, devfn, br, 0xffffffff);
	pcibios_read_config_dword(bus, devfn, br, &dev->size[i]);
	pcibios_write_config_dword(bus, devfn, br, dev->base[i]);
	dev->size[i] &= PCI_BASE_ADDRESS_MEM_MASK;
	dev->size[i] = FIND_FIRST_BIT(dev->size[i]);
	if (dev->size[i] == 0) continue;
	if ((i == 7) || ((dev->base[i] & PCI_BASE_ADDRESS_SPACE) == 0)) {
	    dev->base[i] &= PCI_BASE_ADDRESS_MEM_MASK;
	    dev->virt[i] = ioremap(dev->base[i], dev->size[i]);
	} else {
	    dev->base[i] &= PCI_BASE_ADDRESS_IO_MASK;
	}
	if (dev->size[i] & 0x3ff)
	    printk(", %d [%d b]", i, dev->size[i]);
	else
	    printk(", %d [%d kb]", i, dev->size[i]>>10);
    }
    printk("\n");
    pcibios_write_config_byte(bus, devfn, PCI_COMMAND, cmd);
    MOD_INC_USE_COUNT;
    return &dev->node;
}

static void memory_detach(dev_node_t *node)
{
    memory_dev_t *dev = (memory_dev_t *)node;
    int i;

    dev->stopped = 1;
    if (dev->open) return;
    dev_table[node->minor >> 3] = NULL;
    for (i = 0; i < 8; i++)
	if (dev->virt[i] != NULL) iounmap(dev->virt[i]);
    kfree(dev);
    MOD_DEC_USE_COUNT;
}

/*====================================================================*/

static int memory_open(struct inode *inode, struct file *file)
{
    int minor = MINOR(F_INODE(file)->i_rdev);
    memory_dev_t *dev = dev_table[minor>>3];

    DEBUG(0, "memory_open(%d)\n", minor);
    if ((dev == NULL) || (dev->stopped) || (dev->size[minor&7] == 0))
	return -ENODEV;
    dev->open++;
    MOD_INC_USE_COUNT;
    return 0;
}

static FS_RELEASE_T memory_close(struct inode *inode,
				 struct file *file)
{
    int minor = MINOR(F_INODE(file)->i_rdev);
    memory_dev_t *dev = dev_table[minor>>3];
    
    DEBUG(0, "memory_close(%d)\n", minor);
    dev->open--;
    MOD_DEC_USE_COUNT;
    if (dev->stopped && (dev->open == 0))
	memory_detach((dev_node_t *)dev);
    return (FS_RELEASE_T)0;
}

static FS_SIZE_T memory_read FOPS(struct inode *inode,
				  struct file *file, char *buf,
				  U_FS_SIZE_T count, loff_t *ppos)
{
    int minor = MINOR(F_INODE(file)->i_rdev);
    memory_dev_t *dev = dev_table[minor>>3];
    int space = minor & 7;
    U_FS_SIZE_T i, pos = FPOS;
    
    DEBUG(2, "memory_read(%d, 0x%lx, 0x%lx)\n", minor,
	  (u_long)pos, (u_long)count);

    if (dev->stopped)
	return -ENODEV;
    if (pos >= dev->size[space])
	return 0;
    if (count > dev->size[space] - pos)
	count = dev->size[space] - pos;

    if (space == 0) {
	for (i = 0; i < count; i++, pos++, buf++)
	    pcibios_read_config_byte(dev->bus, dev->devfn, pos, buf);
    } else if (dev->virt[space] != NULL) {
	copy_pc_to_user(buf, dev->virt[space]+pos, count);
    } else {
	for (i = 0; i < count; i++, pos++, buf++)
	    *buf = inb(dev->base[space]+pos);
    }
    FPOS += count;
    return count;
}

static FS_SIZE_T memory_write FOPS(struct inode *inode,
				   struct file *file, CONST char *buf,
				   U_FS_SIZE_T count, loff_t *ppos)
{
    int minor = MINOR(F_INODE(file)->i_rdev);
    memory_dev_t *dev = dev_table[minor>>3];
    int space = minor & 7;
    U_FS_SIZE_T i, pos = FPOS;
    
    DEBUG(2, "memory_read(%d, 0x%lx, 0x%lx)\n", minor,
	  (u_long)pos, (u_long)count);
    
    if (dev->stopped)
	return -ENODEV;
    if (pos >= dev->size[space])
	return 0;
    if (count > dev->size[space] - pos)
	count = dev->size[space] - pos;

    if (space == 0) {
	for (i = 0; i < count; i++, pos++, buf++)
	    pcibios_write_config_byte(dev->bus, dev->devfn, pos, *buf);
    } else if (dev->virt[space] != NULL) {
	copy_user_to_pc(dev->virt[space]+pos, buf, count);
    } else {
	for (i = 0; i < count; i++, pos++, buf++)
	    outb(*buf, dev->base[space]+pos);
    }
    FPOS += count;
    return count;
}

/*====================================================================*/

static struct file_operations memory_fops = {
    NULL,
    memory_read,
    memory_write,
    NULL,
    NULL,
    NULL,
    NULL,
    memory_open,
    NULL_FLUSH
    memory_close,
    NULL
};

struct driver_operations memory_ops = {
    "memory_cb", memory_attach, NULL, NULL, memory_detach
};

int init_module(void) {
    DEBUG(0, "%s\n", version);
    major_dev = register_chrdev(major_dev, "memory_cb", &memory_fops);
    if (major_dev == 0) {
	printk(KERN_NOTICE "memory_cb: unable to grab major "
	       "device number!\n");
	return -1;
    }
    register_driver(&memory_ops);
    return 0;
}

void cleanup_module(void) {
    DEBUG(0, "memory_cb: unloading\n");
    unregister_driver(&memory_ops);
    if (major_dev != 0)
	unregister_chrdev(major_dev, "memory_cb");
}
