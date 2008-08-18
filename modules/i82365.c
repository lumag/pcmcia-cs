/*======================================================================

    Device driver for Intel 82365 and compatible PC Card controllers,
    and Yenta-compatible PCI-to-CardBus controllers.

    i82365.c 1.289 2000/01/24 06:51:58

    The contents of this file are subject to the Mozilla Public
    License Version 1.1 (the "License"); you may not use this file
    except in compliance with the License. You may obtain a copy of
    the License at http://www.mozilla.org/MPL/

    Software distributed under the License is distributed on an "AS
    IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
    implied. See the License for the specific language governing
    rights and limitations under the License.

    The initial developer of the original code is David A. Hinds
    <dhinds@pcmcia.sourceforge.org>.  Portions created by David A. Hinds
    are Copyright (C) 1999 David A. Hinds.  All Rights Reserved.

    Alternatively, the contents of this file may be used under the
    terms of the GNU Public License version 2 (the "GPL"), in which
    case the provisions of the GPL are applicable instead of the
    above.  If you wish to allow the use of your version of this file
    only under the terms of the GPL and not to allow others to use
    your version of this file under the MPL, indicate your decision
    by deleting the provisions above and replace them with the notice
    and other provisions required by the GPL.  If you do not delete
    the provisions above, a recipient may use your version of this
    file under either the MPL or the GPL.
    
======================================================================*/

#include <pcmcia/config.h>
#include <pcmcia/k_compat.h>

#ifdef __LINUX__
#include <linux/module.h>
#include <linux/init.h>
#include <linux/config.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/malloc.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/bitops.h>
#include <asm/segment.h>
#include <asm/system.h>
#endif

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/ss.h>
#include <pcmcia/cs.h>

/* ISA-bus controllers */
#include "i82365.h"
#include "cirrus.h"
#include "vg468.h"
#include "ricoh.h"
#include "o2micro.h"

/* PCI-bus controllers */
#include "yenta.h"
#include "ti113x.h"
#include "smc34c90.h"
#include "topic.h"

#ifdef PCMCIA_DEBUG
static int pc_debug = PCMCIA_DEBUG;
MODULE_PARM(pc_debug, "i");
#define DEBUG(n, args...) if (pc_debug>(n)) printk(KERN_DEBUG args)
static const char *version =
"i82365.c 1.289 2000/01/24 06:51:58 (David Hinds)";
#else
#define DEBUG(n, args...) do { } while (0)
#endif

#ifdef __BEOS__
typedef int32 irq_ret_t;
#define _request_irq(i, h, f, n) \
    install_io_interrupt_handler(i, h, irq_list+i, 0)
#define _free_irq(i, h) remove_io_interrupt_handler(i, h, irq_list+i)
#define _check_irq(i, f) check_irq(i)
static cs_socket_module_info *cs;
static isa_module_info *isa;
static pci_module_info *pci;
#define RSRC_MGR cs->
#define register_ss_entry	cs->_register_ss_entry
#define unregister_ss_entry	cs->_unregister_ss_entry
#define add_timer		cs->_add_timer
#define del_timer		cs->_del_timer
#else
typedef void irq_ret_t;
#define _request_irq(i, h, f, n) request_irq(i, h, f, n, socket)
#define _free_irq(i, h) free_irq(i, socket)
static void irq_count(int, void *, struct pt_regs *);
static int _check_irq(int irq, int flags)
{
#ifdef CONFIG_PNP_BIOS
    extern int check_pnp_irq(int);
    if ((flags != SA_SHIRQ) && check_pnp_irq(irq))
	return -1;
#endif
    if (request_irq(irq, irq_count, flags, "x", NULL) != 0)
	return -1;
    free_irq(irq, NULL);
    return 0;
}
#endif

MODULE_AUTHOR("David Hinds <dhinds@pcmcia.sourceforge.org>");
MODULE_DESCRIPTION("Intel ExCA/Yenta PCMCIA socket driver");

/*====================================================================*/

/* Parameters that can be set with 'insmod' */

#define INT_MODULE_PARM(n, v) static int n = v; MODULE_PARM(n, "i")

/* General options */
INT_MODULE_PARM(poll_interval, 0);	/* in ticks, 0 means never */
INT_MODULE_PARM(cycle_time, 120);	/* in ns, 120 ns = 8.33 MHz */

/* Cirrus options */
INT_MODULE_PARM(has_dma, -1);
INT_MODULE_PARM(has_led, -1);
INT_MODULE_PARM(has_ring, -1);
INT_MODULE_PARM(dynamic_mode, 0);
INT_MODULE_PARM(freq_bypass, -1);
INT_MODULE_PARM(setup_time, -1);
INT_MODULE_PARM(cmd_time, -1);
INT_MODULE_PARM(recov_time, -1);

#ifdef CONFIG_ISA
INT_MODULE_PARM(i365_base, 0x3e0);	/* IO address for probes */
INT_MODULE_PARM(extra_sockets, 0);	/* Probe at i365_base+2? */
INT_MODULE_PARM(ignore, -1);		/* Ignore this socket # */
INT_MODULE_PARM(do_scan, 1);		/* Probe free interrupts? */
INT_MODULE_PARM(cs_irq, 0);		/* card status irq */
INT_MODULE_PARM(irq_mask, 0xffff);	/* bit map of irq's to use */
static int irq_list[16] = { -1 };
MODULE_PARM(irq_list, "1-16i");
/* Vadem options */
INT_MODULE_PARM(async_clock, -1);
INT_MODULE_PARM(cable_mode, -1);
INT_MODULE_PARM(wakeup, 0);
#endif

#ifdef CONFIG_PCI
static int pci_irq_list[8] = { 0 };	/* PCI interrupt assignments */
MODULE_PARM(pci_irq_list, "1-8i");
/* Default memory base addresses for CardBus controllers */
static u_int cb_mem_base[] = { 0x68000000, 0xf8000000 };
MODULE_PARM(cb_mem_base, "i");
INT_MODULE_PARM(do_pci_probe, 1);	/* Scan for PCI bridges? */
INT_MODULE_PARM(fast_pci, -1);
INT_MODULE_PARM(cb_bus_base, 0);
INT_MODULE_PARM(cb_bus_step, 2);
INT_MODULE_PARM(cb_write_post, -1);
INT_MODULE_PARM(irq_mode, -1);		/* Override BIOS routing? */
INT_MODULE_PARM(hold_time, -1);		/* Ricoh specific */
INT_MODULE_PARM(p2cclk, -1);		/* TI specific */
#endif

#if defined(CONFIG_ISA) && defined(CONFIG_PCI)
INT_MODULE_PARM(pci_csc, 1);		/* PCI card status irqs? */
INT_MODULE_PARM(pci_int, 1);		/* PCI IO card irqs? */
#elif defined(CONFIG_ISA) && !defined(CONFIG_PCI)
#define pci_csc		0
#define pci_int		0
#elif !defined(CONFIG_ISA) && defined(CONFIG_PCI)
#define pci_csc		0
#define pci_int		1		/* We must use PCI irq's */
#else
#error "No bus architectures defined!"
#endif

/*====================================================================*/

typedef struct cirrus_state_t {
    u_char		misc1, misc2;
    u_char		timer[6];
} cirrus_state_t;

typedef struct vg46x_state_t {
    u_char		ctl, ema;
} vg46x_state_t;

typedef struct ti113x_state_t {
    u_int		sysctl;
    u_char		cardctl, devctl, diag;
} ti113x_state_t;

typedef struct ricoh_state_t {
    u_short		misc, ctl, io, mem;
} ricoh_state_t;

typedef struct o2micro_state_t {
    u_char		mode_a, mode_b, mode_c, mode_d;
    u_char		mhpg, fifo, mode_e;
} o2micro_state_t;

typedef struct topic_state_t {
    u_char		slot, ccr, cdr;
    u_int		rcr;
} topic_state_t;

typedef struct socket_info_t {
    u_short		type, flags;
    socket_cap_t	cap;
    ioaddr_t		ioaddr;
    u_short		psock;
    u_char		cs_irq, intr;
    void		(*handler)(void *info, u_int events);
    void		*info;
#ifdef HAS_PROC_BUS
    struct proc_dir_entry *proc;
#endif
#ifdef CONFIG_PCI
    u_short		vendor, device;
    u_char		revision, bus, devfn;
    u_short		bcr;
    u_char		pci_lat, cb_lat, sub_bus;
    u_char		cache, pmcs;
    u_int		cb_phys;
    char		*cb_virt;
#endif
    union {
	cirrus_state_t		cirrus;
	vg46x_state_t		vg46x;
	o2micro_state_t		o2micro;
	ti113x_state_t		ti113x;
	ricoh_state_t		ricoh;
	topic_state_t		topic;
    } state;
} socket_info_t;

/* Where we keep track of our sockets... */
static int sockets = 0;
static socket_info_t socket[8] = {
    { 0, }, /* ... */
};

/* Default ISA interrupt mask */
#define I365_MASK	0xdeb8	/* irq 15,14,12,11,10,9,7,5,4,3 */

#ifdef CONFIG_ISA
static int grab_irq;
#ifdef USE_SPIN_LOCKS
static spinlock_t isa_lock = SPIN_LOCK_UNLOCKED;
#endif
#define ISA_LOCK(s, f) \
    if (!((s)->flags & IS_CARDBUS)) spin_lock_irqsave(&isa_lock, f)
#define ISA_UNLOCK(n, f) \
    if (!((s)->flags & IS_CARDBUS)) spin_unlock_irqrestore(&isa_lock, f)
#else
#define ISA_LOCK(n, f) do { } while (0)
#define ISA_UNLOCK(n, f) do { } while (0)
#endif

static struct timer_list poll_timer;

#define flip(v,b,f) (v = ((f)<0) ? v : ((f) ? ((v)|(b)) : ((v)&(~b))))

/*====================================================================*/

#ifndef PCI_VENDOR_ID_INTEL
#define PCI_VENDOR_ID_INTEL		0x8086
#endif
#ifndef PCI_DEVICE_ID_INTEL_82092AA_0
#define PCI_DEVICE_ID_INTEL_82092AA_0	0x1221
#endif
#ifndef PCI_VENDOR_ID_OMEGA
#define PCI_VENDOR_ID_OMEGA		0x119b
#endif
#ifndef PCI_DEVICE_ID_OMEGA_82C092G
#define PCI_DEVICE_ID_OMEGA_82C092G	0x1221
#endif

/* Default settings for PCI command configuration register */
#define CMD_DFLT (PCI_COMMAND_IO|PCI_COMMAND_MEMORY| \
		  PCI_COMMAND_MASTER|PCI_COMMAND_WAIT)

/* Some PCI shortcuts */

static int pci_readb(socket_info_t *s, int r, u_char *v)
{ return pcibios_read_config_byte(s->bus, s->devfn, r, v); }
static int pci_writeb(socket_info_t *s, int r, u_char v)
{ return pcibios_write_config_byte(s->bus, s->devfn, r, v); }
static int pci_readw(socket_info_t *s, int r, u_short *v)
{ return pcibios_read_config_word(s->bus, s->devfn, r, v); }
static int pci_writew(socket_info_t *s, int r, u_short v)
{ return pcibios_write_config_word(s->bus, s->devfn, r, v); }
static int pci_readl(socket_info_t *s, int r, u_int *v)
{ return pcibios_read_config_dword(s->bus, s->devfn, r, v); }
static int pci_writel(socket_info_t *s, int r, u_int v)
{ return pcibios_write_config_dword(s->bus, s->devfn, r, v); }

#define cb_readb(s, r)		readb((s)->cb_virt + (r))
#define cb_readl(s, r)		readl((s)->cb_virt + (r))
#define cb_writeb(s, r, v)	writeb(v, (s)->cb_virt + (r))
#define cb_writel(s, r, v)	writel(v, (s)->cb_virt + (r))

/*====================================================================*/

/* These definitions must match the pcic table! */
typedef enum pcic_id {
#ifdef CONFIG_ISA
    IS_I82365A, IS_I82365B, IS_I82365DF,
    IS_IBM, IS_RF5Cx96, IS_VLSI, IS_VG468, IS_VG469,
    IS_PD6710, IS_PD672X, IS_VT83C469,
#endif
#ifdef CONFIG_PCI
    IS_I82092AA, IS_OM82C092G,
    IS_PD6729, IS_PD6730, IS_PD6832,
    IS_OZ6729, IS_OZ6730, IS_OZ6832, IS_OZ6836, IS_OZ6812,
    IS_RL5C465, IS_RL5C466, IS_RL5C475, IS_RL5C476, IS_RL5C478,
    IS_SMC34C90,
    IS_TI1130, IS_TI1131, IS_TI1250A, IS_TI1220, IS_TI1221, IS_TI1210,
    IS_TI1251A, IS_TI1251B, IS_TI1450, IS_TI1225, IS_TI1211, IS_TI1420,
    IS_TOPIC95_A, IS_TOPIC95_B, IS_TOPIC97, IS_TOPIC100,
    IS_UNK_PCI, IS_UNK_CARDBUS
#endif
} pcic_id;

/* Flags for classifying groups of controllers */
#define IS_VADEM	0x0001
#define IS_CIRRUS	0x0002
#define IS_TI		0x0004
#define IS_O2MICRO	0x0008
#define IS_VIA		0x0010
#define IS_TOPIC	0x0020
#define IS_RICOH	0x0040
#define IS_UNKNOWN	0x0400
#define IS_VG_PWR	0x0800
#define IS_DF_PWR	0x1000
#define IS_PCI		0x2000
#define IS_CARDBUS	0x4000
#define IS_ALIVE	0x8000

typedef struct pcic_t {
    char		*name;
    u_short		flags;
#ifdef CONFIG_PCI
    u_short		vendor, device;
#endif
} pcic_t;

#define ID(a,b) PCI_VENDOR_ID_##a,PCI_DEVICE_ID_##a##_##b

