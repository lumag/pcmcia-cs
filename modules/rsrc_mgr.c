/*======================================================================

    Resource management routines

    rsrc_mgr.c 1.29 1998/01/07 04:44:34 (David Hinds)
    
======================================================================*/

#include <pcmcia/config.h>
#define __NO_VERSION__
#include <pcmcia/k_compat.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/malloc.h>
#include <linux/ioport.h>
#include <linux/timer.h>
#include <asm/io.h>

#include <pcmcia/cs_types.h>
#include <pcmcia/ss.h>
#include <pcmcia/cs.h>
#include <pcmcia/bulkmem.h>
#include <pcmcia/cistpl.h>
#include "cs_internal.h"
#include "rsrc_mgr.h"

/*====================================================================*/

typedef struct memory_entry_t {
    u_long			base, num;
    char			*name;
    struct memory_entry_t	*next;
} memory_entry_t;

/* An ordered linked list of allocated memory blocks */
static memory_entry_t memlist = { 0, 0, NULL, NULL };

typedef struct resource_entry_t {
    u_long			base, num;
    struct resource_entry_t	*next;
} resource_entry_t;

/* Memory resource database */
static resource_entry_t mem_db = { 0, 0, &mem_db };

/* IO port resource database */
static resource_entry_t io_db = { 0, 0, &io_db };

typedef struct irq_info_t {
    u_int			Attributes;
    int				time_share, dyn_share;
    struct socket_info_t	*Socket;
} irq_info_t;

/* Table of IRQ assignments */
static irq_info_t irq_table[16] = { { 0, 0, 0 }, /* etc */ };

/*====================================================================*/

/* Parameters that can be set with 'insmod' */

/* Should we probe resources for conflicts? */
static int probe_io = 1;
static int probe_mem = 1;
static int mem_limit = 0x10000;

MODULE_PARM(probe_io, "i");
MODULE_PARM(probe_mem, "i");
MODULE_PARM(mem_limit, "i");

/*======================================================================

    These functions manage the database of allocated memory blocks.
    
======================================================================*/

static memory_entry_t *find_gap(memory_entry_t *root,
				memory_entry_t *entry)
{
    u_long flags;
    memory_entry_t *p;
    
    if (entry->base > entry->base+entry->num-1)
	return NULL;
    save_flags(flags);
    cli();
    for (p = root; ; p = p->next) {
	if ((p != root) && (p->base+p->num-1 >= entry->base)) {
	    p = NULL;
	    break;
	}
	if ((p->next == NULL) ||
	    (p->next->base > entry->base+entry->num-1))
	    break;
    }
    restore_flags(flags);
    return p;
}

int check_mem_region(u_long base, u_long num)
{
    memory_entry_t entry;
    entry.base = base;
    entry.num = num;
    return (find_gap(&memlist, &entry) == NULL) ? -EBUSY : 0;
}

int register_mem_region(u_long base, u_long num, char *name)
{
    u_long flags;
    memory_entry_t *p, *entry;

    entry = kmalloc(sizeof(memory_entry_t), GFP_ATOMIC);
    entry->base = base;
    entry->num = num;
    entry->name = name;

    save_flags(flags);
    cli();
    p = find_gap(&memlist, entry);
    if (p == NULL) {
	restore_flags(flags);
	kfree_s(entry, sizeof(memory_entry_t));
	return -EBUSY;
    }
    entry->next = p->next;
    p->next = entry;
    restore_flags(flags);
    return 0;
}

int release_mem_region(u_long base, u_long num)
{
    u_long flags;
    memory_entry_t *p, *q;

    save_flags(flags);
    cli();
    for (p = &memlist; ; p = q) {
	q = p->next;
	if (q == NULL) break;
	if ((q->base == base) && (q->num == num)) {
	    p->next = q->next;
	    kfree_s(q, sizeof(memory_entry_t));
	    restore_flags(flags);
	    return 0;
	}
    }
    restore_flags(flags);
    return -EINVAL;
}

/*======================================================================

    These manage the internal databases of available resources
    
======================================================================*/

static int add_interval(resource_entry_t *map, u_long base, u_long num)
{
    resource_entry_t *p, *q;

    for (p = map; ; p = p->next) {
	if ((p != map) && (p->base+p->num-1 >= base))
	    return -1;
	if ((p->next == map) || (p->next->base > base+num-1))
	    break;
    }
    q = kmalloc(sizeof(resource_entry_t), GFP_KERNEL);
    q->base = base; q->num = num;
    q->next = p->next; p->next = q;
    return 0;
} /* add_interval */

/*====================================================================*/

