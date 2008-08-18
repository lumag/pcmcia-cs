#ifndef _COMPAT_PARPORT_PC_H
#define _COMPAT_PARPORT_PC_H

#include_next <linux/parport_pc.h>

/* Be sure that this is only ever included by parport_cs.c! */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,3,6))

#define PARPORT_MODE_TRISTATE	PARPORT_MODE_PCPS2
#define PARPORT_MODE_EPP	PARPORT_MODE_PCEPP
#define PARPORT_MODE_ECP	PARPORT_MODE_PCECP

extern struct parport_operations parport_pc_ops;

static void inc_use_count(void)
{
    MOD_INC_USE_COUNT;
    parport_pc_ops.inc_use_count();
}

static void dec_use_count(void)
{
    MOD_DEC_USE_COUNT;
    parport_pc_ops.dec_use_count();
}

static inline struct parport *
parport_pc_probe_port(int io1, int io2, int irq, int dma, void *ops)
{
    struct parport *p;
    int i;
    static struct { u_int flag; char *name; } mode[] = {
	{ PARPORT_MODE_TRISTATE, "TRISTATE" },
	{ PARPORT_MODE_EPP, "EPP" },
	{ PARPORT_MODE_ECP, "ECP" },
	{ PARPORT_MODE_PCECPEPP, "ECPEPP" },
	{ PARPORT_MODE_PCECPPS2, "ECPPS2" }
    };
    static struct parport_operations parport_cs_ops;

    parport_cs_ops = parport_pc_ops;
    parport_cs_ops.inc_use_count = &inc_use_count;
    parport_cs_ops.dec_use_count = &dec_use_count;

    p = parport_register_port(io1, irq, dma, &parport_cs_ops);
    if (p == NULL)
	return p;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,8))
    p->private_data = kmalloc(sizeof(struct parport_pc_private),
			      GFP_KERNEL);
    ((struct parport_pc_private *)(p->private_data))->ctr = 0x0c;
#endif
    parport_proc_register(p);
    p->flags |= PARPORT_FLAG_COMA;
    parport_pc_write_econtrol(p, 0x00);
    parport_pc_write_control(p, 0x0c);
    parport_pc_write_data(p, 0x00);
    printk(KERN_INFO "%s: PC-style PCMCIA at %#x", p->name, io1);
    if (io2)
	printk(" & %#x", io2);
    printk(", irq %u [SPP", irq);
    for (i = 0; i < 5; i++)
	if (p->modes & mode[i].flag) printk(",%s", mode[i].name);
    printk("]\n");
    return p;
}

static void parport_pc_unregister_port(struct parport *p)
{
    if (!(p->flags & PARPORT_FLAG_COMA))
	parport_quiesce(p);
    parport_proc_unregister(p);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,8))
    kfree(p->private_data);
#endif
    parport_unregister_port(p);
}

#endif

#endif /* _COMPAT_PARPORT_PC_H */