static pcic_t pcic[] = {
#ifdef CONFIG_ISA
    { "Intel i82365sl A step", 0 },
    { "Intel i82365sl B step", 0 },
    { "Intel i82365sl DF", IS_DF_PWR },
    { "IBM Clone", 0 },
    { "Ricoh RF5C296/396", 0 },
    { "VLSI 82C146", 0 },
    { "Vadem VG-468", IS_VADEM },
    { "Vadem VG-469", IS_VADEM|IS_VG_PWR },
    { "Cirrus PD6710", IS_CIRRUS },
    { "Cirrus PD672x", IS_CIRRUS },
    { "VIA VT83C469", IS_CIRRUS|IS_VIA },
#endif
#ifdef CONFIG_PCI
    { "Intel 82092AA", IS_PCI, ID(INTEL, 82092AA_0) },
    { "Omega Micro 82C092G", IS_PCI, ID(OMEGA, 82C092G) },
    { "Cirrus PD6729", IS_CIRRUS|IS_PCI, ID(CIRRUS, 6729) },
    { "Cirrus PD6730", IS_CIRRUS|IS_PCI, PCI_VENDOR_ID_CIRRUS, 0xffff },
    { "Cirrus PD6832", IS_CIRRUS|IS_CARDBUS, ID(CIRRUS, 6832) },
    { "O2Micro OZ6729", IS_O2MICRO|IS_PCI|IS_VG_PWR, ID(O2, 6729) },
    { "O2Micro OZ6730", IS_O2MICRO|IS_PCI|IS_VG_PWR, ID(O2, 6730) },
    { "O2Micro OZ6832/OZ6833", IS_O2MICRO|IS_CARDBUS, ID(O2, 6832) },
    { "O2Micro OZ6836/OZ6860", IS_O2MICRO|IS_CARDBUS, ID(O2, 6836) },
    { "O2Micro OZ6812", IS_O2MICRO|IS_CARDBUS, ID(O2, 6812) },
    { "Ricoh RL5C465", IS_RICOH|IS_CARDBUS, ID(RICOH, RL5C465) },
    { "Ricoh RL5C466", IS_RICOH|IS_CARDBUS, ID(RICOH, RL5C466) },
    { "Ricoh RL5C475", IS_RICOH|IS_CARDBUS, ID(RICOH, RL5C475) },
    { "Ricoh RL5C476", IS_RICOH|IS_CARDBUS, ID(RICOH, RL5C476) },
    { "Ricoh RL5C478", IS_RICOH|IS_CARDBUS, ID(RICOH, RL5C478) },
    { "SMC 34C90", IS_CARDBUS, ID(SMC, 34C90) },
    { "TI 1130", IS_TI|IS_CARDBUS, ID(TI, 1130) },
    { "TI 1131", IS_TI|IS_CARDBUS, ID(TI, 1131) },
    { "TI 1250A", IS_TI|IS_CARDBUS, ID(TI, 1250A) },
    { "TI 1220", IS_TI|IS_CARDBUS, ID(TI, 1220) },
    { "TI 1221", IS_TI|IS_CARDBUS, ID(TI, 1221) },
    { "TI 1210", IS_TI|IS_CARDBUS, ID(TI, 1210) },
    { "TI 1251A", IS_TI|IS_CARDBUS, ID(TI, 1251A) },
    { "TI 1251B", IS_TI|IS_CARDBUS, ID(TI, 1251B) },
    { "TI 1450", IS_TI|IS_CARDBUS, ID(TI, 1450) },
    { "TI 1225", IS_TI|IS_CARDBUS, ID(TI, 1225) },
    { "TI 1211", IS_TI|IS_CARDBUS, ID(TI, 1211) },
    { "TI 1420", IS_TI|IS_CARDBUS, ID(TI, 1420) },
    { "Toshiba ToPIC95-A", IS_CARDBUS|IS_TOPIC, ID(TOSHIBA, TOPIC95_A) },
    { "Toshiba ToPIC95-B", IS_CARDBUS|IS_TOPIC, ID(TOSHIBA, TOPIC95_B) },
    { "Toshiba ToPIC97", IS_CARDBUS|IS_TOPIC, ID(TOSHIBA, TOPIC97) },
    { "Toshiba ToPIC100", IS_CARDBUS|IS_TOPIC, ID(TOSHIBA, TOPIC100) },
    { "Unknown", IS_PCI|IS_UNKNOWN, 0, 0 },
    { "Unknown", IS_CARDBUS|IS_UNKNOWN, 0, 0 }
#endif
};

#define PCIC_COUNT	(sizeof(pcic)/sizeof(pcic_t))

/*====================================================================*/

static u_char i365_get(socket_info_t *s, u_short reg)
{
#ifdef CONFIG_PCI
    if (s->cb_virt)
	return cb_readb(s, 0x0800 + reg);
#endif
    outb(I365_REG(s->psock, reg), s->ioaddr);
    return inb(s->ioaddr+1);
}

static void i365_set(socket_info_t *s, u_short reg, u_char data)
{
#ifdef CONFIG_PCI
    if (s->cb_virt) {
	cb_writeb(s, 0x0800 + reg, data);
	return;
    }
#endif
    outb(I365_REG(s->psock, reg), s->ioaddr);
    outb(data, s->ioaddr+1);
}

static void i365_bset(socket_info_t *s, u_short reg, u_char mask)
{
    u_char d = i365_get(s, reg);
    i365_set(s, reg, d | mask);
}

static void i365_bclr(socket_info_t *s, u_short reg, u_char mask)
{
    u_char d = i365_get(s, reg);
    i365_set(s, reg, d & ~mask);
}

static void i365_bflip(socket_info_t *s, u_short reg, u_char mask, int b)
{
    u_char d = i365_get(s, reg);
    i365_set(s, reg, (b) ? (d | mask) : (d & ~mask));
}

static u_short i365_get_pair(socket_info_t *s, u_short reg)
{
    u_short a = i365_get(s, reg), b = i365_get(s, reg+1);
    return (a + (b<<8));
}

static void i365_set_pair(socket_info_t *s, u_short reg, u_short data)
{
    i365_set(s, reg, data & 0xff);
    i365_set(s, reg+1, data >> 8);
}

/*======================================================================

    Code to save and restore global state information for Cirrus
    PD67xx controllers, and to set and report global configuration
    options.

    The VIA controllers also use these routines, as they are mostly
    Cirrus lookalikes, without the timing registers.
    
======================================================================*/

static void __init cirrus_get_state(socket_info_t *s)
{
    cirrus_state_t *p = &s->state.cirrus;
    int i;

    p->misc1 = i365_get(s, PD67_MISC_CTL_1);
    p->misc1 &= (PD67_MC1_MEDIA_ENA | PD67_MC1_INPACK_ENA);
    p->misc2 = i365_get(s, PD67_MISC_CTL_2);
    for (i = 0; i < 6; i++)
	p->timer[i] = i365_get(s, PD67_TIME_SETUP(0)+i);
}

static void cirrus_set_state(socket_info_t *s)
{
    cirrus_state_t *p = &s->state.cirrus;
    u_char misc;
    int i;

    misc = i365_get(s, PD67_MISC_CTL_2);
    i365_set(s, PD67_MISC_CTL_2, p->misc2);
    if (misc & PD67_MC2_SUSPEND) mdelay(50);
    misc = i365_get(s, PD67_MISC_CTL_1);
    misc &= ~(PD67_MC1_MEDIA_ENA | PD67_MC1_INPACK_ENA);
    i365_set(s, PD67_MISC_CTL_1, misc | p->misc1);
    for (i = 0; i < 6; i++)
	i365_set(s, PD67_TIME_SETUP(0)+i, p->timer[i]);
}

#ifdef CONFIG_PCI
static int cirrus_set_irq_mode(socket_info_t *s, int pcsc, int pint)
{
    flip(s->bcr, PD6832_BCR_MGMT_IRQ_ENA, !pcsc);
    return 0;
}
#endif /* CONFIG_PCI */

static u_int __init cirrus_set_opts(socket_info_t *s, char *buf)
{
    cirrus_state_t *p = &s->state.cirrus;
    u_int mask = 0xffff;

    p->misc1 |= PD67_MC1_SPKR_ENA;
    if (has_ring == -1) has_ring = 1;
    flip(p->misc2, PD67_MC2_IRQ15_RI, has_ring);
    flip(p->misc2, PD67_MC2_DYNAMIC_MODE, dynamic_mode);
    if (p->misc2 & PD67_MC2_IRQ15_RI)
	strcat(buf, " [ring]");
    if (p->misc2 & PD67_MC2_DYNAMIC_MODE)
	strcat(buf, " [dyn mode]");
    if (p->misc1 & PD67_MC1_INPACK_ENA)
	strcat(buf, " [inpack]");
    if (!(s->flags & (IS_PCI | IS_CARDBUS))) {
	if (p->misc2 & PD67_MC2_IRQ15_RI)
	    mask &= ~0x8000;
	if (has_led > 0) {
	    strcat(buf, " [led]");
	    mask &= ~0x1000;
	}
	if (has_dma > 0) {
	    strcat(buf, " [dma]");
	    mask &= ~0x0600;
	flip(p->misc2, PD67_MC2_FREQ_BYPASS, freq_bypass);
	if (p->misc2 & PD67_MC2_FREQ_BYPASS)
	    strcat(buf, " [freq bypass]");
	}
#ifdef CONFIG_PCI
    } else {
	p->misc1 &= ~PD67_MC1_MEDIA_ENA;
	flip(p->misc2, PD67_MC2_FAST_PCI, fast_pci);
	if (p->misc2 & PD67_MC2_IRQ15_RI)
	    mask &= (s->type == IS_PD6730) ? ~0x0400 : ~0x8000;
#endif
    }
    if (!(s->flags & IS_VIA)) {
	if (setup_time >= 0)
	    p->timer[0] = p->timer[3] = setup_time;
	if (cmd_time > 0) {
	    p->timer[1] = cmd_time;
	    p->timer[4] = cmd_time*2+4;
	}
	if (p->timer[1] == 0) {
	    p->timer[1] = 6; p->timer[4] = 16;
	    if (p->timer[0] == 0)
		p->timer[0] = p->timer[3] = 1;
	}
	if (recov_time >= 0)
	    p->timer[2] = p->timer[5] = recov_time;
	buf += strlen(buf);
	sprintf(buf, " [%d/%d/%d] [%d/%d/%d]", p->timer[0], p->timer[1],
		p->timer[2], p->timer[3], p->timer[4], p->timer[5]);
    }
    return mask;
}

/*======================================================================

    Code to save and restore global state information for Vadem VG468
    and VG469 controllers, and to set and report global configuration
    options.
    
======================================================================*/

#ifdef CONFIG_ISA

static void __init vg46x_get_state(socket_info_t *s)
{
    vg46x_state_t *p = &s->state.vg46x;
    p->ctl = i365_get(s, VG468_CTL);
    if (s->type == IS_VG469)
	p->ema = i365_get(s, VG469_EXT_MODE);
}

static void vg46x_set_state(socket_info_t *s)
{
    vg46x_state_t *p = &s->state.vg46x;
    i365_set(s, VG468_CTL, p->ctl);
    if (s->type == IS_VG469)
	i365_set(s, VG469_EXT_MODE, p->ema);
}

static u_int __init vg46x_set_opts(socket_info_t *s, char *buf)
{
    vg46x_state_t *p = &s->state.vg46x;
    
    flip(p->ctl, VG468_CTL_ASYNC, async_clock);
    flip(p->ema, VG469_MODE_CABLE, cable_mode);
    if (p->ctl & VG468_CTL_ASYNC)
	strcat(buf, " [async]");
    if (p->ctl & VG468_CTL_INPACK)
	strcat(buf, " [inpack]");
    if (s->type == IS_VG469) {
	u_char vsel = i365_get(s, VG469_VSELECT);
	if (vsel & VG469_VSEL_EXT_STAT) {
	    strcat(buf, " [ext mode]");
	    if (vsel & VG469_VSEL_EXT_BUS)
		strcat(buf, " [isa buf]");
	}
	if (p->ema & VG469_MODE_CABLE)
	    strcat(buf, " [cable]");
	if (p->ema & VG469_MODE_COMPAT)
	    strcat(buf, " [c step]");
    }
    return 0xffff;
}

#endif

/*======================================================================

    Code to save and restore global state information for TI 1130 and
    TI 1131 controllers, and to set and report global configuration
    options.
    
======================================================================*/

#ifdef CONFIG_PCI

static void __init ti113x_get_state(socket_info_t *s)
{
    ti113x_state_t *p = &s->state.ti113x;
    pci_readl(s, TI113X_SYSTEM_CONTROL, &p->sysctl);
    pci_readb(s, TI113X_CARD_CONTROL, &p->cardctl);
    pci_readb(s, TI113X_DEVICE_CONTROL, &p->devctl);
    pci_readb(s, TI1250_DIAGNOSTIC, &p->diag);
}

static void ti113x_set_state(socket_info_t *s)
{
    ti113x_state_t *p = &s->state.ti113x;
    pci_writel(s, TI113X_SYSTEM_CONTROL, p->sysctl);
    pci_writeb(s, TI113X_CARD_CONTROL, p->cardctl);
    pci_writeb(s, TI113X_DEVICE_CONTROL, p->devctl);
    pci_writeb(s, TI1250_MULTIMEDIA_CTL, 0);
    pci_writeb(s, TI1250_DIAGNOSTIC, p->diag);
    i365_set_pair(s, TI113X_IO_OFFSET(0), 0);
    i365_set_pair(s, TI113X_IO_OFFSET(1), 0);
}

static int ti113x_set_irq_mode(socket_info_t *s, int pcsc, int pint)
{
    ti113x_state_t *p = &s->state.ti113x;
    s->intr = (pcsc) ? I365_INTR_ENA : 0;
    if (s->type <= IS_TI1131) {
	p->cardctl &= ~(TI113X_CCR_PCI_IRQ_ENA |
			TI113X_CCR_PCI_IREQ | TI113X_CCR_PCI_CSC);
	if (pcsc)
	    p->cardctl |= TI113X_CCR_PCI_IRQ_ENA | TI113X_CCR_PCI_CSC;
	if (pint)
	    p->cardctl |= TI113X_CCR_PCI_IRQ_ENA | TI113X_CCR_PCI_IREQ;
    } else if (s->type == IS_TI1250A) {
	p->diag &= TI1250_DIAG_PCI_CSC | TI1250_DIAG_PCI_IREQ;
	if (pcsc)
	    p->diag |= TI1250_DIAG_PCI_CSC;
	if (pint)
	    p->diag |= TI1250_DIAG_PCI_IREQ;
    }
    return 0;
}