static int sub_interval(resource_entry_t *map, u_long base, u_long num)
{
    resource_entry_t *p, *q;

    for (p = map; ; p = q) {
	q = p->next;
	if (q == map)
	    break;
	if ((q->base+q->num > base) && (base+num > q->base)) {
	    if (q->base >= base) {
		if (q->base+q->num <= base+num) {
		    /* Delete whole block */
		    p->next = q->next;
		    kfree_s(q, sizeof(resource_entry_t));
		    /* don't advance the pointer yet */
		    q = p;
		} else {
		    /* Cut off bit from the front */
		    q->num = q->base + q->num - base - num;
		    q->base = base + num;
		}
	    } else if (q->base+q->num <= base+num) {
		/* Cut off bit from the end */
		q->num = base - q->base;
	    } else {
		/* Split the block into two pieces */
		p = kmalloc(sizeof(resource_entry_t), GFP_KERNEL);
		p->base = base+num;
		p->num = q->base+q->num - p->base;
		q->num = base - q->base;
		p->next = q->next ; q->next = p;
	    }
	}
    }
    return 0;
} /* sub_interval */

/*======================================================================

    These routines examine a region of IO or memory addresses to
    determine what ranges might be genuinely available.
    
======================================================================*/

static void do_io_probe(ioaddr_t base, ioaddr_t num)
{
    
    ioaddr_t i, j, bad, any;
    u_char *b, hole;

    printk(KERN_INFO "cs: IO port probe 0x%04x-0x%04x:",
	   base, base+num-1);
    
    /* First, what does a floating port look like? */
    b = kmalloc(256, GFP_KERNEL);
    memset(b, 0, 256);
    for (i = base; i < base+num; i += 8) {
	if (check_region(i, 8) != 0)
	    continue;
	hole = inb_p(i);
	for (j = 1; j < 8; j++)
	    if (inb_p(i+j) != hole) break;
	if (j == 8)
	    b[hole]++;
    }
    hole = 0;
    for (i = 0; i < 256; i++)
	if (b[i] > b[hole]) hole = i;
    kfree(b);

    bad = any = 0;
    for (i = base; i < base+num; i += 8) {
	if (check_region(i, 8) != 0)
	    continue;
	for (j = 0; j < 8; j++)
	    if (inb_p(i+j) != hole) break;
	if (j < 8) {
	    if (!any)
		printk(" excluding");
	    if (!bad)
		bad = any = i;
	} else {
	    if (bad) {
		sub_interval(&io_db, bad, i-bad);
		printk(" %#04x-%#04x", bad, i-1);
		bad = 0;
	    }
	}
    }
    if (bad) {
	if ((num > 16) && (bad == base) && (i == base+num)) {
	    printk(" nothing: probe failed.\n");
	    return;
	} else {
	    sub_interval(&io_db, bad, i-bad);
	    printk(" %#04x-%#04x", bad, i-1);
	}
    }
    
    printk(any ? "\n" : " clean.\n");
}

/*======================================================================

    The memory probe.  If the memory list includes a 64K-aligned block
    below 1MB, we probe in 64K chunks, and as soon as we accumulate at
    least mem_limit free space, we quit.
    
======================================================================*/

#define STEP 8192

static int do_mem_probe(u_long base, u_long num, int pass,
			int (*is_valid)(u_long))
{
    u_long i, j, bad;

    printk(KERN_INFO "cs: memory probe 0x%06lx-0x%06lx:",
	   base, base+num-1);
    if (pass != 0) release_mem_region(base, num);
    bad = 0;
    for (i = base; i < base+num; i = j + STEP) {
	for (j = i; j < base+num; j += STEP)
	    if ((check_mem_region(j, STEP)) == 0 && is_valid(j))
		break;
	if (i != j) {
	    if (!bad) printk(" excluding");
	    printk(" %#05lx-%#05lx", i, j-1);
	    register_mem_region(i, j-i, "reserved");
	    bad += j-i;
	}
    }
    printk(bad ? "\n" : " clean.\n");
    return (num - bad);
}

void validate_mem(int (*is_valid)(u_long), int (*do_cksum)(u_long))
{
    resource_entry_t *m;
    static u_char order[] = { 0xd0, 0xe0, 0xc0, 0xf0 };
    u_long b, ok = 0;
    int i, pass;
    
    if (!probe_mem)
	return;
    /* We do up to two passes through the list */
    for (pass = 0; pass < 2; pass++) {
	for (m = mem_db.next; m != &mem_db; m = m->next) {
	    /* Only probe < 1 MB */
	    if (m->base >= 0x100000) continue;
	    if ((m->base | m->num) & 0xffff) {
		ok += do_mem_probe(m->base, m->num, pass, is_valid);
		continue;
	    }
	    /* Special probe for 64K-aligned block */
	    for (i = 0; i < 4; i++) {
		b = order[i] << 12;
		if ((b >= m->base) && (b+0x10000 <= m->base+m->num)) {
		    if (ok >= mem_limit)
			register_mem_region(b, 0x10000, "skipped");
		    else
			ok += do_mem_probe(b, 0x10000, pass, is_valid);
		}
	    }
	}
	if (ok) return;
	is_valid = do_cksum;
    }
}

/*======================================================================

    These find ranges of I/O ports or memory addresses that are not
    currently allocated by other devices.
    
======================================================================*/

int find_io_region(ioaddr_t *base, ioaddr_t num, char *name)
{
    ioaddr_t align;
    resource_entry_t *m;
    
    if (*base != 0) {
	for (m = io_db.next; m != &io_db; m = m->next) {
	    if ((*base >= m->base) && (*base+num <= m->base+m->num)) {
		if (check_region(*base, num))
		    return -1;
		else {
		    request_region(*base, num, name);
		    return 0;
		}
	    }
	}
	return -1;
    }
    
    for (align = 1; align < num; align *= 2) ;
    for (m = io_db.next; m != &io_db; m = m->next) {
	for (*base = (m->base + align - 1) & (~(align-1));
	     *base+align <= m->base + m->num;
	     *base += align)
	    if (check_region(*base, num) == 0) {
		request_region(*base, num, name);
		return 0;
	    }
    }
    return -1;
} /* find_io_region */

int find_mem_region(u_long *base, u_long num, char *name)
{
    u_long align;
    resource_entry_t *m;
    
    if (*base != 0) {
	for (m = mem_db.next; m != &mem_db; m = m->next) {
	    if ((*base >= m->base) && (*base+num <= m->base+m->num))
		return register_mem_region(*base, num, name);
	}
	return -1;
    }
    
    for (align = 4096; align < num; align *= 2) ;
    for (m = mem_db.next; m != &mem_db; m = m->next) {
	for (*base = (m->base + align - 1) & (~(align-1));
	     *base+align <= m->base+m->num;
	     *base += align)
	    if (register_mem_region(*base, num, name) == 0)
		return 0;
    }
    return -1;
} /* find_mem_region */

/*======================================================================

    This checks to see if an interrupt is available, with support
    for interrupt sharing.  We don't support reserving interrupts
    yet.  If the interrupt is available, we allocate it.
    
======================================================================*/

static void fake_irq IRQ(int i, void *d, struct pt_regs *r) { }

int try_irq(u_int Attributes, int irq, int specific)
{
    irq_info_t *info = &irq_table[irq];
    if (info->Attributes & RES_ALLOCATED) {
	switch (Attributes & IRQ_TYPE) {
	case IRQ_TYPE_EXCLUSIVE:
	    return CS_IN_USE;
	case IRQ_TYPE_TIME:
	    if ((info->Attributes & RES_IRQ_TYPE)
		!= RES_IRQ_TYPE_TIME)
		return CS_IN_USE;
	    if (Attributes & IRQ_FIRST_SHARED)
		return CS_BAD_ATTRIBUTE;
	    info->Attributes |= RES_IRQ_TYPE_TIME | RES_ALLOCATED;
	    info->time_share++;
	    break;
	case IRQ_TYPE_DYNAMIC_SHARING:
	    if ((info->Attributes & RES_IRQ_TYPE)
		!= RES_IRQ_TYPE_DYNAMIC)
		return CS_IN_USE;
	    if (Attributes & IRQ_FIRST_SHARED)
		return CS_BAD_ATTRIBUTE;
	    info->Attributes |= RES_IRQ_TYPE_DYNAMIC | RES_ALLOCATED;
	    info->dyn_share++;
	    break;
	}
    } else {
	if ((info->Attributes & RES_RESERVED) && !specific)
	    return CS_IN_USE;
	if (REQUEST_IRQ(irq, fake_irq, 0, "bogus", NULL) != 0)
	    return CS_IN_USE;
	FREE_IRQ(irq, NULL);
	switch (Attributes & IRQ_TYPE) {
	case IRQ_TYPE_EXCLUSIVE:
	    info->Attributes |= RES_ALLOCATED;
	    break;
	case IRQ_TYPE_TIME:
	    if (!(Attributes & IRQ_FIRST_SHARED))
		return CS_BAD_ATTRIBUTE;
	    info->Attributes |= RES_IRQ_TYPE_TIME | RES_ALLOCATED;
	    info->time_share = 1;
	    break;
	case IRQ_TYPE_DYNAMIC_SHARING:
	    if (!(Attributes & IRQ_FIRST_SHARED))
		return CS_BAD_ATTRIBUTE;
	    info->Attributes |= RES_IRQ_TYPE_DYNAMIC | RES_ALLOCATED;
	    info->dyn_share = 1;
	    break;
	}
    }
    return 0;
} /* try_irq */

/*====================================================================*/