static u_int __init ti113x_set_opts(socket_info_t *s, char *buf)
{
    ti113x_state_t *p = &s->state.ti113x;
    u_int mask = 0xffff;
    int old = (s->type <= IS_TI1131);
    
    flip(p->cardctl, TI113X_CCR_RIENB, has_ring);
    p->cardctl &= ~TI113X_CCR_ZVENABLE;
    p->cardctl |= TI113X_CCR_SPKROUTEN;
    if (!old) flip(p->sysctl, TI122X_SCR_P2CCLK, p2cclk);
    switch (irq_mode) {
    case 0:
	p->devctl &= ~TI113X_DCR_IMODE_MASK;
	break;
    case 1:
	p->devctl &= ~TI113X_DCR_IMODE_MASK;
	p->devctl |= TI113X_DCR_IMODE_ISA;
	break;
    case 2:
	p->devctl &= ~TI113X_DCR_IMODE_MASK;
	p->devctl |= TI113X_DCR_IMODE_SERIAL;
	break;
    case 3:
	p->devctl &= ~TI113X_DCR_IMODE_MASK;
	p->devctl |= TI12XX_DCR_IMODE_ALL_SERIAL;
	break;
    default:
	/* Feeble fallback: if PCI-only but no PCI irq, try ISA */
	if (((p->devctl & TI113X_DCR_IMODE_MASK) == 0) &&
	    (s->cap.pci_irq == 0))
	    p->devctl |= TI113X_DCR_IMODE_ISA;
    }
    if (p->cardctl & TI113X_CCR_RIENB) {
	strcat(buf, " [ring]");
	if (old) mask &= ~0x8000;
    }
    if (old && (p->sysctl & TI113X_SCR_CLKRUN_ENA)) {
	if (p->sysctl & TI113X_SCR_CLKRUN_SEL) {
	    strcat(buf, " [clkrun irq 12]");
	    mask &= ~0x1000;
	} else {
	    strcat(buf, " [clkrun irq 10]");
	    mask &= ~0x0400;
	}
    }
    switch (p->devctl & TI113X_DCR_IMODE_MASK) {
    case TI12XX_DCR_IMODE_PCI_ONLY:
	strcat(buf, " [pci only]");
	mask = 0;
	break;
    case TI113X_DCR_IMODE_ISA:
	strcat(buf, " [isa irq]");
	if (old) mask &= ~0x0018;
	break;
    case TI113X_DCR_IMODE_SERIAL:
	strcat(buf, " [pci + serial irq]");
	mask = 0xffff;
	break;
    case TI12XX_DCR_IMODE_ALL_SERIAL:
	strcat(buf, " [serial pci & irq]");
	mask = 0xffff;
	break;
    }
    return mask;
}

#endif

/*======================================================================

    Code to save and restore global state information for the Ricoh
    RL5C4XX controllers, and to set and report global configuration
    options.
    
======================================================================*/

#ifdef CONFIG_PCI

static void __init ricoh_get_state(socket_info_t *s)
{
    ricoh_state_t *p = &s->state.ricoh;
    pci_readw(s, RL5C4XX_MISC, &p->misc);
    pci_readw(s, RL5C4XX_16BIT_CTL, &p->ctl);
    pci_readw(s, RL5C4XX_16BIT_IO_0, &p->io);
    pci_readw(s, RL5C4XX_16BIT_MEM_0, &p->mem);
}

static void ricoh_set_state(socket_info_t *s)
{
    ricoh_state_t *p = &s->state.ricoh;
    pci_writew(s, RL5C4XX_MISC, p->misc);
    pci_writew(s, RL5C4XX_16BIT_CTL, p->ctl);
    pci_writew(s, RL5C4XX_16BIT_IO_0, p->io);
    pci_writew(s, RL5C4XX_16BIT_MEM_0, p->mem);
}

static u_int __init ricoh_set_opts(socket_info_t *s, char *buf)
{
    ricoh_state_t *p = &s->state.ricoh;
    u_int mask = 0xffff;
    int old = (s->type < IS_RL5C475);

    p->ctl = RL5C4XX_16CTL_IO_TIMING | RL5C4XX_16CTL_MEM_TIMING;
    if (old) p->ctl |= RL5C46X_16CTL_LEVEL_1 | RL5C46X_16CTL_LEVEL_2;
    
    if (setup_time >= 0) {
	p->io = (p->io & ~RL5C4XX_SETUP_MASK) +
	    ((setup_time+1) << RL5C4XX_SETUP_SHIFT);
	p->mem = (p->mem & ~RL5C4XX_SETUP_MASK) +
	    (setup_time << RL5C4XX_SETUP_SHIFT);
    }
    if (cmd_time >= 0) {
	p->io = (p->io & ~RL5C4XX_CMD_MASK) +
	    (cmd_time << RL5C4XX_CMD_SHIFT);
	p->mem = (p->mem & ~RL5C4XX_CMD_MASK) +
	    (cmd_time << RL5C4XX_CMD_SHIFT);
    }
    if (hold_time >= 0) {
	p->io = (p->io & ~RL5C4XX_HOLD_MASK) +
	    (hold_time << RL5C4XX_HOLD_SHIFT);
	p->mem = (p->mem & ~RL5C4XX_HOLD_MASK) +
	    (hold_time << RL5C4XX_HOLD_SHIFT);
    }
    if (!old) {
	switch (irq_mode) {
	case 1:
	    p->misc &= ~RL5C47X_MISC_SRIRQ_ENA; break;
	case 2:
	    p->misc |= RL5C47X_MISC_SRIRQ_ENA; break;
	}
	if (p->misc & RL5C47X_MISC_SRIRQ_ENA)
	    sprintf(buf, " [serial irq]");
	else
	    sprintf(buf, " [isa irq]");
	buf += strlen(buf);
    }
    sprintf(buf, " [io %d/%d/%d] [mem %d/%d/%d]",
	    (p->io & RL5C4XX_SETUP_MASK) >> RL5C4XX_SETUP_SHIFT,
	    (p->io & RL5C4XX_CMD_MASK) >> RL5C4XX_CMD_SHIFT,
	    (p->io & RL5C4XX_HOLD_MASK) >> RL5C4XX_HOLD_SHIFT,
	    (p->mem & RL5C4XX_SETUP_MASK) >> RL5C4XX_SETUP_SHIFT,
	    (p->mem & RL5C4XX_CMD_MASK) >> RL5C4XX_CMD_SHIFT,
	    (p->mem & RL5C4XX_HOLD_MASK) >> RL5C4XX_HOLD_SHIFT);
    return mask;
}

#endif

/*======================================================================

    Code to save and restore global state information for O2Micro
    controllers, and to set and report global configuration options.
    
======================================================================*/

#ifdef CONFIG_PCI

static void __init o2micro_get_state(socket_info_t *s)
{
    o2micro_state_t *p = &s->state.o2micro;
    if ((s->revision == 0x34) || (s->revision == 0x62) ||
	(s->type == IS_OZ6812)) {
	p->mode_a = i365_get(s, O2_MODE_A_2);
	p->mode_b = i365_get(s, O2_MODE_B_2);
    } else {
	p->mode_a = i365_get(s, O2_MODE_A);
	p->mode_b = i365_get(s, O2_MODE_B);
    }
    p->mode_c = i365_get(s, O2_MODE_C);
    p->mode_d = i365_get(s, O2_MODE_D);
    if (s->flags & IS_CARDBUS) {
	p->mhpg = i365_get(s, O2_MHPG_DMA);
	p->fifo = i365_get(s, O2_FIFO_ENA);
	p->mode_e = i365_get(s, O2_MODE_E);
    }
}

static void o2micro_set_state(socket_info_t *s)
{
    o2micro_state_t *p = &s->state.o2micro;
    if ((s->revision == 0x34) || (s->revision == 0x62) ||
	(s->type == IS_OZ6812)) {
	i365_set(s, O2_MODE_A_2, p->mode_a);
	i365_set(s, O2_MODE_B_2, p->mode_b);
    } else {
	i365_set(s, O2_MODE_A, p->mode_a);
	i365_set(s, O2_MODE_B, p->mode_b);
    }
    i365_set(s, O2_MODE_C, p->mode_c);
    i365_set(s, O2_MODE_D, p->mode_d);
    if (s->flags & IS_CARDBUS) {
	i365_set(s, O2_MHPG_DMA, p->mhpg);
	i365_set(s, O2_FIFO_ENA, p->fifo);
	i365_set(s, O2_MODE_E, p->mode_e);
    }
}

static u_int __init o2micro_set_opts(socket_info_t *s, char *buf)
{
    o2micro_state_t *p = &s->state.o2micro;
    u_int mask = 0xffff;

    p->mode_b = (p->mode_b & ~O2_MODE_B_IDENT) | O2_MODE_B_ID_CSTEP;
    flip(p->mode_b, O2_MODE_B_IRQ15_RI, has_ring);
    p->mode_c &= ~(O2_MODE_C_ZVIDEO | O2_MODE_C_DREQ_MASK);
    if (s->flags & IS_CARDBUS) {
	p->mode_d &= ~O2_MODE_D_W97_IRQ;
	p->mode_e &= ~O2_MODE_E_MHPG_DMA;
	p->mhpg |= O2_MHPG_CINT_ENA | O2_MHPG_CSC_ENA;
	p->mhpg &= ~O2_MHPG_CHANNEL;
	if (s->revision == 0x34)
	    p->mode_c = 0x20;
    } else {
	if (p->mode_b & O2_MODE_B_IRQ15_RI) mask &= ~0x8000;
    }
    sprintf(buf, " [a %02x] [b %02x] [c %02x] [d %02x]",
	    p->mode_a, p->mode_b, p->mode_c, p->mode_d);
    if (s->flags & IS_CARDBUS) {
	buf += strlen(buf);
	sprintf(buf, " [mhpg %02x] [fifo %02x] [e %02x]",
		p->mhpg, p->fifo, p->mode_e);
    }
    return mask;
}

#endif

/*======================================================================

    Code to save and restore global state information for the Toshiba
    ToPIC 95 and 97 controllers, and to set and report global
    configuration options.
    
======================================================================*/

#ifdef CONFIG_PCI

static void __init topic_get_state(socket_info_t *s)
{
    topic_state_t *p = &s->state.topic;
    pci_readb(s, TOPIC_SLOT_CONTROL, &p->slot);
    pci_readb(s, TOPIC_CARD_CONTROL, &p->ccr);
    pci_readb(s, TOPIC_CARD_DETECT, &p->cdr);
    pci_readl(s, TOPIC_REGISTER_CONTROL, &p->rcr);
}

static void topic_set_state(socket_info_t *s)
{
    topic_state_t *p = &s->state.topic;
    pci_writeb(s, TOPIC_SLOT_CONTROL, p->slot);
    pci_writeb(s, TOPIC_CARD_CONTROL, p->ccr);
    pci_writeb(s, TOPIC_CARD_DETECT, p->cdr);
    pci_writel(s, TOPIC_REGISTER_CONTROL, p->rcr);
}

static int topic_set_irq_mode(socket_info_t *s, int pcsc, int pint)
{
    if (s->type >= IS_TOPIC97) {
	topic_state_t *p = &s->state.topic;
	flip(p->ccr, TOPIC97_ICR_IRQSEL, pcsc);
	return 0;
    } else {
	/* no ISA card status change irq */
	return !pcsc;
    }
}

static u_int __init topic_set_opts(socket_info_t *s, char *buf)
{
    topic_state_t *p = &s->state.topic;

    p->slot |= TOPIC_SLOT_SLOTON|TOPIC_SLOT_SLOTEN|TOPIC_SLOT_ID_LOCK;
    p->cdr |= TOPIC_CDR_MODE_PC32;
    p->cdr &= ~(TOPIC_CDR_SW_DETECT);
    sprintf(buf, " [slot 0x%02x] [ccr 0x%02x] [cdr 0x%02x] [rcr 0x%02x]",
	    p->slot, p->ccr, p->cdr, p->rcr);
    return 0xffff;
}

#endif

/*======================================================================

    Routines to handle common CardBus options
    
======================================================================*/

#ifdef CONFIG_PCI

static void __init cb_get_state(socket_info_t *s)
{
    pci_readb(s, PCI_CACHE_LINE_SIZE, &s->cache);
    pci_readb(s, PCI_LATENCY_TIMER, &s->pci_lat);
    pci_readb(s, CB_LATENCY_TIMER, &s->cb_lat);
    pci_readb(s, CB_CARDBUS_BUS, &s->cap.cardbus);
    pci_readb(s, CB_SUBORD_BUS, &s->sub_bus);
    pci_readw(s, CB_BRIDGE_CONTROL, &s->bcr);
#ifdef __BEOS__
    pci_readb(s, PCI_INTERRUPT_LINE, &s->cap.pci_irq);
#else
#if (LINUX_VERSION_CODE < VERSION(2,1,93))
    pci_readb(s, PCI_INTERRUPT_LINE, &s->cap.pci_irq);
#else
    {
	struct pci_dev *pdev = pci_find_slot(s->bus, s->devfn);
	s->cap.pci_irq = (pdev) ? pdev->irq : 0;
    }
#endif
#endif
    if ((s->cap.pci_irq == 0) && (pci_csc || pci_int))
	s->cap.pci_irq = pci_irq_list[s - socket];
    if (s->cap.pci_irq >= NR_IRQS) s->cap.pci_irq = 0;
}

static void cb_set_state(socket_info_t *s)
{
    if (s->pmcs)
	pci_writew(s, s->pmcs, PCI_PMCS_PWR_STATE_D0);
    pci_writel(s, CB_LEGACY_MODE_BASE, 0);
    pci_writel(s, PCI_BASE_ADDRESS_0, s->cb_phys);
    pci_writew(s, PCI_COMMAND, CMD_DFLT);
    pci_writeb(s, PCI_CACHE_LINE_SIZE, s->cache);
    pci_writeb(s, PCI_LATENCY_TIMER, s->pci_lat);
    pci_writeb(s, CB_LATENCY_TIMER, s->cb_lat);
    pci_writeb(s, CB_CARDBUS_BUS, s->cap.cardbus);
    pci_writeb(s, CB_SUBORD_BUS, s->sub_bus);
    pci_writew(s, CB_BRIDGE_CONTROL, s->bcr);
}

static int cb_get_irq_mode(socket_info_t *s)
{
    return (!(s->bcr & CB_BCR_ISA_IRQ));
}

static int cb_set_irq_mode(socket_info_t *s, int pcsc, int pint)
{
    flip(s->bcr, CB_BCR_ISA_IRQ, !(pint));
    if (s->flags & IS_CIRRUS)
	return cirrus_set_irq_mode(s, pcsc, pint);
    else if (s->flags & IS_TI)
	return ti113x_set_irq_mode(s, pcsc, pint);
    else if (s->flags & IS_TOPIC)
	return topic_set_irq_mode(s, pcsc, pint);
    /* By default, assume that we can't do ISA status irqs */
    return (!pcsc);
}

static void __init cb_set_opts(socket_info_t *s, char *buf)
{
    s->bcr |= CB_BCR_WRITE_POST;
    /* some TI1130's seem to exhibit problems with write posting */
    if (((s->type == IS_TI1130) && (s->revision == 4) &&
	 (cb_write_post < 0)) || (cb_write_post == 0))
	s->bcr &= ~CB_BCR_WRITE_POST;
    if (s->cache == 0) s->cache = 8;
    if (s->pci_lat == 0) s->pci_lat = 0xa8;
    if (s->cb_lat == 0) s->cb_lat = 0xb0;
    if (s->cap.pci_irq == 0)
	strcat(buf, " [no pci irq]");
    else
	sprintf(buf, " [pci irq %d]", s->cap.pci_irq);
    buf += strlen(buf);
    if ((cb_bus_base > 0) || (s->cap.cardbus == 0)) {
	if (cb_bus_base <= 0) cb_bus_base = 0x20;
	s->cap.cardbus = cb_bus_base;
	s->sub_bus = cb_bus_base+cb_bus_step;
	cb_bus_base += cb_bus_step+1;
    }
    if (!(s->flags & IS_TOPIC))
	s->cap.features |= SS_CAP_PAGE_REGS;
    sprintf(buf, " [lat %d/%d] [bus %d/%d]",
	    s->pci_lat, s->cb_lat, s->cap.cardbus, s->sub_bus);
}

#endif /* CONFIG_PCI */

/*======================================================================

    Power control for Cardbus controllers: used both for 16-bit and
    Cardbus cards.
    
======================================================================*/

#ifdef CONFIG_PCI

static void cb_get_power(socket_info_t *s, socket_state_t *state)
{
    u_int reg = cb_readl(s, CB_SOCKET_CONTROL);
    state->Vcc = state->Vpp = 0;
    switch (reg & CB_SC_VCC_MASK) {
    case CB_SC_VCC_3V:		state->Vcc = 33; break;
    case CB_SC_VCC_5V:		state->Vcc = 50; break;
    }
    switch (reg & CB_SC_VPP_MASK) {
    case CB_SC_VPP_3V:		state->Vpp = 33; break;
    case CB_SC_VPP_5V:		state->Vpp = 50; break;
    case CB_SC_VPP_12V:		state->Vpp = 120; break;
    }
}

static int cb_set_power(socket_info_t *s, socket_state_t *state)
{
    u_int reg = 0;
    switch (state->Vcc) {
    case 0:		reg = 0; break;
    case 33:		reg = CB_SC_VCC_3V; break;
    case 50:		reg = CB_SC_VCC_5V; break;
    default:		return -EINVAL;
    }
    switch (state->Vpp) {
    case 0:		break;
    case 33:		reg |= CB_SC_VPP_3V; break;
    case 50:		reg |= CB_SC_VPP_5V; break;
    case 120:		reg |= CB_SC_VPP_12V; break;
    default:		return -EINVAL;
    }
    if (reg != cb_readl(s, CB_SOCKET_CONTROL))
	cb_writel(s, CB_SOCKET_CONTROL, reg);
    return 0;
}

#endif

/*======================================================================

    Generic routines to get and set controller options
    
======================================================================*/

static void __init get_bridge_state(socket_info_t *s)
{
    if (s->flags & IS_CIRRUS)
	cirrus_get_state(s);
#ifdef CONFIG_ISA
    else if (s->flags & IS_VADEM)
	vg46x_get_state(s);
#endif
#ifdef CONFIG_PCI
    else if (s->flags & IS_O2MICRO)
	o2micro_get_state(s);
    else if (s->flags & IS_TI)
	ti113x_get_state(s);
    else if (s->flags & IS_RICOH)
	ricoh_get_state(s);
    else if (s->flags & IS_TOPIC)
	topic_get_state(s);
    if (s->flags & IS_CARDBUS)
	cb_get_state(s);
#endif
}

static void set_bridge_state(socket_info_t *s)
{
#ifdef CONFIG_PCI
    if (s->flags & IS_CARDBUS)
	cb_set_state(s);
#endif
    if (s->flags & IS_CIRRUS)
	cirrus_set_state(s);
    else {
	i365_set(s, I365_GBLCTL, 0x00);
	i365_set(s, I365_GENCTL, 0x00);
    }
    i365_bflip(s, I365_INTCTL, I365_INTR_ENA, s->intr);
#ifdef CONFIG_ISA
    if (s->flags & IS_VADEM)
	vg46x_set_state(s);
#endif
#ifdef CONFIG_PCI
    if (s->flags & IS_O2MICRO)
	o2micro_set_state(s);
    else if (s->flags & IS_TI)
	ti113x_set_state(s);
    else if (s->flags & IS_RICOH)
	ricoh_set_state(s);
    else if (s->flags & IS_TOPIC)
	topic_set_state(s);
#endif
}

static u_int __init set_bridge_opts(socket_info_t *s, u_short ns)
{
    u_short i;
    u_int m = 0xffff;
    char buf[128];

    for (i = 0; i < ns; i++) {
	if (s[i].flags & IS_ALIVE) {
	    printk(KERN_INFO "    host opts [%d]: already alive!\n", i);
	    continue;
	}
	buf[0] = '\0';
	get_bridge_state(s+i);
	if (s[i].flags & IS_CIRRUS)
	    m = cirrus_set_opts(s+i, buf);
#ifdef CONFIG_ISA
	else if (s[i].flags & IS_VADEM)
	    m = vg46x_set_opts(s+i, buf);
#endif
#ifdef CONFIG_PCI
	else if (s[i].flags & IS_O2MICRO)
	    m = o2micro_set_opts(s+i, buf);
	else if (s[i].flags & IS_TI)
	    m = ti113x_set_opts(s+i, buf);
	else if (s[i].flags & IS_RICOH)
	    m = ricoh_set_opts(s+i, buf);
	else if (s[i].flags & IS_TOPIC)
	    m = topic_set_opts(s+i, buf);
	if (s[i].flags & IS_CARDBUS)
	    cb_set_opts(s+i, buf+strlen(buf));
#endif
	set_bridge_state(s+i);
	printk(KERN_INFO "    host opts [%d]:%s\n", i,
	       (*buf) ? buf : " none");
    }
#ifdef CONFIG_PCI
    /* Mask out all PCI interrupts */
    for (i = 0; i < sockets; i++)
	m &= ~(1<<s[i].cap.pci_irq);
#ifdef __LINUX__
#if (LINUX_VERSION_CODE > VERSION(2,1,0))
    {
	struct pci_dev *p;
	for (p = pci_devices; p; p = p->next)
	    m &= ~(1<<p->irq);
    }
#endif
#endif
#endif
    return m;
}

/*======================================================================

    Interrupt testing code, for ISA and PCI interrupts
    
======================================================================*/

static volatile u_int irq_hits;
static socket_info_t *irq_sock = socket;

static irq_ret_t irq_count IRQ(int irq, void *dev, struct pt_regs *regs)
{
#ifdef CONFIG_PCI
    if (irq_sock->flags & IS_CARDBUS) {
	cb_writel(irq_sock, CB_SOCKET_EVENT, -1);
    } else
#endif
    i365_get(irq_sock, I365_CSC);
    irq_hits++;
#ifndef __BEOS__
    DEBUG(2, "-> hit on irq %d\n", irq);
#endif
    return (irq_ret_t)1;
}

#ifdef CONFIG_ISA

static u_int __init test_irq(socket_info_t *s, int irq)
{
#ifdef CONFIG_PNP_BIOS
    extern int check_pnp_irq(int);
    if (check_pnp_irq(irq)) return 1;
#endif

    DEBUG(2, "  testing ISA irq %d\n", irq);
    if (_request_irq(irq, irq_count, 0, "scan") != 0)
	return 1;
    irq_hits = 0; irq_sock = s;
    __set_current_state(TASK_UNINTERRUPTIBLE);
    schedule_timeout(HZ/100);
    if (irq_hits) {
	_free_irq(irq, irq_count);
	DEBUG(2, "    spurious hit!\n");
	return 1;
    }

    /* Generate one interrupt */
#ifdef CONFIG_PCI
    if (s->flags & IS_CARDBUS) {
	cb_writel(s, CB_SOCKET_EVENT, -1);
	i365_set(s, I365_CSCINT, I365_CSC_STSCHG | (irq << 4));
	cb_writel(s, CB_SOCKET_EVENT, -1);
	cb_writel(s, CB_SOCKET_MASK, CB_SM_CSTSCHG);
	cb_writel(s, CB_SOCKET_FORCE, CB_SE_CSTSCHG);
	udelay(1000);
	cb_writel(s, CB_SOCKET_EVENT, -1);
	cb_writel(s, CB_SOCKET_MASK, 0);
    } else
#endif
    {
	i365_set(s, I365_CSCINT, I365_CSC_DETECT | (irq << 4));
	i365_bset(s, I365_GENCTL, I365_CTL_SW_IRQ);
	udelay(1000);
    }

    _free_irq(irq, irq_count);

    /* mask all interrupts */
    i365_set(s, I365_CSCINT, 0);
    DEBUG(2, "    hits = %d\n", irq_hits);
    
    return (irq_hits != 1);
}

static u_int __init isa_scan(socket_info_t *s, u_int mask0)
{
    u_int mask1 = 0;
    int i;

#ifdef __alpha__
#define PIC 0x4d0
    /* Don't probe level-triggered interrupts -- reserved for PCI */
    mask0 &= ~(inb(PIC) | (inb(PIC+1) << 8));
#endif
    
#ifdef CONFIG_PCI
    /* Only scan if we can select ISA csc irq's */
    if (!(s->flags & IS_CARDBUS) || (cb_set_irq_mode(s, 0, 0) == 0))
#endif
    if (do_scan) {
	set_bridge_state(s);
	i365_set(s, I365_CSCINT, 0);
	for (i = 0; i < 16; i++)
	    if ((mask0 & (1 << i)) && (test_irq(s, i) == 0))
		mask1 |= (1 << i);
	for (i = 0; i < 16; i++)
	    if ((mask1 & (1 << i)) && (test_irq(s, i) != 0))
		mask1 ^= (1 << i);
    }
    
    printk(KERN_INFO "    ISA irqs (");
    if (mask1) {
	printk("scanned");
    } else {
	/* Fallback: just find interrupts that aren't in use */
	for (i = 0; i < 16; i++)
	    if ((mask0 & (1 << i)) && (_check_irq(i, 0) == 0))
		mask1 |= (1 << i);
	printk("default");
	/* If scan failed, default to polled status */
	if (!cs_irq && (poll_interval == 0)) poll_interval = HZ;
    }
    printk(") = ");
    
    for (i = 0; i < 16; i++)
	if (mask1 & (1<<i))
	    printk("%s%d", ((mask1 & ((1<<i)-1)) ? "," : ""), i);
    if (mask1 == 0) printk("none!");
    
    return mask1;
}

#endif /* CONFIG_ISA */

/*====================================================================*/

#ifdef CONFIG_ISA

static int __init isa_identify(u_short port, u_short sock)
{
    socket_info_t *s = socket+sockets;
    u_char val;
    int type = -1;

    /* Use the next free entry in the socket table */
    s->ioaddr = port;
    s->psock = sock;
    
    /* Wake up a sleepy Cirrus controller */
    if (wakeup) {
	i365_bclr(s, PD67_MISC_CTL_2, PD67_MC2_SUSPEND);
	/* Pause at least 50 ms */
	mdelay(50);
    }
    
    if ((val = i365_get(s, I365_IDENT)) & 0x70)
	return -1;
    switch (val) {
    case 0x82:
	type = IS_I82365A; break;
    case 0x83:
	type = IS_I82365B; break;
    case 0x84:
	type = IS_I82365DF; break;
    case 0x88: case 0x89: case 0x8a:
	type = IS_IBM; break;
    }
    
    /* Check for Vadem VG-468 chips */
    outb(0x0e, port);
    outb(0x37, port);
    i365_bset(s, VG468_MISC, VG468_MISC_VADEMREV);
    val = i365_get(s, I365_IDENT);
    if (val & I365_IDENT_VADEM) {
	i365_bclr(s, VG468_MISC, VG468_MISC_VADEMREV);
	type = ((val & 7) >= 4) ? IS_VG469 : IS_VG468;
    }

    /* Check for Ricoh chips */
    val = i365_get(s, RF5C_CHIP_ID);
    if ((val == RF5C_CHIP_RF5C296) || (val == RF5C_CHIP_RF5C396))
	type = IS_RF5Cx96;
    
    /* Check for Cirrus CL-PD67xx chips */
    i365_set(s, PD67_CHIP_INFO, 0);
    val = i365_get(s, PD67_CHIP_INFO);
    if ((val & PD67_INFO_CHIP_ID) == PD67_INFO_CHIP_ID) {
	val = i365_get(s, PD67_CHIP_INFO);
	if ((val & PD67_INFO_CHIP_ID) == 0) {
	    type = (val & PD67_INFO_SLOTS) ? IS_PD672X : IS_PD6710;
	    i365_set(s, PD67_EXT_INDEX, 0xe5);
	    if (i365_get(s, PD67_EXT_INDEX) != 0xe5)
		type = IS_VT83C469;
	}
    }
    return type;
} /* isa_identify */