void undo_irq(u_int Attributes, int irq)
{
    irq_info_t *info;

    info = &irq_table[irq];
    switch (Attributes & IRQ_TYPE) {
    case IRQ_TYPE_EXCLUSIVE:
	info->Attributes &= RES_RESERVED;
	break;
    case IRQ_TYPE_TIME:
	info->time_share--;
	if (info->time_share == 0)
	    info->Attributes &= RES_RESERVED;
	break;
    case IRQ_TYPE_DYNAMIC_SHARING:
	info->dyn_share--;
	if (info->dyn_share == 0)
	    info->Attributes &= RES_RESERVED;
	break;
    }
}
    
/*======================================================================

    The various adjust_* calls form the external interface to the
    resource database.
    
======================================================================*/

static int adjust_memory(adjust_t *adj)
{
    u_long base, num;
    int i;

    base = (u_long)adj->resource.memory.Base;
    num = adj->resource.memory.Size;
    if ((num == 0) || (base+num-1 < base))
	return CS_BAD_SIZE;

    switch (adj->Action) {
    case ADD_MANAGED_RESOURCE:
	if (add_interval(&mem_db, base, num) != 0)
	    return CS_IN_USE;
	break;
    case REMOVE_MANAGED_RESOURCE:
	sub_interval(&mem_db, base, num);
	for (i = 0; i < sockets; i++) {
	    socket_info_t *s = socket_table[i];
	    release_mem_region(s->cis_mem.sys_start, 0x1000);
	    s->cis_mem.sys_start = 0;
	}
	break;
    default:
	return CS_UNSUPPORTED_FUNCTION;
	break;
    }
    
    return CS_SUCCESS;
} /* adjust_mem */

/*====================================================================*/

static int adjust_io(adjust_t *adj)
{
    int base, num;
    
    base = adj->resource.io.BasePort;
    num = adj->resource.io.NumPorts;
    if ((base < 0) || (base > 0xffff))
	return CS_BAD_BASE;
    if ((num <= 0) || (base+num > 0x10000) || (base+num <= base))
	return CS_BAD_SIZE;

    switch (adj->Action) {
    case ADD_MANAGED_RESOURCE:
	if (add_interval(&io_db, base, num) != 0)
	    return CS_IN_USE;
	if (probe_io)
	    do_io_probe(base, num);
	break;
    case REMOVE_MANAGED_RESOURCE:
	sub_interval(&io_db, base, num);
	break;
    default:
	return CS_UNSUPPORTED_FUNCTION;
	break;
    }

    return CS_SUCCESS;
} /* adjust_io */

/*====================================================================*/

static int adjust_irq(adjust_t *adj)
{
    int irq;
    irq_info_t *info;
    
    irq = adj->resource.irq.IRQ;
    if ((irq < 0) || (irq > 15))
	return CS_BAD_IRQ;
    info = &irq_table[irq];
    
    switch (adj->Action) {
    case ADD_MANAGED_RESOURCE:
	if (info->Attributes & RES_REMOVED)
	    info->Attributes &= ~(RES_REMOVED|RES_ALLOCATED);
	else
	    if (adj->Attributes & RES_ALLOCATED)
		return CS_IN_USE;
	if (adj->Attributes & RES_RESERVED)
	    info->Attributes |= RES_RESERVED;
	else
	    info->Attributes &= ~RES_RESERVED;
	break;
    case REMOVE_MANAGED_RESOURCE:
	if (info->Attributes & RES_REMOVED)
	    return 0;
	if (info->Attributes & RES_ALLOCATED)
	    return CS_IN_USE;
	info->Attributes |= RES_ALLOCATED|RES_REMOVED;
	info->Attributes &= ~RES_RESERVED;
	break;
    default:
	return CS_UNSUPPORTED_FUNCTION;
	break;
    }
    
    return 0;
} /* adjust_irq */

/*====================================================================*/

int adjust_resource_info(client_handle_t handle, adjust_t *adj)
{
    if (CHECK_HANDLE(handle))
	return CS_BAD_HANDLE;
    
    switch (adj->Resource) {
    case RES_MEMORY_RANGE:
	return adjust_memory(adj);
	break;
    case RES_IO_RANGE:
	return adjust_io(adj);
	break;
    case RES_IRQ:
	return adjust_irq(adj);
	break;
    }
    return CS_UNSUPPORTED_FUNCTION;
} /* adjust_resource_info */

/*====================================================================*/

void release_resource_db(void)
{
    resource_entry_t *p, *q;
    
    for (p = mem_db.next; p != &mem_db; p = q) {
	q = p->next;
	kfree_s(p, sizeof(resource_entry_t));
    }
    for (p = io_db.next; p != &io_db; p = q) {
	q = p->next;
	kfree_s(p, sizeof(resource_entry_t));
    }
}