#endif

/*======================================================================

    See if a card is present, powered up, in IO mode, and already
    bound to a (non PC Card) Linux driver.  We leave these alone.

    We make an exception for cards that seem to be serial devices.
    
======================================================================*/

static int __init is_alive(socket_info_t *s)
{
    u_char stat;
    u_short start, stop;
    
    stat = i365_get(s, I365_STATUS);
    start = i365_get_pair(s, I365_IO(0)+I365_W_START);
    stop = i365_get_pair(s, I365_IO(0)+I365_W_STOP);
    if ((stop - start < 0x40) && (stop - start >= 0x07) &&
	((start & 0xfeef) != 0x02e8) && (start >= 0x100) &&
	(stat & I365_CS_DETECT) && (stat & I365_CS_POWERON) &&
	(i365_get(s, I365_INTCTL) & I365_PC_IOCARD) &&
	(i365_get(s, I365_ADDRWIN) & I365_ENA_IO(0)) &&
	(check_region(start, stop-start+1) != 0))
	return 1;
    else
	return 0;
}

/*====================================================================*/

static void __init add_socket(u_short port, int psock, int type)
{
    socket_info_t *s = socket+sockets;
    s->ioaddr = port;
    s->psock = psock;
    s->type = type;
    s->flags = pcic[type].flags;
    if (is_alive(s))
	s->flags |= IS_ALIVE;
    sockets++;
}

static void __init add_pcic(int ns, int type)
{
    u_int mask = 0, i;
    int use_pci = 0, isa_irq = 0;
    socket_info_t *s = &socket[sockets-ns];

    if (s->ioaddr > 0) request_region(s->ioaddr, 2, "i82365");
    
    if (sockets == ns) printk("\n");
    printk(KERN_INFO "  %s", pcic[type].name);
#ifdef CONFIG_PCI
    if (s->flags & IS_UNKNOWN)
	printk(" [0x%04x 0x%04x]", s->vendor, s->device);
    if (s->flags & IS_CARDBUS)
	printk(" PCI-to-CardBus at slot %02x:%02x, mem 0x%08x\n",
	       s->bus, PCI_SLOT(s->devfn), s->cb_phys);
    else if (s->flags & IS_PCI)
	printk(" PCI-to-PCMCIA at slot %02x:%02x, port %#x\n",
	       s->bus, PCI_SLOT(s->devfn), s->ioaddr);
    else
#endif
	printk(" ISA-to-PCMCIA at port %#x ofs 0x%02x\n",
	       s->ioaddr, s->psock*0x40);

#ifdef CONFIG_ISA
    /* Set host options, build basic interrupt mask */
    if (irq_list[0] == -1)
	mask = irq_mask;
    else
	for (i = mask = 0; i < 16; i++)
	    mask |= (1<<irq_list[i]);
#endif
    mask &= I365_MASK & set_bridge_opts(s, ns);
#ifdef CONFIG_ISA
    /* Scan for ISA interrupts */
    mask = isa_scan(s, mask);
#else
    printk(KERN_INFO "    PCI card interrupts,");
#endif
    
#ifdef CONFIG_PCI
    /* Can we use PCI interrupts for card status changes? */
    if (pci_csc && s->cap.pci_irq) {
	for (i = 0; i < ns; i++)
	    if (_check_irq(s[i].cap.pci_irq, SA_SHIRQ)) break;
	if (i == ns) {
	    use_pci = 1;
	    printk(" PCI status changes\n");
	}
    }
#endif
    
#ifdef CONFIG_ISA
    /* Poll if only two interrupts available */
    if (!use_pci && !poll_interval) {
	u_int tmp = (mask & 0xff20);
	tmp = tmp & (tmp-1);
	if ((tmp & (tmp-1)) == 0)
	    poll_interval = HZ;
    }
    /* Only try an ISA cs_irq if this is the first controller */
    if (!use_pci && !grab_irq && (cs_irq || !poll_interval)) {
	/* Avoid irq 12 unless it is explicitly requested */
	u_int cs_mask = mask & ((cs_irq) ? (1<<cs_irq) : ~(1<<12));
	for (cs_irq = 15; cs_irq > 0; cs_irq--)
	    if ((cs_mask & (1 << cs_irq)) &&
		(_check_irq(cs_irq, 0) == 0))
		break;
	if (cs_irq) {
	    grab_irq = 1;
	    isa_irq = cs_irq;
	    printk(" status change on irq %d\n", cs_irq);
	}
    }
#endif
    
    if (!use_pci && !isa_irq) {
	if (poll_interval == 0)
	    poll_interval = HZ;
	printk(" polling interval = %d ms\n",
	       poll_interval * 1000 / HZ);
    }
    
    /* Update socket interrupt information, capabilities */
    for (i = 0; i < ns; i++) {
	s[i].cap.features |= SS_CAP_PCCARD;
	s[i].cap.map_size = 0x1000;
	s[i].cap.irq_mask = mask;
	if (pci_int && s[i].cap.pci_irq)
	    s[i].cap.irq_mask |= (1 << s[i].cap.pci_irq);
	s[i].cs_irq = isa_irq;
#ifdef CONFIG_PCI
	if (s[i].flags & IS_CARDBUS) {
	    s[i].cap.features |= SS_CAP_CARDBUS;
	    cb_set_irq_mode(s+i, pci_csc && s[i].cap.pci_irq,
			    pci_int && s[i].cap.pci_irq);
	}
#endif
    }

} /* add_pcic */

/*====================================================================*/

#ifdef CONFIG_PCI

#ifdef __BEOS__
typedef u_short pci_id_t;
static int pci_lookup(u_int class, pci_id_t *id,
		      u_char *bus, u_char *devfn)
{
    pci_info info;
    while (pci->get_nth_pci_info((*id)++, &info) == 0) {
	if (((info.class_base<<8) + info.class_sub) == class) {
	    *bus = info.bus;
	    *devfn = PCI_DEVFN(info.device, info.function);
	    return 0;
	}
    }
    return -1;
}
#else
#if (LINUX_VERSION_CODE < VERSION(2,1,93))
typedef u_short pci_id_t;
#define pci_lookup(c,i,b,d) pcibios_find_class((c)<<8,(*i)++,b,d)
#else
typedef struct pci_dev *pci_id_t;
static int __init pci_lookup(u_int class, pci_id_t *id,
			     u_char *bus, u_char *devfn)
{
    if ((*id = pci_find_class(class<<8, *id)) != NULL) {
	*bus = (*id)->bus->number;
	*devfn = (*id)->devfn;
	return 0;
    } else return -1;
}
#endif
#endif

static void __init add_pci_bridge(int type, u_short v, u_short d)
{
    socket_info_t *s = &socket[sockets];
    u_int addr, ns;

    if (type == PCIC_COUNT) type = IS_UNK_PCI;
    pci_readl(s, PCI_BASE_ADDRESS_0, &addr);
    addr &= ~0x1;
    pci_writew(s, PCI_COMMAND, CMD_DFLT);
    for (ns = 0; ns < ((type == IS_I82092AA) ? 4 : 2); ns++) {
	s[ns].bus = s->bus; s[ns].devfn = s->devfn;
	s[ns].vendor = v; s[ns].device = d;
	add_socket(addr, ns, type);
    }
    add_pcic(ns, type);
}

static int check_cb_mapping(socket_info_t *s)
{
    /* A few sanity checks to validate the bridge mapping */
    if ((cb_readb(s, 0x800+I365_IDENT) & 0x70) ||
	(cb_readb(s, 0x800+I365_CSC) && cb_readb(s, 0x800+I365_CSC) &&
	 cb_readb(s, 0x800+I365_CSC)) || cb_readl(s, CB_SOCKET_FORCE))
	return 1;
    return 0;
}

static void __init add_cb_bridge(int type, u_short v, u_short d0)
{
    socket_info_t *s = &socket[sockets];
    u_char bus = s->bus, devfn = s->devfn;
    u_short d, ns, i;
    u_char a, b, r, max;
    
    /* PCI bus enumeration is broken on some systems */
    for (ns = 0; ns < sockets; ns++)
	if ((socket[ns].bus == bus) &&
	    (socket[ns].devfn == devfn))
	    return;
    
    if (type == PCIC_COUNT) type = IS_UNK_CARDBUS;
    pci_readb(s, PCI_HEADER_TYPE, &a);
    pci_readb(s, PCI_CLASS_REVISION, &r);
    max = (a & 0x80) ? 8 : 1;
    for (ns = 0; ns < max; ns++, s++, devfn++) {
	s->bus = bus; s->devfn = devfn;
	if (pci_readw(s, PCI_DEVICE_ID, &d) || (d != d0))
	    break;
	s->vendor = v; s->device = d; s->revision = r;

#ifdef __LINUX__
#if (LINUX_VERSION_CODE >= VERSION(2,3,24))
	pci_enable_device(pci_find_slot(bus, devfn));
#endif
#endif
	
	/* Check for PCI power management capabilities */
	pci_readb(s, PCI_STATUS, &a);
	if (a & PCI_STATUS_CAPLIST) {
	    pci_readb(s, PCI_CB_CAPABILITY_POINTER, &b);
	    while (b != 0) {
		pci_readb(s, b+PCI_CAPABILITY_ID, &a);
		if (a == PCI_CAPABILITY_PM) {
		    s->pmcs = b + PCI_PM_CONTROL_STATUS;
		    /* Make sure we're in D0 state */
		    pci_writew(s, s->pmcs, PCI_PMCS_PWR_STATE_D0);
		    break;
		}
		pci_readb(s, b+PCI_NEXT_CAPABILITY, &b);
	    }
	}
	
	/* Map CardBus registers if they are not already mapped */
	pci_writel(s, CB_LEGACY_MODE_BASE, 0);
	pci_readl(s, PCI_BASE_ADDRESS_0, &s->cb_phys);
	if (s->cb_phys == 0) {
	    pci_writew(s, PCI_COMMAND, CMD_DFLT);
	    for (i = 0; i < sizeof(cb_mem_base)/sizeof(u_int); i++) {
		s->cb_phys = cb_mem_base[i];
		s->cb_virt = ioremap(s->cb_phys, 0x1000);
		pci_writel(s, PCI_BASE_ADDRESS_0, s->cb_phys);
		/* Simple sanity checks */
		if (check_cb_mapping(s) == 0) break;
		iounmap(s->cb_virt);
	    }
	    if (i == sizeof(cb_mem_base)/sizeof(u_int)) {
		pci_writel(s, PCI_BASE_ADDRESS_0, 0);
		s->cb_phys = 0; s->cb_virt = NULL;
		printk("\n");
		printk(KERN_NOTICE "  Bridge register mapping failed:"
		       " check cb_mem_base setting\n");
		break;
	    }
	    cb_mem_base[0] = cb_mem_base[i] + PAGE_SIZE;
	} else {
	    s->cb_virt = ioremap(s->cb_phys, 0x1000);
	    if (check_cb_mapping(s) != 0) {
		printk(KERN_NOTICE "  Bad bridge mapping at 0x%08x!\n",
		       s->cb_phys);
		break;
	    }
	}
	
	request_mem_region(s->cb_phys, 0x1000, "i82365");
	add_socket(0, 0, type);
    }
    if (ns == 0) return;
    
    s -= ns;
    if (ns == 2) {
	/* Nasty special check for bad bus mapping */
	pci_readb(s+0, CB_CARDBUS_BUS, &a);
	pci_readb(s+1, CB_CARDBUS_BUS, &b);
	if (a == b) {
	    pci_writeb(s+0, CB_CARDBUS_BUS, 0);
	    pci_writeb(s+1, CB_CARDBUS_BUS, 0);
	}
    }
    add_pcic(ns, type);

    /* Re-do card voltage detection, if needed: this checks for
       card presence with no voltage detect bits set */
    for (a = 0; a < ns; a++)
	if (!(cb_readl(s+a, CB_SOCKET_STATE) & 0x3c86))
	    cb_writel(s+a, CB_SOCKET_FORCE, CB_SF_CVSTEST);
    for (i = 0; i < 200; i++) {
	for (a = 0; a < ns; a++)
	    if (!(cb_readl(s+a, CB_SOCKET_STATE) & 0x3c86)) break;
	if (a == ns) break;
	__set_current_state(TASK_UNINTERRUPTIBLE);
	schedule_timeout(HZ/20);
    }
    if (i == 200)
	printk(KERN_NOTICE "i82365: card voltage interrogation"
	       " timed out!\n");

#ifdef __LINUX__
#if (LINUX_VERSION_CODE >= VERSION(2,1,103))
    /* Set up PCI bus bridge structures if needed */
    for (a = 0; a < ns; a++) {
	struct pci_dev *self = pci_find_slot(bus, s[a].devfn);
	struct pci_bus *child, *parent = self->bus;
	for (child = parent->children; child; child = child->next)
	    if (child->number == s[a].cap.cardbus) break;
	if (!child) {
	    child = kmalloc(sizeof(struct pci_bus), GFP_KERNEL);
	    memset(child, 0, sizeof(struct pci_bus));
	    child->self = self;
	    child->primary = bus;
	    child->number = child->secondary = s[a].cap.cardbus;
	    child->subordinate = s[a].sub_bus;
	    child->parent = parent;
#if (LINUX_VERSION_CODE >= VERSION(2,3,15))
	    child->ops = parent->ops;
#endif
	    child->next = parent->children;
	    parent->children = child;
	}
	s[a].cap.cb_bus = child;
    }
#endif
#endif
}

static void __init pci_probe(u_int class, void (add_fn)
			     (int, u_short, u_short))
{
    socket_info_t *s = &socket[sockets];
    u_short i, v, d;
    pci_id_t id;
    
    id = 0;
    while (pci_lookup(class, &id, &s->bus, &s->devfn) == 0) {
	if (PCI_FUNC(s->devfn) != 0) continue;
	pci_readw(s, PCI_VENDOR_ID, &v);
	pci_readw(s, PCI_DEVICE_ID, &d);
	for (i = 0; i < PCIC_COUNT; i++)
	    if ((pcic[i].vendor == v) && (pcic[i].device == d)) break;
	add_fn(i, v, d);
	s = &socket[sockets];
    }
}

#endif /* CONFIG_PCI */

/*====================================================================*/

#ifdef CONFIG_ISA

static void __init isa_probe(void)
{
    int i, j, sock, k, ns, id;
    ioaddr_t port;

#ifndef __BEOS__
    if (check_region(i365_base, 2) != 0) {
	if (sockets == 0)
	    printk("port conflict at %#x\n", i365_base);
	return;
    }
#endif

    id = isa_identify(i365_base, 0);
    if ((id == IS_I82365DF) && (isa_identify(i365_base, 1) != id)) {
	for (i = 0; i < 4; i++) {
	    if (i == ignore) continue;
	    port = i365_base + ((i & 1) << 2) + ((i & 2) << 1);
	    sock = (i & 1) << 1;
	    if (isa_identify(port, sock) == IS_I82365DF) {
		add_socket(port, sock, IS_VLSI);
		add_pcic(1, IS_VLSI);
	    }
	}
    } else {
	for (i = 0; i < (extra_sockets ? 8 : 4); i += 2) {
	    port = i365_base + 2*(i>>2);
	    sock = (i & 3);
	    id = isa_identify(port, sock);
	    if (id < 0) continue;

	    for (j = ns = 0; j < 2; j++) {
		/* Does the socket exist? */
		if ((ignore == i+j) || (isa_identify(port, sock+j) < 0))
		    continue;
		/* Check for bad socket decode */
		for (k = 0; k <= sockets; k++)
		    i365_set(socket+k, I365_MEM(0)+I365_W_OFF, k);
		for (k = 0; k <= sockets; k++)
		    if (i365_get(socket+k, I365_MEM(0)+I365_W_OFF) != k)
			break;
		if (k <= sockets) break;
		add_socket(port, sock+j, id); ns++;
	    }
	    if (ns != 0) add_pcic(ns, id);
	}
    }
}

#endif

/*======================================================================

    The card status event handler.  This may either be interrupt
    driven or polled.  It monitors mainly for card insert and eject
    events; there are various other kinds of events that can be
    monitored (ready/busy, status change, etc), but they are almost
    never used.
    
======================================================================*/

static irq_ret_t pcic_interrupt IRQ(int irq, void *dev,
				    struct pt_regs *regs)
{
#ifdef __BEOS__
    int irq = (int *)dev - irq_list;
#endif
    int i, j, csc;
    u_int events, active;
#ifdef CONFIG_ISA
    u_long flags = 0;
#endif
    
    DEBUG(4, "i82365: pcic_interrupt(%d)\n", irq);

    for (j = 0; j < 20; j++) {
	active = 0;
	for (i = 0; i < sockets; i++) {
	    socket_info_t *s = &socket[i];
	    if ((s->cs_irq != irq) && (s->cap.pci_irq != irq))
		continue;
	    ISA_LOCK(s, flags);
	    csc = i365_get(s, I365_CSC);
#ifdef CONFIG_PCI
	    if ((s->flags & IS_CARDBUS) &&
		(cb_readl(s,CB_SOCKET_EVENT) & CB_SE_CCD)) {
		cb_writel(s, CB_SOCKET_EVENT, CB_SE_CCD);
		csc |= I365_CSC_DETECT;
	    }
#endif
	    if ((csc == 0) || (!s->handler) ||
		(i365_get(s, I365_IDENT) & 0x70)) {
		ISA_UNLOCK(s, flags);
		continue;
	    }
	    events = (csc & I365_CSC_DETECT) ? SS_DETECT : 0;
	    if (i365_get(s, I365_INTCTL) & I365_PC_IOCARD)
		events |= (csc & I365_CSC_STSCHG) ? SS_STSCHG : 0;
	    else {
		events |= (csc & I365_CSC_BVD1) ? SS_BATDEAD : 0;
		events |= (csc & I365_CSC_BVD2) ? SS_BATWARN : 0;
		events |= (csc & I365_CSC_READY) ? SS_READY : 0;
	    }
	    ISA_UNLOCK(s, flags);
	    DEBUG(2, "i82365: socket %d event 0x%02x\n", i, events);
	    if (events)
		s->handler(s->info, events);
	    active |= events;
	}
	if (!active) break;
    }
    if (j == 20)
	printk(KERN_NOTICE "i82365: infinite loop in interrupt handler\n");

    DEBUG(4, "i82365: interrupt done\n");
    return (irq_ret_t)1;
} /* pcic_interrupt */

static void pcic_interrupt_wrapper(u_long data)
{
#ifdef __BEOS__
    pcic_interrupt IRQ(0, irq_list, NULL);
#else
    pcic_interrupt(0, NULL, NULL);
#endif
    poll_timer.expires = jiffies + poll_interval;
    add_timer(&poll_timer);
}

/*====================================================================*/

static int pcic_register_callback(socket_info_t *s, ss_callback_t *call)
{
    if (call == NULL) {
	s->handler = NULL;
	MOD_DEC_USE_COUNT;
    } else {
	MOD_INC_USE_COUNT;
	s->handler = call->handler;
	s->info = call->info;
    }
    return 0;
} /* pcic_register_callback */

/*====================================================================*/

static int pcic_inquire_socket(socket_info_t *s, socket_cap_t *cap)
{
    *cap = s->cap;
    return 0;
}

/*====================================================================*/

static int i365_get_status(socket_info_t *s, u_int *value)
{
    u_int status;
    
    status = i365_get(s, I365_STATUS);
    *value = ((status & I365_CS_DETECT) == I365_CS_DETECT)
	? SS_DETECT : 0;
    if (i365_get(s, I365_INTCTL) & I365_PC_IOCARD)
	*value |= (status & I365_CS_STSCHG) ? 0 : SS_STSCHG;
    else {
	*value |= (status & I365_CS_BVD1) ? 0 : SS_BATDEAD;
	*value |= (status & I365_CS_BVD2) ? 0 : SS_BATWARN;
    }
    *value |= (status & I365_CS_WRPROT) ? SS_WRPROT : 0;
    *value |= (status & I365_CS_READY) ? SS_READY : 0;
    *value |= (status & I365_CS_POWERON) ? SS_POWERON : 0;

#ifdef CONFIG_PCI
    if (s->flags & IS_CARDBUS) {
	status = cb_readl(s, CB_SOCKET_STATE);
	*value |= (status & CB_SS_32BIT) ? SS_CARDBUS : 0;
	*value |= (status & CB_SS_3VCARD) ? SS_3VCARD : 0;
	*value |= (status & CB_SS_XVCARD) ? SS_XVCARD : 0;
    } else if (s->flags & IS_O2MICRO) {
	status = i365_get(s, O2_MODE_B);
	*value |= (status & O2_MODE_B_VS1) ? 0 : SS_3VCARD;
	*value |= (status & O2_MODE_B_VS2) ? 0 : SS_XVCARD;
    }
#endif
#ifdef CONFIG_ISA
    if (s->type == IS_VG469) {
	status = i365_get(s, VG469_VSENSE);
	if (s->psock & 1) {
	    *value |= (status & VG469_VSENSE_B_VS1) ? 0 : SS_3VCARD;
	    *value |= (status & VG469_VSENSE_B_VS2) ? 0 : SS_XVCARD;
	} else {
	    *value |= (status & VG469_VSENSE_A_VS1) ? 0 : SS_3VCARD;
	    *value |= (status & VG469_VSENSE_A_VS2) ? 0 : SS_XVCARD;
	}
    }
#endif
    
    DEBUG(1, "i82365: GetStatus(%d) = %#4.4x\n", s-socket, *value);
    return 0;
} /* i365_get_status */

/*====================================================================*/

static int i365_get_socket(socket_info_t *s, socket_state_t *state)
{
    u_char reg, vcc, vpp;
    
    reg = i365_get(s, I365_POWER);
    state->flags = (reg & I365_PWR_AUTO) ? SS_PWR_AUTO : 0;
    state->flags |= (reg & I365_PWR_OUT) ? SS_OUTPUT_ENA : 0;
    vcc = reg & I365_VCC_MASK; vpp = reg & I365_VPP1_MASK;
    state->Vcc = state->Vpp = 0;
#ifdef CONFIG_PCI
    if (s->flags & IS_CARDBUS) {
	cb_get_power(s, state);
    } else
#endif
    if (s->flags & IS_CIRRUS) {
	if (i365_get(s, PD67_MISC_CTL_1) & PD67_MC1_VCC_3V) {
	    if (reg & I365_VCC_5V) state->Vcc = 33;
	    if (vpp == I365_VPP1_5V) state->Vpp = 33;
	} else {
	    if (reg & I365_VCC_5V) state->Vcc = 50;
	    if (vpp == I365_VPP1_5V) state->Vpp = 50;
	}
	if (vpp == I365_VPP1_12V) state->Vpp = 120;
    } else if (s->flags & IS_VG_PWR) {
	if (i365_get(s, VG469_VSELECT) & VG469_VSEL_VCC) {
	    if (reg & I365_VCC_5V) state->Vcc = 33;
	    if (vpp == I365_VPP1_5V) state->Vpp = 33;
	} else {
	    if (reg & I365_VCC_5V) state->Vcc = 50;
	    if (vpp == I365_VPP1_5V) state->Vpp = 50;
	}
	if (vpp == I365_VPP1_12V) state->Vpp = 120;
    } else if (s->flags & IS_DF_PWR) {
	if (vcc == I365_VCC_3V) state->Vcc = 33;
	if (vcc == I365_VCC_5V) state->Vcc = 50;
	if (vpp == I365_VPP1_5V) state->Vpp = 50;
	if (vpp == I365_VPP1_12V) state->Vpp = 120;
    } else {
	if (reg & I365_VCC_5V) {
	    state->Vcc = 50;
	    if (vpp == I365_VPP1_5V) state->Vpp = 50;
	    if (vpp == I365_VPP1_12V) state->Vpp = 120;
	}
    }

    /* IO card, RESET flags, IO interrupt */
    reg = i365_get(s, I365_INTCTL);
    state->flags |= (reg & I365_PC_RESET) ? 0 : SS_RESET;
    state->flags |= (reg & I365_PC_IOCARD) ? SS_IOCARD : 0;
#ifdef CONFIG_PCI
    if (cb_get_irq_mode(s) != 0)
	state->io_irq = s->cap.pci_irq;
    else
#endif
	state->io_irq = reg & I365_IRQ_MASK;
    
    /* Card status change mask */
    reg = i365_get(s, I365_CSCINT);
    state->csc_mask = (reg & I365_CSC_DETECT) ? SS_DETECT : 0;
    if (state->flags & SS_IOCARD)
	state->csc_mask |= (reg & I365_CSC_STSCHG) ? SS_STSCHG : 0;
    else {
	state->csc_mask |= (reg & I365_CSC_BVD1) ? SS_BATDEAD : 0;
	state->csc_mask |= (reg & I365_CSC_BVD2) ? SS_BATWARN : 0;
	state->csc_mask |= (reg & I365_CSC_READY) ? SS_READY : 0;
    }
    
    DEBUG(1, "i82365: GetSocket(%d) = flags %#3.3x, Vcc %d, Vpp %d, "
	  "io_irq %d, csc_mask %#2.2x\n", s-socket, state->flags,
	  state->Vcc, state->Vpp, state->io_irq, state->csc_mask);
    return 0;
} /* i365_get_socket */

/*====================================================================*/

static int i365_set_socket(socket_info_t *s, socket_state_t *state)
{
    u_char reg;
    
    DEBUG(1, "i82365: SetSocket(%d, flags %#3.3x, Vcc %d, Vpp %d, "
	  "io_irq %d, csc_mask %#2.2x)\n", s-socket, state->flags,
	  state->Vcc, state->Vpp, state->io_irq, state->csc_mask);
    
    /* First set global controller options */
#ifdef CONFIG_PCI
    if ((s->flags & IS_CARDBUS) && s->cap.pci_irq)
	cb_set_irq_mode(s, pci_csc, (s->cap.pci_irq == state->io_irq));
    s->bcr &= ~CB_BCR_CB_RESET;
#endif
    set_bridge_state(s);
    
    /* IO card, RESET flag, IO interrupt */
    reg = s->intr;
    if (state->io_irq != s->cap.pci_irq) reg |= state->io_irq;
    reg |= (state->flags & SS_RESET) ? 0 : I365_PC_RESET;
    reg |= (state->flags & SS_IOCARD) ? I365_PC_IOCARD : 0;
    i365_set(s, I365_INTCTL, reg);
    
    reg = I365_PWR_NORESET;
    if (state->flags & SS_PWR_AUTO) reg |= I365_PWR_AUTO;
    if (state->flags & SS_OUTPUT_ENA) reg |= I365_PWR_OUT;

#ifdef CONFIG_PCI
    if (s->flags & IS_CARDBUS) {
	cb_set_power(s, state);
	reg |= i365_get(s, I365_POWER) & (I365_VCC_MASK|I365_VPP1_MASK);
    } else
#endif
    if (s->flags & IS_CIRRUS) {
	if (state->Vpp != 0) {
	    if (state->Vpp == 120)
		reg |= I365_VPP1_12V;
	    else if (state->Vpp == state->Vcc)
		reg |= I365_VPP1_5V;
	    else return -EINVAL;
	}
	if (state->Vcc != 0) {
	    reg |= I365_VCC_5V;
	    if (state->Vcc == 33)
		i365_bset(s, PD67_MISC_CTL_1, PD67_MC1_VCC_3V);
	    else if (state->Vcc == 50)
		i365_bclr(s, PD67_MISC_CTL_1, PD67_MC1_VCC_3V);
	    else return -EINVAL;
	}
    } else if (s->flags & IS_VG_PWR) {
	if (state->Vpp != 0) {
	    if (state->Vpp == 120)
		reg |= I365_VPP1_12V;
	    else if (state->Vpp == state->Vcc)
		reg |= I365_VPP1_5V;
	    else return -EINVAL;
	}
	if (state->Vcc != 0) {
	    reg |= I365_VCC_5V;
	    if (state->Vcc == 33)
		i365_bset(s, VG469_VSELECT, VG469_VSEL_VCC);
	    else if (state->Vcc == 50)
		i365_bclr(s, VG469_VSELECT, VG469_VSEL_VCC);
	    else return -EINVAL;
	}
    } else if (s->flags & IS_DF_PWR) {
	switch (state->Vcc) {
	case 0:		break;
	case 33:   	reg |= I365_VCC_3V; break;
	case 50:	reg |= I365_VCC_5V; break;
	default:	return -EINVAL;
	}
	switch (state->Vpp) {
	case 0:		break;
	case 50:   	reg |= I365_VPP1_5V; break;
	case 120:	reg |= I365_VPP1_12V; break;
	default:	return -EINVAL;
	}
    } else {
	switch (state->Vcc) {
	case 0:		break;
	case 50:	reg |= I365_VCC_5V; break;
	default:	return -EINVAL;
	}
	switch (state->Vpp) {
	case 0:		break;
	case 50:	reg |= I365_VPP1_5V | I365_VPP2_5V; break;
	case 120:	reg |= I365_VPP1_12V | I365_VPP2_12V; break;
	default:	return -EINVAL;
	}
    }
    
    if (reg != i365_get(s, I365_POWER))
	i365_set(s, I365_POWER, reg);

    /* Card status change interrupt mask */
    reg = s->cs_irq << 4;
    if (state->csc_mask & SS_DETECT) reg |= I365_CSC_DETECT;
    if (state->flags & SS_IOCARD) {
	if (state->csc_mask & SS_STSCHG) reg |= I365_CSC_STSCHG;
    } else {
	if (state->csc_mask & SS_BATDEAD) reg |= I365_CSC_BVD1;
	if (state->csc_mask & SS_BATWARN) reg |= I365_CSC_BVD2;
	if (state->csc_mask & SS_READY) reg |= I365_CSC_READY;
    }
    i365_set(s, I365_CSCINT, reg);
    i365_get(s, I365_CSC);
#ifdef CONFIG_PCI
    if (s->flags & IS_CARDBUS) {
	if (s->cs_irq || (pci_csc && s->cap.pci_irq))
	    cb_writel(s, CB_SOCKET_MASK, CB_SM_CCD);
	cb_writel(s, CB_SOCKET_EVENT, -1);
    }
#endif

    return 0;
} /* i365_set_socket */

/*====================================================================*/

static int i365_get_io_map(socket_info_t *s, struct pccard_io_map *io)
{
    u_char map, ioctl, addr;
    
    map = io->map;
    if (map > 1) return -EINVAL;
    io->start = i365_get_pair(s, I365_IO(map)+I365_W_START);
    io->stop = i365_get_pair(s, I365_IO(map)+I365_W_STOP);
    ioctl = i365_get(s, I365_IOCTL);
    addr = i365_get(s, I365_ADDRWIN);
    io->speed = (ioctl & I365_IOCTL_WAIT(map)) ? cycle_time : 0;
    io->flags  = (addr & I365_ENA_IO(map)) ? MAP_ACTIVE : 0;
    io->flags |= (ioctl & I365_IOCTL_0WS(map)) ? MAP_0WS : 0;
    io->flags |= (ioctl & I365_IOCTL_16BIT(map)) ? MAP_16BIT : 0;
    io->flags |= (ioctl & I365_IOCTL_IOCS16(map)) ? MAP_AUTOSZ : 0;
    DEBUG(1, "i82365: GetIOMap(%d, %d) = %#2.2x, %d ns, %#4.4x-%#4.4x\n",
	  s-socket, map, io->flags, io->speed, io->start, io->stop);
    return 0;
} /* i365_get_io_map */

/*====================================================================*/

static int i365_set_io_map(socket_info_t *s, struct pccard_io_map *io)
{
    u_char map, ioctl;
    
    DEBUG(1, "i82365: SetIOMap(%d, %d, %#2.2x, %d ns, %#4.4x-%#4.4x)\n",
	  s-socket, io->map, io->flags, io->speed, io->start, io->stop);
    map = io->map;
    if ((map > 1) || (io->start > 0xffff) || (io->stop > 0xffff) ||
	(io->stop < io->start)) return -EINVAL;
    /* Turn off the window before changing anything */
    if (i365_get(s, I365_ADDRWIN) & I365_ENA_IO(map))
	i365_bclr(s, I365_ADDRWIN, I365_ENA_IO(map));
    i365_set_pair(s, I365_IO(map)+I365_W_START, io->start);
    i365_set_pair(s, I365_IO(map)+I365_W_STOP, io->stop);
    ioctl = i365_get(s, I365_IOCTL) & ~I365_IOCTL_MASK(map);
    if (io->speed) ioctl |= I365_IOCTL_WAIT(map);
    if (io->flags & MAP_0WS) ioctl |= I365_IOCTL_0WS(map);
    if (io->flags & MAP_16BIT) ioctl |= I365_IOCTL_16BIT(map);
    if (io->flags & MAP_AUTOSZ) ioctl |= I365_IOCTL_IOCS16(map);
    i365_set(s, I365_IOCTL, ioctl);
    /* Turn on the window if necessary */
    if (io->flags & MAP_ACTIVE)
	i365_bset(s, I365_ADDRWIN, I365_ENA_IO(map));
    return 0;
} /* i365_set_io_map */

/*====================================================================*/

static int i365_get_mem_map(socket_info_t *s, struct pccard_mem_map *mem)
{
    u_short base, i;
    u_char map, addr;
    
    map = mem->map;
    if (map > 4) return -EINVAL;
    addr = i365_get(s, I365_ADDRWIN);
    mem->flags = (addr & I365_ENA_MEM(map)) ? MAP_ACTIVE : 0;
    base = I365_MEM(map);
    
    i = i365_get_pair(s, base+I365_W_START);
    mem->flags |= (i & I365_MEM_16BIT) ? MAP_16BIT : 0;
    mem->flags |= (i & I365_MEM_0WS) ? MAP_0WS : 0;
    mem->sys_start = ((u_long)(i & 0x0fff) << 12);
    
    i = i365_get_pair(s, base+I365_W_STOP);
    mem->speed  = (i & I365_MEM_WS0) ? 1 : 0;
    mem->speed += (i & I365_MEM_WS1) ? 2 : 0;
    mem->speed *= cycle_time;
    mem->sys_stop = ((u_long)(i & 0x0fff) << 12) + 0x0fff;
    
    i = i365_get_pair(s, base+I365_W_OFF);
    mem->flags |= (i & I365_MEM_WRPROT) ? MAP_WRPROT : 0;
    mem->flags |= (i & I365_MEM_REG) ? MAP_ATTRIB : 0;
    mem->card_start = ((u_int)(i & 0x3fff) << 12) + mem->sys_start;
    mem->card_start &= 0x3ffffff;

#ifdef CONFIG_PCI
    /* Take care of high byte, for PCI controllers */
    if (s->type == IS_PD6729) {
	i365_set(s, PD67_EXT_INDEX, PD67_MEM_PAGE(map));
	addr = i365_get(s, PD67_EXT_DATA) << 24;
    } else if (s->flags & IS_CARDBUS) {
	addr = i365_get(s, CB_MEM_PAGE(map)) << 24;
	mem->sys_stop += addr; mem->sys_start += addr;
    }
#endif
    
    DEBUG(1, "i82365: GetMemMap(%d, %d) = %#2.2x, %d ns, %#5.5lx-%#5."
	  "5lx, %#5.5x\n", s-socket, mem->map, mem->flags, mem->speed,
	  mem->sys_start, mem->sys_stop, mem->card_start);
    return 0;
} /* i365_get_mem_map */

/*====================================================================*/
  
static int i365_set_mem_map(socket_info_t *s, struct pccard_mem_map *mem)
{
    u_short base, i;
    u_char map;
    
    DEBUG(1, "i82365: SetMemMap(%d, %d, %#2.2x, %d ns, %#5.5lx-%#5.5"
	  "lx, %#5.5x)\n", s-socket, mem->map, mem->flags, mem->speed,
	  mem->sys_start, mem->sys_stop, mem->card_start);

    map = mem->map;
    if ((map > 4) || (mem->card_start > 0x3ffffff) ||
	(mem->sys_start > mem->sys_stop) || (mem->speed > 1000))
	return -EINVAL;
    if (!(s->flags & (IS_PCI | IS_CARDBUS)) &&
	((mem->sys_start > 0xffffff) || (mem->sys_stop > 0xffffff)))
	return -EINVAL;
	
    /* Turn off the window before changing anything */
    if (i365_get(s, I365_ADDRWIN) & I365_ENA_MEM(map))
	i365_bclr(s, I365_ADDRWIN, I365_ENA_MEM(map));

#ifdef CONFIG_PCI
    /* Take care of high byte, for PCI controllers */
    if (s->type == IS_PD6729) {
	i365_set(s, PD67_EXT_INDEX, PD67_MEM_PAGE(map));
	i365_set(s, PD67_EXT_DATA, (mem->sys_start >> 24));
    } else if (s->flags & IS_CARDBUS)
	i365_set(s, CB_MEM_PAGE(map), mem->sys_start >> 24);
#endif
    
    base = I365_MEM(map);
    i = (mem->sys_start >> 12) & 0x0fff;
    if (mem->flags & MAP_16BIT) i |= I365_MEM_16BIT;
    if (mem->flags & MAP_0WS) i |= I365_MEM_0WS;
    i365_set_pair(s, base+I365_W_START, i);
    
    i = (mem->sys_stop >> 12) & 0x0fff;
    switch (mem->speed / cycle_time) {
    case 0:	break;
    case 1:	i |= I365_MEM_WS0; break;
    case 2:	i |= I365_MEM_WS1; break;
    default:	i |= I365_MEM_WS1 | I365_MEM_WS0; break;
    }
    i365_set_pair(s, base+I365_W_STOP, i);
    
    i = ((mem->card_start - mem->sys_start) >> 12) & 0x3fff;
    if (mem->flags & MAP_WRPROT) i |= I365_MEM_WRPROT;
    if (mem->flags & MAP_ATTRIB) i |= I365_MEM_REG;
    i365_set_pair(s, base+I365_W_OFF, i);
    
    /* Turn on the window if necessary */
    if (mem->flags & MAP_ACTIVE)
	i365_bset(s, I365_ADDRWIN, I365_ENA_MEM(map));
    return 0;
} /* i365_set_mem_map */

/*======================================================================

    All the stuff that is strictly for Cardbus cards goes here.

======================================================================*/

#ifdef CONFIG_CARDBUS

static int cb_get_status(socket_info_t *s, u_int *value)
{
    u_int state = cb_readl(s, CB_SOCKET_STATE);
    *value = (state & CB_SS_32BIT) ? SS_CARDBUS : 0;
    *value |= (state & CB_SS_CCD) ? 0 : SS_DETECT;
    *value |= (state & CB_SS_CSTSCHG) ? SS_STSCHG : 0;
    *value |= (state & CB_SS_PWRCYCLE) ? (SS_POWERON|SS_READY) : 0;
    *value |= (state & CB_SS_3VCARD) ? SS_3VCARD : 0;
    *value |= (state & CB_SS_XVCARD) ? SS_XVCARD : 0;
    DEBUG(1, "yenta: GetStatus(%d) = %#4.4x\n", s-socket, *value);
    return 0;
} /* cb_get_status */

static int cb_get_socket(socket_info_t *s, socket_state_t *state)
{
    u_short bcr;

    cb_get_power(s, state);
    pci_readw(s, CB_BRIDGE_CONTROL, &bcr);
    state->flags |= (bcr & CB_BCR_CB_RESET) ? SS_RESET : 0;
    if (cb_get_irq_mode(s) != 0)
	state->io_irq = s->cap.pci_irq;
    else
	state->io_irq = i365_get(s, I365_INTCTL) & I365_IRQ_MASK;
    DEBUG(1, "yenta: GetSocket(%d) = flags %#3.3x, Vcc %d, Vpp %d, "
	  "io_irq %d, csc_mask %#2.2x\n", s-socket, state->flags,
	  state->Vcc, state->Vpp, state->io_irq, state->csc_mask);
    return 0;
} /* cb_get_socket */

static int cb_set_socket(socket_info_t *s, socket_state_t *state)
{
    u_int reg;
    
    DEBUG(1, "yenta: SetSocket(%d, flags %#3.3x, Vcc %d, Vpp %d, "
	  "io_irq %d, csc_mask %#2.2x)\n", s-socket, state->flags,
	  state->Vcc, state->Vpp, state->io_irq, state->csc_mask);
    
    /* First set global controller options */
    if (s->cap.pci_irq)
	cb_set_irq_mode(s, pci_csc,
			(s->cap.pci_irq == state->io_irq));
    s->bcr &= ~CB_BCR_CB_RESET;
    s->bcr |= (state->flags & SS_RESET) ? CB_BCR_CB_RESET : 0;
    set_bridge_state(s);
    
    cb_set_power(s, state);
    
    /* Handle IO interrupt using ISA routing */
    reg = s->intr;
    if (state->io_irq != s->cap.pci_irq) reg |= state->io_irq;
    i365_set(s, I365_INTCTL, reg);
    
    /* Handle CSC mask */
    if (!s->cs_irq && (!pci_csc || !s->cap.pci_irq))
	return 0;
    reg = (s->cs_irq << 4);
    if (state->csc_mask & SS_DETECT) reg |= I365_CSC_DETECT;
    i365_set(s, I365_CSCINT, reg);
    i365_get(s, I365_CSC);
    cb_writel(s, CB_SOCKET_MASK, CB_SM_CCD);
    cb_writel(s, CB_SOCKET_EVENT, -1);
    
    return 0;
} /* cb_set_socket */

static int cb_get_bridge(socket_info_t *s, struct cb_bridge_map *m)
{
    u_char map = m->map;

    if (map > 1) return -EINVAL;
    m->flags &= MAP_IOSPACE;
    map += (m->flags & MAP_IOSPACE) ? 2 : 0;
    pci_readl(s, CB_MEM_BASE(map), &m->start);
    pci_readl(s, CB_MEM_LIMIT(map), &m->stop);
    if (m->start || m->stop) {
	m->flags |= MAP_ACTIVE;
	m->stop |= (map > 1) ? 3 : 0x0fff;
    }
    if (map > 1) {
	u_short bcr;
	pci_readw(s, CB_BRIDGE_CONTROL, &bcr);
	m->flags |= (bcr & CB_BCR_PREFETCH(map)) ? MAP_PREFETCH : 0;
    }
    DEBUG(1, "yenta: GetBridge(%d, %d) = %#2.2x, %#4.4x-%#4.4x\n",
	  s-socket, map, m->flags, m->start, m->stop);
    return 0;
}

static int cb_set_bridge(socket_info_t *s, struct cb_bridge_map *m)
{
    u_char map;
    
    DEBUG(1, "yenta: SetBridge(%d, %d, %#2.2x, %#4.4x-%#4.4x)\n",
	  s-socket, m->map, m->flags, m->start, m->stop);
    map = m->map;
    if (!(s->flags & IS_CARDBUS) || (map > 1) || (m->stop < m->start))
	return -EINVAL;
    if (m->flags & MAP_IOSPACE) {
	if ((m->stop > 0xffff) || (m->start & 3) ||
	    ((m->stop & 3) != 3))
	    return -EINVAL;
	map += 2;
    } else {
	u_short bcr;
	if ((m->start & 0x0fff) || ((m->stop & 0x0fff) != 0x0fff))
	    return -EINVAL;
	pci_readw(s, CB_BRIDGE_CONTROL, &bcr);
	bcr &= ~CB_BCR_PREFETCH(map);
	bcr |= (m->flags & MAP_PREFETCH) ? CB_BCR_PREFETCH(map) : 0;
	pci_writew(s, CB_BRIDGE_CONTROL, bcr);
    }
    if (m->flags & MAP_ACTIVE) {
	pci_writel(s, CB_MEM_BASE(map), m->start);
	pci_writel(s, CB_MEM_LIMIT(map), m->stop);
    } else {
	pci_writel(s, CB_MEM_BASE(map), 0);
	pci_writel(s, CB_MEM_LIMIT(map), 0);
    }
    return 0;
}

#endif /* CONFIG_CARDBUS */

/*======================================================================

    Routines for accessing socket information and register dumps via
    /proc/bus/pccard/...
    
======================================================================*/

#ifdef HAS_PROC_BUS

static int proc_read_info(char *buf, char **start, off_t pos,
			  int count, int *eof, void *data)
{
    socket_info_t *s = data;
    char *p = buf;
    p += sprintf(p, "type:     %s\npsock:    %d\n",
		 pcic[s->type].name, s->psock);
#ifdef CONFIG_PCI
    if (s->flags & (IS_PCI|IS_CARDBUS))
	p += sprintf(p, "bus:      %02x\ndevfn:    %02x.%1x\n",
		     s->bus, PCI_SLOT(s->devfn), PCI_FUNC(s->devfn));
    if (s->flags & IS_CARDBUS)
	p += sprintf(p, "cardbus:  %02x\n", s->cap.cardbus);
#endif
    return (p - buf);
}

static int proc_read_exca(char *buf, char **start, off_t pos,
			  int count, int *eof, void *data)
{
    socket_info_t *s = data;
    char *p = buf;
    int i, top;
    
#ifdef CONFIG_ISA
    u_long flags = 0;
#endif
    ISA_LOCK(s, flags);
    top = 0x40;
    if (s->flags & IS_CARDBUS)
	top = (s->flags & IS_CIRRUS) ? 0x140 : 0x50;
    for (i = 0; i < top; i += 4) {
	if (i == 0x50) {
	    p += sprintf(p, "\n");
	    i = 0x100;
	}
	p += sprintf(p, "%02x %02x %02x %02x%s",
		     i365_get(s,i), i365_get(s,i+1),
		     i365_get(s,i+2), i365_get(s,i+3),
		     ((i % 16) == 12) ? "\n" : " ");
    }
    ISA_UNLOCK(s, flags);
    return (p - buf);
}

#ifdef CONFIG_PCI
static int proc_read_pci(char *buf, char **start, off_t pos,
			 int count, int *eof, void *data)
{
    socket_info_t *s = data;
    char *p = buf;
    u_int a, b, c, d;
    int i;
    
    for (i = 0; i < 0xc0; i += 0x10) {
	pci_readl(s, i, &a);
	pci_readl(s, i+4, &b);
	pci_readl(s, i+8, &c);
	pci_readl(s, i+12, &d);
	p += sprintf(p, "%08x %08x %08x %08x\n", a, b, c, d);
    }
    return (p - buf);
}

static int proc_read_cardbus(char *buf, char **start, off_t pos,
			     int count, int *eof, void *data)
{
    socket_info_t *s = data;
    char *p = buf;
    int i, top;

    top = (s->flags & IS_O2MICRO) ? 0x30 : 0x20;
    for (i = 0; i < top; i += 0x10)
	p += sprintf(p, "%08x %08x %08x %08x\n",
		     cb_readl(s,i+0x00), cb_readl(s,i+0x04),
		     cb_readl(s,i+0x08), cb_readl(s,i+0x0c));
    return (p - buf);
}
#endif

static void pcic_proc_setup(socket_info_t *s, struct proc_dir_entry *base)
{
    struct proc_dir_entry *ent;
    ent = create_proc_entry("info", 0, base);
    if (ent) {
	ent->read_proc = proc_read_info;
	ent->data = s;
    }
    ent = create_proc_entry("exca", 0, base);
    if (ent) {
	ent->read_proc = proc_read_exca;
	ent->data = s;
    }
#ifdef CONFIG_PCI
    if (s->flags & (IS_PCI|IS_CARDBUS)) {
	ent = create_proc_entry("pci", 0, base);
	if (ent) {
	    ent->read_proc = proc_read_pci;
	    ent->data = s;
	}
    }
    if (s->flags & IS_CARDBUS) {
	ent = create_proc_entry("cardbus", 0, base);
	if (ent) {
	    ent->read_proc = proc_read_cardbus;
	    ent->data = s;
	}
    }
#endif
    s->proc = base;
}

static void pcic_proc_remove(socket_info_t *s)
{
    struct proc_dir_entry *base = s->proc;
    if (base == NULL) return;
    remove_proc_entry("info", base);
    remove_proc_entry("exca", base);
#ifdef CONFIG_PCI
    if (s->flags & (IS_PCI|IS_CARDBUS))
	remove_proc_entry("pci", base);
    if (s->flags & IS_CARDBUS)
	remove_proc_entry("cardbus", base);
#endif
}

#endif /* HAS_PROC_BUS */

/*====================================================================*/

typedef int (*subfn_t)(socket_info_t *, void *);

static subfn_t pcic_service_table[] = {
    (subfn_t)&pcic_register_callback,
    (subfn_t)&pcic_inquire_socket,
    (subfn_t)&i365_get_status,
    (subfn_t)&i365_get_socket,
    (subfn_t)&i365_set_socket,
    (subfn_t)&i365_get_io_map,
    (subfn_t)&i365_set_io_map,
    (subfn_t)&i365_get_mem_map,
    (subfn_t)&i365_set_mem_map,
#ifdef CONFIG_CARDBUS
    (subfn_t)&cb_get_bridge,
    (subfn_t)&cb_set_bridge,
#else
    NULL, NULL,
#endif
#ifdef HAS_PROC_BUS
    (subfn_t)&pcic_proc_setup
#endif
};

#define NFUNC (sizeof(pcic_service_table)/sizeof(subfn_t))

static int pcic_service(u_int sock, u_int cmd, void *arg)
{
    socket_info_t *s = &socket[sock];
    subfn_t fn;
    int ret;
#ifdef CONFIG_ISA
    u_long flags = 0;
#endif
    
    DEBUG(2, "pcic_ioctl(%d, %d, 0x%p)\n", sock, cmd, arg);

    if (cmd >= NFUNC)
	return -EINVAL;

    if (s->flags & IS_ALIVE) {
	if (cmd == SS_GetStatus)
	    *(u_int *)arg = 0;
	return -EINVAL;
    }
    
    fn = pcic_service_table[cmd];
#ifdef CONFIG_CARDBUS
    if ((s->flags & IS_CARDBUS) &&
	(cb_readl(s, CB_SOCKET_STATE) & CB_SS_32BIT)) {
	if (cmd == SS_GetStatus)
	    fn = (subfn_t)&cb_get_status;
	else if (cmd == SS_GetSocket)
	    fn = (subfn_t)&cb_get_socket;
	else if (cmd == SS_SetSocket)
	    fn = (subfn_t)&cb_set_socket;
    }
#endif

    ISA_LOCK(s, flags);
    ret = (fn == NULL) ? -EINVAL : fn(s, arg);
    ISA_UNLOCK(s, flags);
    return ret;
} /* pcic_service */

/*====================================================================*/

static int __init init_i82365(void)
{
#ifdef __LINUX__
    servinfo_t serv;
    CardServices(GetCardServicesInfo, &serv);
    if (serv.Revision != CS_RELEASE_CODE) {
	printk(KERN_NOTICE "i82365: Card Services release "
	       "does not match!\n");
	return -1;
    }
#endif
    DEBUG(0, "%s\n", version);
    printk(KERN_INFO "Intel PCIC probe: ");
    sockets = 0;

    ACQUIRE_RESOURCE_LOCK;
    
#ifdef CONFIG_PCI
    if (do_pci_probe && pcibios_present()) {
	pci_probe(PCI_CLASS_BRIDGE_CARDBUS, add_cb_bridge);
	pci_probe(PCI_CLASS_BRIDGE_PCMCIA, add_pci_bridge);
    }
#endif

#ifdef CONFIG_ISA
    isa_probe();
#endif

    RELEASE_RESOURCE_LOCK;
    
    if (sockets == 0) {
	printk("not found.\n");
	return -ENODEV;
    }

    /* Set up interrupt handler(s) */
#ifdef CONFIG_ISA
    if (grab_irq != 0)
	_request_irq(cs_irq, pcic_interrupt, 0, "i82365");
#endif
#ifdef CONFIG_PCI
    if (pci_csc) {
	u_int i, irq, mask = 0;
	for (i = 0; i < sockets; i++) {
	    irq = socket[i].cap.pci_irq;
	    if (irq && !(mask & (1<<irq)))
		_request_irq(irq, pcic_interrupt, SA_SHIRQ, "i82365");
	    mask |= (1<<irq);
	}
    }
#endif
    
    if (register_ss_entry(sockets, &pcic_service) != 0)
	printk(KERN_NOTICE "i82365: register_ss_entry() failed\n");

    /* Finally, schedule a polling interrupt */
    if (poll_interval != 0) {
	poll_timer.function = pcic_interrupt_wrapper;
	poll_timer.data = 0;
	poll_timer.prev = poll_timer.next = NULL;
    	poll_timer.expires = jiffies + poll_interval;
	add_timer(&poll_timer);
    }
    
    return 0;
    
} /* init_i82365 */

static void __exit exit_i82365(void)
{
    int i;
    unregister_ss_entry(&pcic_service);
    if (poll_interval != 0)
	del_timer(&poll_timer);
#ifdef CONFIG_ISA
    if (grab_irq != 0)
	_free_irq(cs_irq, pcic_interrupt);
#endif
#ifdef CONFIG_PCI
    if (pci_csc) {
	u_int irq, mask = 0;
	for (i = 0; i < sockets; i++) {
	    irq = socket[i].cap.pci_irq;
	    if (irq && !(mask & (1<<irq)))
		_free_irq(irq, pcic_interrupt);
	    mask |= (1<<irq);
	}
    }
#endif
    for (i = 0; i < sockets; i++) {
	socket_info_t *s = &socket[i];
	/* Turn off all interrupt sources! */
	i365_set(s, I365_CSCINT, 0);
#ifdef HAS_PROC_BUS
	pcic_proc_remove(s);
#endif
#ifdef CONFIG_PCI
	if (s->flags & IS_CARDBUS)
	    cb_writel(s, CB_SOCKET_MASK, 0);
	if (s->cb_virt) {
	    iounmap(s->cb_virt);
	    release_mem_region(s->cb_phys, 0x1000);
	} else
#endif
	    release_region(s->ioaddr, 2);
    }
} /* exit_i82365 */

#ifdef __LINUX__

module_init(init_i82365);
module_exit(exit_i82365);

#endif

/*====================================================================*/

#ifdef __BEOS__

static status_t std_ops(int32 op)
{
    int ret;
    switch (op) {
    case B_MODULE_INIT:
	ret = get_module(CS_SOCKET_MODULE_NAME, (struct module_info **)&cs);
	if (ret != B_OK) return ret;
	ret = get_module(B_ISA_MODULE_NAME, (struct module_info **)&isa);
	if (ret != B_OK) return ret;
	ret = get_module(B_PCI_MODULE_NAME, (struct module_info **)&pci);
	if (ret != B_OK) return ret;
	return pcic_init();
	break;
    case B_MODULE_UNINIT:
	exit_i82365();
	if (pci) put_module(B_PCI_MODULE_NAME);
	if (isa) put_module(B_ISA_MODULE_NAME);
	if (cs) put_module(CS_SOCKET_MODULE_NAME);
	break;
    }
    return B_OK;
}

static module_info pcic_mod_info = {
    SS_MODULE_NAME("i82365"), 0, &std_ops
};

_EXPORT module_info *modules[] = {
    &pcic_mod_info,
    NULL
};

#endif
