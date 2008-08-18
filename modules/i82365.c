/*======================================================================

    Device driver for Intel 82365 and compatible PC Card controllers,
    and Yenta-compatible PCI-to-CardBus controllers.

    i82365.c 1.183 1998/05/25 11:26:46

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

#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/string.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/bitops.h>
#include <asm/segment.h>
#include <asm/system.h>

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/malloc.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>

#include <pcmcia/version.h>
#include <pcmcia/ss.h>

/* ISA-bus controllers */
#include "i82365.h"
#include "pd67xx.h"
#include "vg468.h"
#include "ricoh.h"
#include "o2micro.h"

/* PCI-bus controllers */
#include "yenta.h"
#include "rl5c466.h"
#include "ti113x.h"
#include "smc34c90.h"
#include "pd6832.h"
#include "topic.h"

#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include "rsrc_mgr.h"

#ifdef PCMCIA_DEBUG
static int pc_debug = PCMCIA_DEBUG;
MODULE_PARM(pc_debug, "i");
#define DEBUG(n, args...) if (pc_debug>(n)) printk(KERN_DEBUG args)
static const char *version =
"i82365.c 1.183 1998/05/25 11:26:46 (David Hinds)";
#else
#define DEBUG(n, args...)
#endif

/*====================================================================*/

/* Parameters that can be set with 'insmod' */

#ifdef CONFIG_ISA
/* Default base address for i82365sl and other ISA chips */
static int i365_base = 0x3e0;
/* Should we probe at 0x3e2 for an extra ISA controller? */
static int extra_sockets = 0;
/* Specify a socket number to ignore */
static int ignore = -1;
/* Probe for safe interrupts? */
static int do_scan = 1;
/* Bit map or list of interrupts to choose from */
static u_int irq_mask = 0xffff;
static int irq_list[16] = { -1 };
/* The card status change interrupt -- 0 means autoselect */
static int cs_irq = 0;
#endif

/* Poll status interval -- 0 means default to interrupt */
static int poll_interval = 0;

/* External clock time, in nanoseconds.  120 ns = 8.33 MHz */
static int cycle_time = 120;

/* Cirrus options */
static int has_dma = -1;
static int has_led = -1;
static int has_ring = -1;
static int dynamic_mode = 0;
static int freq_bypass = -1;
static int setup_time = -1;
static int cmd_time = -1;
static int recov_time = -1;

#ifdef CONFIG_ISA
/* Vadem options */
static int async_clock = -1;
static int cable_mode = -1;
static int wakeup = 0;
#endif

#ifdef CONFIG_ISA
MODULE_PARM(i365_base, "i");
MODULE_PARM(ignore, "i");
MODULE_PARM(extra_sockets, "i");
MODULE_PARM(do_scan, "i");
MODULE_PARM(irq_mask, "i");
MODULE_PARM(irq_list, "1-16i");
MODULE_PARM(cs_irq, "i");
MODULE_PARM(async_clock, "i");
MODULE_PARM(cable_mode, "i");
MODULE_PARM(wakeup, "i");
#endif

MODULE_PARM(poll_interval, "i");
MODULE_PARM(cycle_time, "i");
MODULE_PARM(has_dma, "i");
MODULE_PARM(has_led, "i");
MODULE_PARM(has_ring, "i");
MODULE_PARM(dynamic_mode, "i");
MODULE_PARM(freq_bypass, "i");
MODULE_PARM(setup_time, "i");
MODULE_PARM(cmd_time, "i");
MODULE_PARM(recov_time, "i");

#ifdef CONFIG_PCI
/* Default memory base address for CardBus controllers */
static u_int cb_mem_base = 0x68000000;
static int fast_pci = -1;
static int hold_time = -1;
/* Override BIOS interrupt routing mode? */
static int irq_mode = -1;
static int has_clkrun = -1;
static int clkrun_sel = -1;
static int pci_latency = -1;
static int cb_latency = -1;
static int cb_bus_base = 0x20;
MODULE_PARM(cb_mem_base, "i");
MODULE_PARM(fast_pci, "i");
MODULE_PARM(hold_time, "i");
MODULE_PARM(irq_mode, "i");
MODULE_PARM(has_clkrun, "i");
MODULE_PARM(clkrun_sel, "i");
MODULE_PARM(pci_latency, "i");
MODULE_PARM(cb_latency, "i");
MODULE_PARM(cb_bus_base, "i");
#endif

#ifdef CONFIG_ISA
#ifdef CONFIG_PCI
/* PCI card status change interrupts? */
static int pci_csc = 0;
/* PCI IO card functional interrupts? */
static int pci_int = 0;
MODULE_PARM(pci_csc, "i");
MODULE_PARM(pci_int, "i");
#else
#define pci_csc		0
#define pci_int		0
#endif
#else /* no ISA */
#ifdef CONFIG_PCI
#define pci_csc		0
#define pci_int		1
#else
#error "No bus architectures defined!"
#endif
#endif

/*====================================================================*/

typedef struct pd672x_state_t {
    u_char		misc1, misc2;
    u_char		timer[6];
} pd672x_state_t;

typedef struct vg46x_state_t {
    u_char		ctl, ema;
} vg46x_state_t;

typedef struct ti113x_state_t {
    u_int		sysctl;
    u_char		cardctl, devctl;
} ti113x_state_t;

typedef struct rl5c466_state_t {
    u_short		misc, ctl, io, mem;
} rl5c466_state_t;

typedef struct topic_state_t {
    u_char		slot, ccr, cdr;
    u_int		rcr;
} topic_state_t;

typedef struct socket_info_t {
    u_short		type, flags;
    socket_cap_t	cap;
    u_short		ioaddr;
    u_short		psock;
    u_char		cs_irq;
    void		(*handler)(void *info, u_int events);
    void		*info;
#ifdef CONFIG_PROC_FS
    struct proc_dir_entry *proc;
#endif
#ifdef CONFIG_PCI
    u_char		bus, devfn;
    u_short		bcr;
    u_char		pci_lat, cb_lat, sub_bus;
    u_int		cb_phys;
    char		*cb_virt;
#endif
    union {
	pd672x_state_t		pd672x;
	vg46x_state_t		vg46x;
#ifdef CONFIG_PCI
	ti113x_state_t		ti113x;
	rl5c466_state_t		rl5c466;
	topic_state_t		topic;
#endif
    } state;
} socket_info_t;

/* Where we keep track of our sockets... */
static int sockets = 0;
static socket_info_t socket[8] = {
    { 0, }, /* ... */
};

/* Default interrupt mask */
#define I365_MASK	0xdeb8	/* irq 15,14,12,11,10,9,7,5,4,3 */

static void pcic_interrupt_wrapper(u_long);
static void pcic_interrupt IRQ(int irq, void *dev, struct pt_regs *regs);
static int pcic_service(u_int sock, u_int cmd, void *arg);
#ifdef CONFIG_PROC_FS
static void pcic_proc_remove(u_short sock);
#endif

#ifdef CONFIG_ISA
static int grab_irq;
#ifdef __SMP__
static spinlock_t isa_lock = SPIN_LOCK_UNLOCKED;
#endif
#endif
static struct timer_list poll_timer;

/*====================================================================*/

#ifdef CONFIG_PCI

#ifndef PCI_VENDOR_ID_OMEGA
#define PCI_VENDOR_ID_OMEGA		0x119b
#endif
#ifndef PCI_DEVICE_ID_OMEGA_PCMCIA
#define PCI_DEVICE_ID_OMEGA_PCMCIA	0x1221
#endif

#endif

/* These definitions must match the pcic_info table! */
typedef enum pcic_id {
#ifdef CONFIG_ISA
    IS_I82365A, IS_I82365B, IS_I82365DF,
    IS_IBM, IS_RF5Cx96, IS_VLSI, IS_VG468, IS_VG469,
    IS_PD6710, IS_PD672X, IS_VT83C469,
#endif
#ifdef CONFIG_PCI
    IS_PD6729, IS_PD6730, IS_OZ6729, IS_OZ6730,
    IS_I82092AA, IS_OM82C092G,
    IS_PD6832, IS_OZ6832, IS_RL5C466, IS_SMC34C90,
    IS_TI1130, IS_TI1131, IS_TI1250A, IS_TI1220,
#ifdef CONFIG_TOPIC
    IS_TOPIC95
#endif
#endif
} pcic_id;

/* Flags for classifying groups of controllers */
#define IS_VADEM	0x0001
#define IS_CIRRUS	0x0002
#define IS_TI		0x0004
#define IS_O2MICRO	0x0008
#define IS_VIA		0x0010
#define IS_VG_PWR	0x1000
#define IS_DF_PWR	0x2000
#define IS_PCI		0x4000
#define IS_CARDBUS	0x8000

typedef struct pcic_info_t {
    char		*name;
    u_short		flags;
#ifdef CONFIG_PCI
    u_short		vendor, device;
#endif
} pcic_info_t;

static pcic_info_t pcic_info[] = {
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
    { "Cirrus PD6729", IS_CIRRUS|IS_PCI,
      PCI_VENDOR_ID_CIRRUS, PCI_DEVICE_ID_CIRRUS_6729 },
    { "Cirrus PD6730", IS_CIRRUS|IS_PCI,
      PCI_VENDOR_ID_CIRRUS, 0xffff },
    { "O2Micro OZ6729", IS_O2MICRO|IS_PCI|IS_VG_PWR,
      PCI_VENDOR_ID_O2, PCI_DEVICE_ID_O2_6729 },
    { "O2Micro OZ6730", IS_O2MICRO|IS_PCI|IS_VG_PWR,
      PCI_VENDOR_ID_O2, PCI_DEVICE_ID_O2_6730 },
    { "Intel 82092AA", IS_PCI,
      PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_OMEGA_PCMCIA },
    { "Omega Micro 82C092G", IS_PCI,
      PCI_VENDOR_ID_OMEGA, PCI_DEVICE_ID_OMEGA_PCMCIA },
    { "Cirrus PD6832", IS_CIRRUS|IS_CARDBUS,
      PCI_VENDOR_ID_CIRRUS, PCI_DEVICE_ID_CIRRUS_6832 },
    { "O2Micro OZ6832", IS_O2MICRO|IS_CARDBUS|IS_VG_PWR,
      PCI_VENDOR_ID_O2, PCI_DEVICE_ID_O2_6832 },
    { "Ricoh RL5C466", IS_CARDBUS,
      PCI_VENDOR_ID_RICOH, PCI_DEVICE_ID_RICOH_RL5C466 },
    { "SMC 34C90", IS_CARDBUS,
      PCI_VENDOR_ID_SMC, PCI_DEVICE_ID_SMC_34C90 },
    { "TI 1130", IS_TI|IS_CARDBUS,
      PCI_VENDOR_ID_TI, PCI_DEVICE_ID_TI_1130 },
    { "TI 1131", IS_TI|IS_CARDBUS,
      PCI_VENDOR_ID_TI, PCI_DEVICE_ID_TI_1131 },
    { "TI 1250A", IS_TI|IS_CARDBUS,
      PCI_VENDOR_ID_TI, PCI_DEVICE_ID_TI_1250A },
    { "TI 1220", IS_TI|IS_CARDBUS,
      PCI_VENDOR_ID_TI, PCI_DEVICE_ID_TI_1220 },
#ifdef CONFIG_TOPIC
    { "Toshiba ToPIC95", IS_CARDBUS,
      PCI_VENDOR_ID_TOSHIBA, PCI_DEVICE_ID_TOSHIBA_TOPIC95 }
#endif
#endif
};

#define PCIC_COUNT	(sizeof(pcic_info)/sizeof(pcic_info_t))

/*====================================================================*/

/* Some PCI shortcuts */

#ifdef CONFIG_PCI

#ifndef PCI_FUNC
#define PCI_FUNC(devfn)		((devfn)&7)
#endif

#define pci_readb		pcibios_read_config_byte
#define pci_writeb		pcibios_write_config_byte
#define pci_readw		pcibios_read_config_word
#define pci_writew		pcibios_write_config_word
#define pci_readl		pcibios_read_config_dword
#define pci_writel		pcibios_write_config_dword

#define cb_readb(s, r)		readb(socket[s].cb_virt + (r))
#define cb_readl(s, r)		readl(socket[s].cb_virt + (r))
#define cb_writeb(s, r, v)	writeb(v, socket[s].cb_virt + (r))
#define cb_writel(s, r, v)	writel(v, socket[s].cb_virt + (r))

static void cb_get_power(u_short sock, socket_state_t *state);
static void cb_set_power(u_short sock, socket_state_t *state);
#endif

/*====================================================================*/

static u_char i365_get(u_short sock, u_short reg)
{
#ifdef CONFIG_PCI
    if (socket[sock].cb_virt)
	return cb_readb(sock, 0x0800 + reg);
    else
#endif
    {
	u_short port = socket[sock].ioaddr;
	u_char val;
	reg = I365_REG(socket[sock].psock, reg);
	outb_p(reg, port); val = inb_p(port+1);
	return val;
    }
}

static void i365_set(u_short sock, u_short reg, u_char data)
{
#ifdef CONFIG_PCI
    if (socket[sock].cb_virt)
	cb_writeb(sock, 0x0800 + reg, data);
    else
#endif
    {
	u_short port = socket[sock].ioaddr;
	u_char val = I365_REG(socket[sock].psock, reg);
	outb_p(val, port); outb_p(data, port+1);
    }
}

static void i365_bset(u_short sock, u_short reg, u_char mask)
{
    u_char d = i365_get(sock, reg);
    d |= mask;
    i365_set(sock, reg, d);
}

static void i365_bclr(u_short sock, u_short reg, u_char mask)
{
    u_char d = i365_get(sock, reg);
    d &= ~mask;
    i365_set(sock, reg, d);
}

static void i365_bflip(u_short sock, u_short reg, u_char mask, int b)
{
    u_char d = i365_get(sock, reg);
    if (b)
	d |= mask;
    else
	d &= ~mask;
    i365_set(sock, reg, d);
}

static u_short i365_get_pair(u_short sock, u_short reg)
{
    u_short a, b;
    a = i365_get(sock, reg);
    b = i365_get(sock, reg+1);
    return (a + (b<<8));
}

static void i365_set_pair(u_short sock, u_short reg, u_short data)
{
    i365_set(sock, reg, data & 0xff);
    i365_set(sock, reg+1, data >> 8);
}

/*======================================================================

    Code to save and restore global state information for Cirrus
    PD67xx controllers, and to set and report global configuration
    options.

    The VIA controllers also use these routines, as they are mostly
    Cirrus lookalikes, without the timing registers.
    
======================================================================*/

#define flip(v,b,f) (v = ((f)<0) ? v : ((f) ? ((v)|(b)) : ((v)&(~b))))

static void pd67xx_get_state(u_short s)
{
    int i;
    pd672x_state_t *p = &socket[s].state.pd672x;
    p->misc1 = i365_get(s, PD67_MISC_CTL_1);
    p->misc1 &= (PD67_MC1_MEDIA_ENA | PD67_MC1_INPACK_ENA);
    p->misc2 = i365_get(s, PD67_MISC_CTL_2);
    for (i = 0; i < 6; i++)
	p->timer[i] = i365_get(s, PD67_TIME_SETUP(0)+i);
}

static void pd67xx_set_state(u_short s)
{
    int i;
    u_char misc1;
    pd672x_state_t *p = &socket[s].state.pd672x;
    misc1 = i365_get(s, PD67_MISC_CTL_1);
    misc1 &= ~(PD67_MC1_MEDIA_ENA | PD67_MC1_INPACK_ENA);
    i365_set(s, PD67_MISC_CTL_1, misc1 | p->misc1);
    i365_set(s, PD67_MISC_CTL_2, p->misc2);
    for (i = 0; i < 6; i++)
	i365_set(s, PD67_TIME_SETUP(0)+i, p->timer[i]);
}

static u_int pd67xx_set_opts(u_short s, char *buf)
{
    socket_info_t *t = &socket[s];
    pd672x_state_t *p = &socket[s].state.pd672x;
    u_int mask = 0xffff;

    if (has_ring == -1) has_ring = 1;
    flip(p->misc2, PD67_MC2_IRQ15_RI, has_ring);
    flip(p->misc2, PD67_MC2_DYNAMIC_MODE, dynamic_mode);
    if (p->misc2 & PD67_MC2_IRQ15_RI)
	strcat(buf, " [ring]");
    if (p->misc2 & PD67_MC2_DYNAMIC_MODE)
	strcat(buf, " [dyn mode]");
    if (p->misc1 & PD67_MC1_INPACK_ENA)
	strcat(buf, " [inpack]");
    if (!(t->flags & (IS_PCI | IS_CARDBUS))) {
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
	    mask &= (socket[s].type == IS_PD6730) ? ~0x0400 : ~0x8000;
    }
    if (t->flags & IS_CARDBUS) {
	t->bcr &= ~PD6832_BCR_MGMT_IRQ_ENA;
	t->bcr |= (pci_csc) ? 0 : PD6832_BCR_MGMT_IRQ_ENA;
#endif
    }
    if (!(t->flags & IS_VIA)) {
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

static void vg46x_get_state(u_short s)
{
    vg46x_state_t *p = &socket[s].state.vg46x;
    p->ctl = i365_get(s, VG468_CTL);
    if (socket[s].type == IS_VG469)
	p->ema = i365_get(s, VG469_EXT_MODE);
}

static void vg46x_set_state(u_short s)
{
    vg46x_state_t *p = &socket[s].state.vg46x;
    i365_set(s, VG468_CTL, p->ctl);
    if (socket[s].type == IS_VG469)
	i365_set(s, VG469_EXT_MODE, p->ema);
}

static u_int vg46x_set_opts(u_short s, char *buf)
{
    vg46x_state_t *p = &socket[s].state.vg46x;
    
    flip(p->ctl, VG468_CTL_ASYNC, async_clock);
    flip(p->ema, VG469_MODE_CABLE, cable_mode);
    if (p->ctl & VG468_CTL_ASYNC)
	strcat(buf, " [async]");
    if (p->ctl & VG468_CTL_INPACK)
	strcat(buf, " [inpack]");
    if (socket[s].type == IS_VG469) {
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

static void ti113x_get_state(u_short s)
{
    socket_info_t *t = &socket[s];
    ti113x_state_t *p = &socket[s].state.ti113x;
    pci_readl(t->bus, t->devfn, TI113X_SYSTEM_CONTROL, &p->sysctl);
    pci_readb(t->bus, t->devfn, TI113X_CARD_CONTROL, &p->cardctl);
    pci_readb(t->bus, t->devfn, TI113X_DEVICE_CONTROL, &p->devctl);
}

static void ti113x_set_state(u_short s)
{
    socket_info_t *t = &socket[s];
    ti113x_state_t *p = &socket[s].state.ti113x;
    pci_writel(t->bus, t->devfn, TI113X_SYSTEM_CONTROL, p->sysctl);
    pci_writeb(t->bus, t->devfn, TI113X_CARD_CONTROL, p->cardctl);
    pci_writeb(t->bus, t->devfn, TI113X_DEVICE_CONTROL, p->devctl);
}

static u_int ti113x_set_opts(u_short s, char *buf)
{
    socket_info_t *t = &socket[s];
    ti113x_state_t *p = &t->state.ti113x;
    u_int mask = 0xffff;
    int old = (t->type == IS_TI1130) || (t->type == IS_TI1131);
    
    flip(p->sysctl, TI113X_SCR_CLKRUN_ENA, has_clkrun);
    flip(p->sysctl, TI113X_SCR_CLKRUN_SEL, clkrun_sel);
    flip(p->cardctl, TI113X_CCR_RIENB, has_ring);
    p->cardctl &= ~TI113X_CCR_ZVENABLE;
    if (old) {
	p->cardctl &= ~(TI113X_CCR_PCI_IRQ_ENA |
			TI113X_CCR_PCI_IREQ | TI113X_CCR_PCI_CSC);
	if (pci_csc)
	    p->cardctl |= TI113X_CCR_PCI_IRQ_ENA | TI113X_CCR_PCI_CSC;
	if (pci_int)
	    p->cardctl |= TI113X_CCR_PCI_IRQ_ENA | TI113X_CCR_PCI_IREQ;
    }
    switch (irq_mode) {
    case 1:
	p->devctl &= ~TI113X_DCR_IMODE_MASK;
	p->devctl |= TI113X_DCR_IMODE_ISA;
	break;
    case 2:
	p->devctl &= ~TI113X_DCR_IMODE_MASK;
	p->devctl |= TI113X_DCR_IMODE_SERIAL;
	break;
    default:
	if ((p->devctl & TI113X_DCR_IMODE_MASK) == 0)
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
    if (p->sysctl & TI113X_SCR_PWRSAVINGS)
	strcat(buf, " [pwr save]");
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
    RL5C466 controller, and to set and report global configuration
    options.
    
======================================================================*/

#ifdef CONFIG_PCI

static void rl5c466_get_state(u_short s)
{
    socket_info_t *t = &socket[s];
    rl5c466_state_t *p = &socket[s].state.rl5c466;
    pci_readw(t->bus, t->devfn, RL5C466_MISC0, &p->misc);
    pci_readw(t->bus, t->devfn, RL5C466_16BIT_CTL, &p->ctl);
    pci_readw(t->bus, t->devfn, RL5C466_16BIT_IO_0, &p->io);
    pci_readw(t->bus, t->devfn, RL5C466_16BIT_MEM_0, &p->mem);
}

static void rl5c466_set_state(u_short s)
{
    socket_info_t *t = &socket[s];
    rl5c466_state_t *p = &socket[s].state.rl5c466;
    if (t->psock & 2) p->ctl |= RL5C466_16CTL_INDEX_SEL;
    pci_writew(t->bus, t->devfn, RL5C466_MISC0, p->misc);
    pci_writew(t->bus, t->devfn, RL5C466_16BIT_CTL, p->ctl);
    pci_writew(t->bus, t->devfn, RL5C466_16BIT_IO_0, p->io);
    pci_writew(t->bus, t->devfn, RL5C466_16BIT_MEM_0, p->mem);
}

static u_int rl5c466_set_opts(u_short s, char *buf)
{
    rl5c466_state_t *p = &socket[s].state.rl5c466;
    u_int mask = 0xffff;

    p->ctl = RL5C466_16CTL_LEVEL_1 | RL5C466_16CTL_LEVEL_2 |
	RL5C466_16CTL_IO_TIMING | RL5C466_16CTL_MEM_TIMING;
    
    if (setup_time >= 0) {
	p->io = (p->io & ~RL5C466_SETUP_MASK) +
	    ((setup_time+1) << RL5C466_SETUP_SHIFT);
	p->mem = (p->mem & ~RL5C466_SETUP_MASK) +
	    (setup_time << RL5C466_SETUP_SHIFT);
    }
    if (cmd_time >= 0) {
	p->io = (p->io & ~RL5C466_CMD_MASK) +
	    (cmd_time << RL5C466_CMD_SHIFT);
	p->mem = (p->mem & ~RL5C466_CMD_MASK) +
	    (cmd_time << RL5C466_CMD_SHIFT);
    }
    if (hold_time >= 0) {
	p->io = (p->io & ~RL5C466_HOLD_MASK) +
	    (hold_time << RL5C466_HOLD_SHIFT);
	p->mem = (p->mem & ~RL5C466_HOLD_MASK) +
	    (hold_time << RL5C466_HOLD_SHIFT);
    }
    sprintf(buf, " [io %d/%d/%d] [mem %d/%d/%d]",
	    (p->io & RL5C466_SETUP_MASK) >> RL5C466_SETUP_SHIFT,
	    (p->io & RL5C466_CMD_MASK) >> RL5C466_CMD_SHIFT,
	    (p->io & RL5C466_HOLD_MASK) >> RL5C466_HOLD_SHIFT,
	    (p->mem & RL5C466_SETUP_MASK) >> RL5C466_SETUP_SHIFT,
	    (p->mem & RL5C466_CMD_MASK) >> RL5C466_CMD_SHIFT,
	    (p->mem & RL5C466_HOLD_MASK) >> RL5C466_HOLD_SHIFT);
    return mask;
}

#endif

/*======================================================================

    Code to save and restore global state information for the Toshiba
    ToPIC 95 controller, and to set and report global configuration
    options.
    
======================================================================*/

#ifdef CONFIG_PCI
#ifdef CONFIG_TOPIC

static void topic_get_state(u_short s)
{
    socket_info_t *t = &socket[s];
    topic_state_t *p = &socket[s].state.topic;
    pci_readb(t->bus, t->devfn, TOPIC_SLOT_CONTROL, &p->slot);
    pci_readb(t->bus, t->devfn, TOPIC_CARD_CONTROL, &p->ccr);
    pci_readb(t->bus, t->devfn, TOPIC_CARD_DETECT, &p->cdr);
    pci_readl(t->bus, t->devfn, TOPIC_REGISTER_CONTROL, &p->rcr);
}

static void topic_set_state(u_short s)
{
    socket_info_t *t = &socket[s];
    topic_state_t *p = &socket[s].state.topic;
    pci_writeb(t->bus, t->devfn, TOPIC_SLOT_CONTROL, p->slot);
    pci_writeb(t->bus, t->devfn, TOPIC_CARD_CONTROL, p->ccr);
    pci_writeb(t->bus, t->devfn, TOPIC_CARD_DETECT, p->cdr);
    pci_writel(t->bus, t->devfn, TOPIC_REGISTER_CONTROL, p->rcr);
}

static u_int topic_set_opts(u_short s, char *buf)
{
    topic_state_t *p = &socket[s].state.topic;
    u_int mask = 0xffff;

#ifdef CONFIG_ISA
    pci_csc = 1; /* Card status changes are always PCI */
#endif
    p->slot |= TOPIC_SLOT_SLOTON | TOPIC_SLOT_SLOTEN | TOPIC_SLOT_ID_LOCK;
    switch (p->ccr & TOPIC_CCR_CLOCK) {
    case 0:
	strcat(buf, " [no clock]"); break;
    case TOPIC_CCR_CCLK:
	strcat(buf, " [cclk]"); break;
    case TOPIC_CCR_PCICLK_2:
	strcat(buf, " [pci clk/2]"); break;
    case TOPIC_CCR_PCICLK:
	strcat(buf, " [pci clk]"); break;
    }
    p->cdr |= TOPIC_CDR_MODE_PC32;
    p->cdr &= ~(TOPIC_CDR_SW_DETECT);
    sprintf(buf, " [slot 0x%02x] [ccr 0x%02x] [cdr 0x%02x] [rcr 0x%08x]",
	    p->slot, p->ccr, p->cdr, p->rcr);
    return mask;
}

#endif
#endif

/*======================================================================

    Routines to handle common CardBus options
    
======================================================================*/

#ifdef CONFIG_PCI

static void cb_get_state(u_short s)
{
    socket_info_t *t = &socket[s];
    pci_readb(t->bus, t->devfn, PCI_LATENCY_TIMER, &t->pci_lat);
    pci_readb(t->bus, t->devfn, CB_LATENCY_TIMER, &t->cb_lat);
    pci_readb(t->bus, t->devfn, CB_CARDBUS_BUS, &t->cap.cardbus);
    pci_readb(t->bus, t->devfn, CB_SUBORD_BUS, &t->sub_bus);
    pci_readw(t->bus, t->devfn, CB_BRIDGE_CONTROL, &t->bcr);
#if (LINUX_VERSION_CODE < VERSION(2,1,93))
    pci_readb(t->bus, t->devfn, PCI_INTERRUPT_LINE, &t->cap.pci_irq);
#else
    {
	struct pci_dev *pdev = pci_find_slot(t->bus, t->devfn);
	t->cap.pci_irq = (pdev) ? pdev->irq : 0;
    }
#endif
    if (t->cap.pci_irq >= NR_IRQS) t->cap.pci_irq = 0;
}

static void cb_set_state(u_short s)
{
    socket_info_t *t = &socket[s];
    pci_writeb(t->bus, t->devfn, PCI_LATENCY_TIMER, t->pci_lat);
    pci_writeb(t->bus, t->devfn, CB_LATENCY_TIMER, t->cb_lat);
    pci_writeb(t->bus, t->devfn, CB_CARDBUS_BUS, t->cap.cardbus);
    pci_writeb(t->bus, t->devfn, CB_SUBORD_BUS, t->sub_bus);
    pci_writew(t->bus, t->devfn, CB_BRIDGE_CONTROL, t->bcr);
}

static u_int cb_set_opts(u_short s, char *buf)
{
    socket_info_t *t = &socket[s];
    if (pci_latency >= 0) t->pci_lat = pci_latency;
    if (t->pci_lat == 0) t->pci_lat = 0xa8;
    if (cb_latency >= 0) t->cb_lat = cb_latency;
    if (t->cb_lat == 0) t->cb_lat = 0xb0;
    t->bcr |= CB_BCR_ISA_IRQ;
    if (t->cap.pci_irq == 0)
	strcat(buf, " [no pci irq]");
    else {
	sprintf(buf, " [pci irq %d]", t->cap.pci_irq);
	if (pci_int) t->bcr &= ~CB_BCR_ISA_IRQ;
    }
    buf += strlen(buf);
    if (t->cap.cardbus == 0) {
	t->cap.cardbus = cb_bus_base;
	t->sub_bus = cb_bus_base+2;
	cb_bus_base += 3;
    }
    sprintf(buf, " [lat %d/%d] [bus %d/%d]",
	    t->pci_lat, t->cb_lat, t->cap.cardbus, t->sub_bus);
    /* mask out PCI interrupts */
    return ~(1 << t->cap.pci_irq);
}

#endif

/*======================================================================

    Generic routines to get and set controller options
    
======================================================================*/

static void get_host_state(u_short s)
{
    if (socket[s].flags & IS_CIRRUS)
	pd67xx_get_state(s);
#ifdef CONFIG_ISA
    else if (socket[s].flags & IS_VADEM)
	vg46x_get_state(s);
#endif
#ifdef CONFIG_PCI
    else if (socket[s].flags & IS_TI)
	ti113x_get_state(s);
    else if (socket[s].type == IS_RL5C466)
	rl5c466_get_state(s);
#ifdef CONFIG_TOPIC
    else if (socket[s].type == IS_TOPIC95)
	topic_get_state(s);
#endif
    if (socket[s].flags & IS_CARDBUS)
	cb_get_state(s);
#endif
}

static void set_host_state(u_short s)
{
    if (socket[s].flags & IS_CIRRUS)
	pd67xx_set_state(s);
#ifdef CONFIG_ISA
    else if (socket[s].flags & IS_VADEM)
	vg46x_set_state(s);
#endif
#ifdef CONFIG_PCI
    else if (socket[s].flags & IS_TI)
	ti113x_set_state(s);
    else if (socket[s].type == IS_RL5C466)
	rl5c466_set_state(s);
#ifdef CONFIG_TOPIC
    else if (socket[s].type == IS_TOPIC95)
	topic_set_state(s);
#endif
    if (socket[s].flags & IS_CARDBUS)
	cb_set_state(s);
#endif
}

static u_int set_host_opts(u_short s, u_short ns)
{
    u_short i;
    u_int m = 0xffff;
    char buf[128];

    for (i = s; i < s+ns; i++) {
	buf[0] = '\0';
	get_host_state(i);
	if (socket[i].flags & IS_CIRRUS)
	    m = pd67xx_set_opts(i, buf);
#ifdef CONFIG_ISA
	else if (socket[i].flags & IS_VADEM)
	    m = vg46x_set_opts(i, buf);
#endif
#ifdef CONFIG_PCI
	else if (socket[i].flags & IS_TI)
	    m = ti113x_set_opts(i, buf);
	else if (socket[i].type == IS_RL5C466)
	    m = rl5c466_set_opts(i, buf);
#ifdef CONFIG_TOPIC
	else if (socket[s].type == IS_TOPIC95)
	    m = topic_set_opts(s, buf);
#endif
	if (socket[s].flags & IS_CARDBUS)
	    m &= cb_set_opts(i, buf+strlen(buf));
	if (!(socket[s].flags & IS_CIRRUS))
	    i365_bclr(s, I365_GBLCTL, I365_GBL_WRBACK);
#endif
	set_host_state(i);
	printk(KERN_INFO "    host opts [%d]:%s\n", i,
	       (*buf) ? buf : " none");
    }
    return m;
}

/*======================================================================

    Interrupt testing code, for ISA bus interrupts only.
    
======================================================================*/

static volatile u_int irq_hits;

static void irq_count IRQ(int irq, void *dev, struct pt_regs *regs)
{
    irq_hits++;
}

#ifdef CONFIG_ISA

static u_int test_irq(u_short sock, int irq)
{
    DEBUG(2, "  testing ISA irq %d\n", irq);
    
    if (REQUEST_IRQ(irq, irq_count, 0, "irq scan", NULL) != 0)
	return 1;
    irq_hits = 0;
    mdelay(10);
    if (irq_hits) {
	FREE_IRQ(irq, NULL);
	return 1;
    }

    /* Generate one interrupt */
    i365_set(sock, I365_CSCINT, I365_CSC_DETECT | (irq << 4));
    i365_bclr(sock, I365_INTCTL, I365_INTR_ENA);
    i365_bset(sock, I365_GENCTL, I365_CTL_SW_IRQ);

    udelay(1000);
    FREE_IRQ(irq, NULL);

    /* Turn off interrupts */
    i365_set(sock, I365_CSCINT, 0);
    
    return (irq_hits != 1);
}

static u_int irq_scan(u_short sock, u_int mask0)
{
    u_int mask1;
    int i;

#ifdef __alpha__
#define PIC 0x4d0
    /* Don't probe level-triggered interrupts -- reserved for PCI */
    int level_mask = inb_p(PIC) | (inb_p(PIC+1) << 8);
    if (level_mask)
	mask0 &= ~level_mask;
#endif
    
    mask1 = 0;
    if (do_scan) {
	i365_set(sock, I365_CSCINT, 0);
	for (i = 0; i < 16; i++)
	    if ((mask0 & (1 << i)) && (test_irq(sock, i) == 0))
		mask1 |= (1 << i);
	for (i = 0; i < 16; i++)
	    if ((mask1 & (1 << i)) && (test_irq(sock, i) != 0))
		mask1 ^= (1 << i);
    }
    
    printk(KERN_INFO "    ISA irqs (");
    if (mask1) {
	printk("scanned");
    } else {
	/* Fallback: just find interrupts that aren't in use */
	for (i = 0; i < 16; i++)
	    if ((mask0 & (1 << i)) &&
		(REQUEST_IRQ(i, irq_count, 0, "x", NULL) == 0)) {
		mask1 |= (1 << i);
		FREE_IRQ(i, NULL);
	    }
	printk("default");
    }
    printk(") = ");
    
    for (i = 0; i < 16; i++)
	if (mask1 & (1<<i))
	    printk("%s%d", ((mask1 & ((1<<i)-1)) ? "," : ""), i);
    
    return mask1;
}

#endif

/*====================================================================*/

/* Time conversion functions */

static int to_cycles(int ns)
{
    return ns/cycle_time;
} /* speed_convert */

static int to_ns(int cycles)
{
    return cycle_time*cycles;
}

/*====================================================================*/

#ifdef CONFIG_ISA

static int identify(u_short port, u_short sock)
{
    u_char val;
    int type = -1;

    /* Use the next free entry in the socket table */
    socket[sockets].ioaddr = port;
    socket[sockets].psock = sock;
    
    /* Wake up a sleepy Cirrus controller */
    if (wakeup) {
	i365_bclr(sockets, PD67_MISC_CTL_2, PD67_MC2_SUSPEND);
	/* Pause at least 50 ms */
	mdelay(50);
    }
    
    if ((val = i365_get(sockets, I365_IDENT)) & 0x70)
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
    outb_p(0x0e, port);
    outb_p(0x37, port);
    i365_bset(sockets, VG468_MISC, VG468_MISC_VADEMREV);
    val = i365_get(sockets, I365_IDENT);
    if (val & I365_IDENT_VADEM) {
	i365_bclr(sockets, VG468_MISC, VG468_MISC_VADEMREV);
	type = ((val & 7) >= 4) ? IS_VG469 : IS_VG468;
    }

    /* Check for Ricoh chips */
    val = i365_get(sockets, RF5C_CHIP_ID);
    if ((val == RF5C_CHIP_RF5C296) || (val == RF5C_CHIP_RF5C396))
	type = IS_RF5Cx96;
    
    /* Check for Cirrus CL-PD67xx chips */
    i365_set(sockets, PD67_CHIP_INFO, 0);
    val = i365_get(sockets, PD67_CHIP_INFO);
    if ((val & PD67_INFO_CHIP_ID) == PD67_INFO_CHIP_ID) {
	val = i365_get(sockets, PD67_CHIP_INFO);
	if ((val & PD67_INFO_CHIP_ID) == 0)
	    type = (val & PD67_INFO_SLOTS) ? IS_PD672X : IS_PD6710;
	i365_set(sockets, PD67_EXT_INDEX, 0xe5);
	if (i365_get(sockets, PD67_EXT_INDEX) != 0xe5)
	    type = IS_VT83C469;
    }
    return type;
} /* identify */

#endif

/*====================================================================*/

static void add_socket(u_short port, int psock, int type)
{
    socket[sockets].ioaddr = port;
    socket[sockets].psock = psock;
    socket[sockets].type = type;
    socket[sockets].flags = pcic_info[type].flags;
    sockets++;
}

static void add_pcic(int ns, int type)
{
#ifdef CONFIG_ISA
    u_int scan;
#endif
    u_int mask = 0, i, base;
    int pci_irq = 0, isa_irq = 0;
    socket_info_t *t = &socket[sockets-ns];

    base = sockets-ns;
    request_region(t->ioaddr, 2, "i82365");
    
    if (base == 0) printk("\n");
    printk(KERN_INFO "  %s", pcic_info[type].name);
#ifdef CONFIG_PCI
    if (t->flags & IS_CARDBUS)
	printk(" CardBus at mem 0x%08x", t->cb_phys);
    else
#endif
	printk("%s at port %#x ofs 0x%02x",
	       ((t->flags & IS_PCI) ? " PCI" : ""),
	       t->ioaddr, t->psock*0x40);
    printk(", %d socket%s\n", ns, ((ns > 1) ? "s" : ""));

#ifdef CONFIG_ISA
    /* Set host options, build basic interrupt mask */
    if (irq_list[0] == -1)
	mask = irq_mask;
    else
	for (i = mask = 0; i < 16; i++)
	    mask |= (1<<irq_list[i]);
#endif
    mask &= I365_MASK & set_host_opts(base, ns);
#ifdef CONFIG_PCI
    if (pci_int && t->cap.pci_irq)
	printk(KERN_INFO "    PCI card interrupts,");
#endif
#ifdef CONFIG_ISA
    if (!pci_int || !t->cap.pci_irq) {
	/* Scan interrupts if possible */
	mask = irq_scan(base, mask);
    }
#endif
    
#ifdef CONFIG_PCI
    /* Can we use a PCI interrupt for card status changes? */
    if (pci_csc && t->cap.pci_irq) {
	for (i = 0; i < ns; i++) {
	    if (REQUEST_IRQ(t[i].cap.pci_irq, irq_count,
			    SA_SHIRQ, "i", NULL) != 0)
		break;
	    FREE_IRQ(t[i].cap.pci_irq, NULL);
	}
	pci_irq = (i == ns);
	printk(" PCI status changes\n");
    }
#endif
    
#ifdef CONFIG_ISA
    /* Poll if only two interrupts available */
    if (!pci_irq && !poll_interval) {
	scan = (mask & (mask-1));
	if ((scan & (scan-1)) == 0)
	    poll_interval = HZ;
    }
    /* Only try an ISA cs_irq if this is the first controller */
    if (!pci_irq && !grab_irq && !poll_interval) {
	/* Avoid irq 12 unless it is explicitly requested */
	u_int cs_mask = mask & ((cs_irq) ? (1<<cs_irq) : ~(1<<12));
	for (cs_irq = 15; cs_irq > 0; cs_irq--)
	    if ((cs_mask & (1 << cs_irq)) &&
		(REQUEST_IRQ(cs_irq, irq_count, 0, "i", NULL) == 0))
		break;
	if (cs_irq) {
	    FREE_IRQ(cs_irq, NULL);
	    grab_irq = 1;
	    isa_irq = cs_irq;
	    printk(" status change on irq %d\n", cs_irq);
	}
    }
#endif
    
    if (!pci_irq && !isa_irq) {
	if (poll_interval == 0)
	    poll_interval = HZ;
	printk(" polled status, interval = %d ms\n",
	       poll_interval * 1000 / HZ);
    }
    
    /* Turn on interrupts, but with all events masked */
    for (i = 0; i < ns; i++) {
	t[i].cap.map_size = 0x1000;
	if (pci_int && t[i].cap.pci_irq)
	    t[i].cap.irq_mask = (1 << t[i].cap.pci_irq);
	else
	    t[i].cap.irq_mask = mask;
	t[i].cs_irq = isa_irq;
	if (isa_irq || pci_irq) {
	    i365_set(i, I365_CSCINT, (isa_irq << 4));
	    i365_bclr(i, I365_INTCTL, I365_INTR_ENA);
	    i365_get(i, I365_CSC);
	}
    }

} /* add_pcic */

/*======================================================================

    See if a card is present, powered up, in IO mode, and already
    bound to a (non PC Card) Linux driver.  We leave these alone.

    We make an exception for cards that seem to be serial devices.
    
======================================================================*/

static int is_active(u_short port, u_short sock)
{
    u_char stat;
    u_short start, stop;
    
    /* Use the next free entry in the socket table */
    socket[sockets].ioaddr = port;
    socket[sockets].psock = sock;
    
    stat = i365_get(sockets, I365_STATUS);
    start = i365_get_pair(sockets, I365_IO(0)+I365_W_START);
    stop = i365_get_pair(sockets, I365_IO(0)+I365_W_STOP);
    if ((stat & I365_CS_DETECT) && (stat & I365_CS_POWERON) &&
	(i365_get(sockets, I365_INTCTL) & I365_PC_IOCARD) &&
	(i365_get(sockets, I365_ADDRWIN) & I365_ENA_IO(0)) &&
	(check_region(start, stop-start+1) != 0) &&
	((start & 0xfeef) != 0x02e8))
	return 1;
    else
	return 0;
}

/*====================================================================*/

#ifdef CONFIG_PCI

/* Default settings for PCI command configuration register */
#define CMD_DFLT (PCI_COMMAND_IO|PCI_COMMAND_MEMORY| \
		  PCI_COMMAND_MASTER|PCI_COMMAND_WAIT)

static void pci_to_pccard_probe(u_short vendor, u_short device, int type)
{
    u_short i, ns;
    u_char bus, devfn;
    u_int addr;

#if (LINUX_VERSION_CODE < VERSION(2,1,93))
    u_short index = 0;
    while (pcibios_find_device(vendor, device,
			       index++, &bus, &devfn) == 0) {
#else
    struct pci_dev *pdev = NULL;
    while ((pdev = pci_find_device(vendor, device, pdev)) != NULL) {
	bus = pdev->bus->number;
	devfn = pdev->devfn;
#endif
	/* Some controllers seem to report multiple functions */
	if (PCI_FUNC(devfn) != 0)
	    continue;
	pci_readl(bus, devfn, PCI_BASE_ADDRESS_0, &addr);
	addr &= ~0x1;
	pci_writew(bus, devfn, PCI_COMMAND, CMD_DFLT);
	for (i = ns = 0; i < 2; i++) {
	    if (is_active(addr, i)) continue;
	    add_socket(addr, i, type); ns++;
	}
	add_pcic(ns, type);
    }
}

static void cb_setup(int type, u_char bus, u_char devfn)
{
    socket_info_t *s = &socket[sockets];
    u_short bcr, ns;

    s->bus = bus; s->devfn = devfn;
    pci_writew(bus, devfn, PCI_COMMAND, CMD_DFLT);

    ns = devfn & 1;
    if (type == IS_RL5C466) {
	pci_readw(bus, devfn, CB_BRIDGE_CONTROL, &bcr);
	ns += (bcr & RL5C466_BCR_3E2_ENA) ? 4 : 0;
    } else if ((type == IS_PD6832) || (type == IS_OZ6832))
	pci_readw(bus, devfn, PD6832_SOCKET_NUMBER, &ns);

    /* Map CardBus registers if they are not already mapped */
    pci_writel(bus, devfn, CB_LEGACY_MODE_BASE, 0);
    pci_readl(bus, devfn, PCI_BASE_ADDRESS_0, &s->cb_phys);
    if (s->cb_phys == 0) {
	s->cb_phys = cb_mem_base;
	pci_writel(bus, devfn, PCI_BASE_ADDRESS_0, s->cb_phys);
	cb_mem_base += 0x1000;
    }
    s->cb_virt = ioremap(s->cb_phys, 0x1000);
    if (s->cap.pci_irq >= NR_IRQS) s->cap.pci_irq = 0;
    
    add_socket(0, ns & 3, type);
}

static void pci_probe(void)
{
    u_short i;
    u_char bus, devfn;
    
    /* Check for PCI-to-PC-Card and Cardbus controllers */
    for (i = 0; i < PCIC_COUNT; i++) {
	if (pcic_info[i].flags & IS_PCI)
	    pci_to_pccard_probe(pcic_info[i].vendor,
				pcic_info[i].device, i);
	if (!(pcic_info[i].flags & IS_CARDBUS))
	    continue;
#if (LINUX_VERSION_CODE < VERSION(2,1,93))
	{
	    u_short index = 0;
	    while (pcibios_find_device(pcic_info[i].vendor,
				       pcic_info[i].device,
				       index++, &bus, &devfn) == 0) {
#else
	{
	    struct pci_dev *pdev = NULL;
	    while ((pdev = pci_find_device(pcic_info[i].vendor,
					   pcic_info[i].device,
					   pdev)) != NULL) {
		bus = pdev->bus->number;
		devfn = pdev->devfn;
#endif
		if (PCI_FUNC(devfn) == 0) {
		    cb_setup(i, bus, devfn);
		    cb_setup(i, bus, devfn+1);
		    add_pcic(2, i);
		}
	    }
	}
    }
    
}
#endif /* CONFIG_PCI */

/*====================================================================*/

#ifdef CONFIG_ISA

static void isa_probe(void)
{
    int i, j, sock, k;
    int ns, id;
    u_short port;
    
    if (check_region(i365_base, 2) != 0) {
	if (sockets == 0)
	    printk("port conflict at %#x\n", i365_base);
	return;
    }

    id = identify(i365_base, 0);
    if ((id == IS_I82365DF) && (identify(i365_base, 1) != id)) {
	for (i = 0; i < 4; i++) {
	    if (i == ignore) continue;
	    port = i365_base + ((i & 1) << 2) + ((i & 2) << 1);
	    sock = (i & 1) << 1;
	    if ((identify(port, sock) == IS_I82365DF) &&
		!is_active(port, sock)) {
		add_socket(port, sock, IS_VLSI);
		add_pcic(1, IS_VLSI);
	    }
	}
    } else {
	for (i = 0; i < (extra_sockets ? 8 : 4); i += 2) {
	    port = i365_base + 2*(i>>2);
	    sock = (i & 3);
	    id = identify(port, sock);
	    if (id < 0) continue;

	    for (j = ns = 0; j < 2; j++) {
		/* Does the socket exist? */
		if ((ignore == i+j) || (identify(port, sock+j) < 0) ||
		    is_active(port, sock+j))
		    continue;
		/* Check for bad socket decode */
		for (k = 0; k <= sockets; k++)
		    i365_set(k, I365_MEM(0)+I365_W_OFF, k);
		for (k = 0; k <= sockets; k++)
		    if (i365_get(k, I365_MEM(0)+I365_W_OFF) != k)
			break;
		if (k <= sockets) break;
		add_socket(port, sock+j, id); ns++;
	    }
	    if (ns != 0) add_pcic(ns, id);
	}
    }
}

#endif

/*====================================================================*/

static int pcic_init(void)
{
    DEBUG(0, "%s\n", version);
    printk(KERN_INFO "Intel PCIC probe: ");
    sockets = 0;

#ifdef CONFIG_PCI
    if (pcibios_present())
	pci_probe();
#endif

#ifdef CONFIG_ISA
    isa_probe();
#endif
	
    if (sockets == 0) {
	printk("not found.\n");
	return -ENODEV;
    }
    
    /* Set up interrupt handler, and/or polling */
#ifdef CONFIG_ISA
    if (grab_irq != 0)
	REQUEST_IRQ(cs_irq, pcic_interrupt, 0, "i82365", NULL);
#endif
#ifdef CONFIG_PCI
    if (pci_csc) {
	u_int i, irq, mask = 0;
	for (i = 0; i < sockets; i++) {
	    irq = socket[i].cap.pci_irq;
	    if (irq && !(mask & (1<<irq)))
		REQUEST_IRQ(irq, pcic_interrupt, SA_SHIRQ,
			    "i82365", pcic_interrupt);
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
    	poll_timer.expires = RUN_AT(poll_interval);
	add_timer(&poll_timer);
    }
    
    return 0;
    
} /* pcic_init */
  
/*====================================================================*/

static void pcic_finish(void)
{
    int i;
#ifdef CONFIG_PROC_FS
    for (i = 0; i < sockets; i++) pcic_proc_remove(i);
#endif
    unregister_ss_entry(&pcic_service);
    if (poll_interval != 0)
	del_timer(&poll_timer);
#ifdef CONFIG_ISA
    if (grab_irq != 0)
	FREE_IRQ(cs_irq, NULL);
#endif
#ifdef CONFIG_PCI
    if (pci_csc) {
	u_int irq, mask = 0;
	for (i = 0; i < sockets; i++) {
	    irq = socket[i].cap.pci_irq;
	    if (irq && !(mask & (1<<irq)))
		FREE_IRQ(irq, pcic_interrupt);
	    mask |= (1<<irq);
	}
    }
#endif
    for (i = 0; i < sockets; i++) {
	i365_set(i, I365_CSCINT, 0);
	release_region(socket[i].ioaddr, 2);
#ifdef CONFIG_PCI
	if (socket[i].cb_virt)
	    iounmap(socket[i].cb_virt);
#endif
    }
} /* pcic_finish */

/*====================================================================*/

static void pcic_interrupt_wrapper(u_long data)
{
    pcic_interrupt IRQ(0, NULL, NULL);
    poll_timer.expires = RUN_AT(poll_interval);
    add_timer(&poll_timer);
}

static void pcic_interrupt IRQ(int irq, void *dev, struct pt_regs *regs)
{
    int i, j, csc;
    u_int events, active;
#ifdef CONFIG_ISA
    u_long flags = 0;
#endif
    
    DEBUG(2, "i82365: pcic_interrupt(%d)\n", irq);

    for (j = 0; j < 20; j++) {
	active = 0;
	for (i = 0; i < sockets; i++) {
	    if ((socket[i].cs_irq != irq) &&
		(socket[i].cap.pci_irq != irq))
		continue;
#ifdef CONFIG_ISA
	    if (!(socket[i].flags & IS_CARDBUS))
		spin_lock_irqsave(&isa_lock, flags);
#endif
	    csc = i365_get(i, I365_CSC);
	    if ((csc == 0) || (!socket[i].handler) ||
		(i365_get(i, I365_IDENT) & 0x70)) {
#ifdef CONFIG_ISA
		if (!(socket[i].flags & IS_CARDBUS))
		    spin_unlock_irqrestore(&isa_lock, flags);
#endif
		continue;
	    }
	    events = (csc & I365_CSC_DETECT) ? SS_DETECT : 0;
	    if (i365_get(i, I365_INTCTL) & I365_PC_IOCARD)
		events |= (csc & I365_CSC_STSCHG) ? SS_STSCHG : 0;
	    else {
		events |= (csc & I365_CSC_BVD1) ? SS_BATDEAD : 0;
		events |= (csc & I365_CSC_BVD2) ? SS_BATWARN : 0;
		events |= (csc & I365_CSC_READY) ? SS_READY : 0;
	    }
#ifdef CONFIG_ISA
	    if (!(socket[i].flags & IS_CARDBUS))
		spin_unlock_irqrestore(&isa_lock, flags);
#endif
	    DEBUG(2, "i82365: socket %d event 0x%02x\n", i, events);
	    if (events)
		socket[i].handler(socket[i].info, events);
	    active |= events;
	}
	if (!active) break;
    }
    if (j == 20)
	printk(KERN_NOTICE "i82365: infinite loop in interrupt handler\n");

    DEBUG(2, "i82365: interrupt done\n");

} /* pcic_interrupt */

/*====================================================================*/

static int pcic_register_callback(u_short sock, ss_callback_t *call)
{
    if (call == NULL) {
	socket[sock].handler = NULL;
	MOD_DEC_USE_COUNT;
    } else {
	MOD_INC_USE_COUNT;
	socket[sock].handler = call->handler;
	socket[sock].info = call->info;
    }
    return 0;
} /* pcic_register_callback */

/*====================================================================*/

static int pcic_inquire_socket(u_short sock, socket_cap_t *cap)
{
    *cap = socket[sock].cap;
    return 0;
} /* pcic_inquire_socket */

/*====================================================================*/

static int i365_get_status(u_short sock, u_int *value)
{
    u_int status;
    
    status = i365_get(sock, I365_STATUS);
    *value = ((status & I365_CS_DETECT) == I365_CS_DETECT)
	? SS_DETECT : 0;
    if (i365_get(sock, I365_INTCTL) & I365_PC_IOCARD)
	*value |= (status & I365_CS_STSCHG) ? 0 : SS_STSCHG;
    else {
	*value |= (status & I365_CS_BVD1) ? 0 : SS_BATDEAD;
	*value |= (status & I365_CS_BVD2) ? 0 : SS_BATWARN;
    }
    *value |= (status & I365_CS_WRPROT) ? SS_WRPROT : 0;
    *value |= (status & I365_CS_READY) ? SS_READY : 0;
    *value |= (status & I365_CS_POWERON) ? SS_POWERON : 0;

#ifdef CONFIG_PCI
    if (socket[sock].flags & IS_CARDBUS) {
	status = cb_readl(sock, CB_SOCKET_STATE);
	*value |= (status & CB_SS_3VCARD) ? SS_3VCARD : 0;
	*value |= (status & CB_SS_XVCARD) ? SS_XVCARD : 0;
    } else if (socket[sock].flags & IS_O2MICRO) {
	status = i365_get(sock, O2_MODE_B);
	*value |= (status & O2_MODE_B_VS1) ? 0 : SS_3VCARD;
	*value |= (status & O2_MODE_B_VS2) ? 0 : SS_XVCARD;
    } else
#endif
#ifdef CONFIG_ISA
    if (socket[sock].type == IS_VG469) {
	status = i365_get(sock, VG469_VSENSE);
	if (socket[sock].psock & 1) {
	    *value |= (status & VG469_VSENSE_B_VS1) ? 0 : SS_3VCARD;
	    *value |= (status & VG469_VSENSE_B_VS2) ? 0 : SS_XVCARD;
	} else {
	    *value |= (status & VG469_VSENSE_A_VS1) ? 0 : SS_3VCARD;
	    *value |= (status & VG469_VSENSE_A_VS2) ? 0 : SS_XVCARD;
	}
#endif
    }
    
    DEBUG(1, "i82365: GetStatus(%d) = %#4.4x\n", sock, *value);
    return 0;
} /* i365_get_status */

/*====================================================================*/

static int i365_get_socket(u_short sock, socket_state_t *state)
{
    u_char reg, vcc, vpp;
    
    reg = i365_get(sock, I365_POWER);
    state->flags = (reg & I365_PWR_AUTO) ? SS_PWR_AUTO : 0;
    state->flags |= (reg & I365_PWR_OUT) ? SS_OUTPUT_ENA : 0;
    vcc = reg & I365_VCC_MASK; vpp = reg & I365_VPP_MASK;
    state->Vcc = state->Vpp = 0;
#ifdef CONFIG_PCI
    if (socket[sock].flags & IS_CARDBUS) {
	cb_get_power(sock, state);
    } else
#endif
    if (socket[sock].flags & IS_CIRRUS) {
	if (i365_get(sock, PD67_MISC_CTL_1) & PD67_MC1_VCC_3V) {
	    if (reg & I365_VCC_5V) state->Vcc = 33;
	    if (vpp == I365_VPP_5V) state->Vpp = 33;
	} else {
	    if (reg & I365_VCC_5V) state->Vcc = 50;
	    if (vpp == I365_VPP_5V) state->Vpp = 50;
	}
	if (vpp == I365_VPP_12V) state->Vpp = 120;
    } else if (socket[sock].flags & IS_VG_PWR) {
	if (i365_get(sock, VG469_VSELECT) & VG469_VSEL_VCC) {
	    if (reg & I365_VCC_5V) state->Vcc = 33;
	    if (vpp == I365_VPP_5V) state->Vpp = 33;
	} else {
	    if (reg & I365_VCC_5V) state->Vcc = 50;
	    if (vpp == I365_VPP_5V) state->Vpp = 50;
	}
	if (vpp == I365_VPP_12V) state->Vpp = 120;
    } else if (socket[sock].flags & IS_DF_PWR) {
	if (vcc == I365_VCC_3V) state->Vcc = 33;
	if (vcc == I365_VCC_5V) state->Vcc = 50;
	if (vpp == I365_VPP_5V) state->Vpp = 50;
	if (vpp == I365_VPP_12V) state->Vpp = 120;
    } else {
	if (reg & I365_VCC_5V) {
	    state->Vcc = 50;
	    if (vpp == I365_VPP_5V) state->Vpp = 50;
	    if (vpp == I365_VPP_12V) state->Vpp = 120;
	}
    }

    /* IO card, RESET flags, IO interrupt */
    reg = i365_get(sock, I365_INTCTL);
    state->flags |= (reg & I365_PC_RESET) ? 0 : SS_RESET;
    if (reg & I365_PC_IOCARD) state->flags |= SS_IOCARD;
    state->io_irq = reg & I365_IRQ_MASK;
    
    /* DMA mode, speaker control */
    if (socket[sock].flags & IS_CIRRUS) {
	if (i365_get(sock, PD67_MISC_CTL_1) & PD67_MC1_SPKR_ENA)
	    state->flags |= SS_SPKR_ENA;
	if (i365_get(sock, PD67_MISC_CTL_2) & PD67_MC2_DMA_MODE)
	    state->flags |= SS_DMA_MODE;
    }
    
    /* Card status change mask */
    reg = i365_get(sock, I365_CSCINT);
    state->csc_mask = (reg & I365_CSC_DETECT) ? SS_DETECT : 0;
    if (state->flags & SS_IOCARD)
	state->csc_mask |= (reg & I365_CSC_STSCHG) ? SS_STSCHG : 0;
    else {
	state->csc_mask |= (reg & I365_CSC_BVD1) ? SS_BATDEAD : 0;
	state->csc_mask |= (reg & I365_CSC_BVD2) ? SS_BATWARN : 0;
	state->csc_mask |= (reg & I365_CSC_READY) ? SS_READY : 0;
    }
    
    DEBUG(1, "i82365: GetSocket(%d) = flags %#3.3x, Vcc %d, Vpp %d, "
	  "io_irq %d, csc_mask %#2.2x\n", sock, state->flags,
	  state->Vcc, state->Vpp, state->io_irq, state->csc_mask);
    return 0;
} /* i365_get_socket */

/*====================================================================*/

static int i365_set_socket(u_short sock, socket_state_t *state)
{
    u_char reg;
    
    DEBUG(1, "i82365: SetSocket(%d, flags %#3.3x, Vcc %d, Vpp %d, "
	  "io_irq %d, csc_mask %#2.2x)\n", sock, state->flags,
	  state->Vcc, state->Vpp, state->io_irq, state->csc_mask);

    /* Did something put the controller to sleep? */
    if (socket[sock].flags & IS_CIRRUS) {
	if (i365_get(sock, PD67_MISC_CTL_2) & PD67_MC2_SUSPEND) {
	    i365_bclr(sock, PD67_MISC_CTL_2, PD67_MC2_SUSPEND);
	    /* Pause at least 50 ms */
	    mdelay(50);
	}
    }

    /* Now set global controller options */
    set_host_state(sock);
    
    /* IO card, RESET flags, IO interrupt */
    reg = i365_get(sock, I365_INTCTL);
    reg &= ~(I365_PC_IOCARD | I365_PC_RESET | I365_IRQ_MASK);
    /* Note that the reset signal is inverted */
    reg |= (state->flags & SS_RESET) ? 0 : I365_PC_RESET;
    reg |= (state->flags & SS_IOCARD) ? I365_PC_IOCARD : 0;
    reg |= state->io_irq;
    i365_set(sock, I365_INTCTL, reg);

    reg = I365_PWR_NORESET;
    if (state->flags & SS_PWR_AUTO) reg |= I365_PWR_AUTO;
    if (state->flags & SS_OUTPUT_ENA) reg |= I365_PWR_OUT;

#ifdef CONFIG_PCI
    if (socket[sock].flags & IS_CARDBUS) {
	cb_set_power(sock, state);
	reg |= i365_get(sock, I365_POWER) & (I365_VCC_MASK|I365_VPP_MASK);
    } else
#endif
    if (socket[sock].flags & IS_CIRRUS) {
	if (state->Vpp != 0) {
	    if (state->Vpp == 120)
		reg |= I365_VPP_12V;
	    else if (state->Vpp == state->Vcc)
		reg |= I365_VPP_5V;
	    else return -EINVAL;
	}
	if (state->Vcc != 0) {
	    reg |= I365_VCC_5V;
	    if (state->Vcc == 33)
		i365_bset(sock, PD67_MISC_CTL_1, PD67_MC1_VCC_3V);
	    else if (state->Vcc == 50)
		i365_bclr(sock, PD67_MISC_CTL_1, PD67_MC1_VCC_3V);
	    else return -EINVAL;
	}
    } else if (socket[sock].flags & IS_VG_PWR) {
	if (state->Vpp != 0) {
	    if (state->Vpp == 120)
		reg |= I365_VPP_12V;
	    else if (state->Vpp == state->Vcc)
		reg |= I365_VPP_5V;
	    else return -EINVAL;
	}
	if (state->Vcc != 0) {
	    reg |= I365_VCC_5V;
	    if (state->Vcc == 33)
		i365_bset(sock, VG469_VSELECT, VG469_VSEL_VCC);
	    else if (state->Vcc == 50)
		i365_bclr(sock, VG469_VSELECT, VG469_VSEL_VCC);
	    else return -EINVAL;
	}
    } else if (socket[sock].flags & IS_DF_PWR) {
	switch (state->Vcc) {
	case 0:		break;
	case 33:   	reg |= I365_VCC_3V; break;
	case 50:	reg |= I365_VCC_5V; break;
	default:	return -EINVAL;
	}
	switch (state->Vpp) {
	case 0:		break;
	case 50:   	reg |= I365_VPP_5V; break;
	case 120:	reg |= I365_VPP_12V; break;
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
	case 50:	reg |= I365_VPP_5V; break;
	case 120:	reg |= I365_VPP_12V; break;
	default:	return -EINVAL;
	}
    }
    
    if (reg != i365_get(sock, I365_POWER))
	i365_set(sock, I365_POWER, reg);

    /* Chipset-specific functions */
    if (socket[sock].flags & IS_CIRRUS) {
	/* Speaker control */
	i365_bflip(sock, PD67_MISC_CTL_1, PD67_MC1_SPKR_ENA,
		   state->flags & SS_SPKR_ENA);
	/* DMA control */
	i365_set(sock, PD67_EXT_INDEX, PD67_DMA_CTL);
	reg = i365_get(sock, PD67_EXT_DATA) & ~PD67_DMA_MODE;
	if (state->flags & SS_DMA_MODE) {
	    reg |= PD67_DMA_DREQ_BVD2;
	    i365_set(sock, PD67_EXT_DATA, reg);
	    i365_bset(sock, PD67_MISC_CTL_2, PD67_MC2_DMA_MODE);
	} else {
	    i365_bclr(sock, PD67_MISC_CTL_2, PD67_MC2_DMA_MODE);
	    i365_set(sock, PD67_EXT_DATA, reg);
	}
    }
    
    /* Card status change interrupt mask */
    reg = (socket[sock].cs_irq << 4);
    if (state->csc_mask & SS_DETECT) reg |= I365_CSC_DETECT;
    if (state->flags & SS_IOCARD) {
	if (state->csc_mask & SS_STSCHG) reg |= I365_CSC_STSCHG;
    } else {
	if (state->csc_mask & SS_BATDEAD) reg |= I365_CSC_BVD1;
	if (state->csc_mask & SS_BATWARN) reg |= I365_CSC_BVD2;
	if (state->csc_mask & SS_READY) reg |= I365_CSC_READY;
    }
    i365_set(sock, I365_CSCINT, reg);
    i365_get(sock, I365_CSC);
    
    return 0;
} /* i365_set_socket */

/*====================================================================*/

static int i365_get_io_map(u_short sock, struct pccard_io_map *io)
{
    u_char map, ioctl, addr;
    
    map = io->map;
    if (map > 1) return -EINVAL;
    io->start = i365_get_pair(sock, I365_IO(map)+I365_W_START);
    io->stop = i365_get_pair(sock, I365_IO(map)+I365_W_STOP);
    ioctl = i365_get(sock, I365_IOCTL);
    addr = i365_get(sock, I365_ADDRWIN);
    io->speed = to_ns(ioctl & I365_IOCTL_WAIT(map)) ? 1 : 0;
    io->flags  = (addr & I365_ENA_IO(map)) ? MAP_ACTIVE : 0;
    io->flags |= (ioctl & I365_IOCTL_0WS(map)) ? MAP_0WS : 0;
    io->flags |= (ioctl & I365_IOCTL_16BIT(map)) ? MAP_16BIT : 0;
    io->flags |= (ioctl & I365_IOCTL_IOCS16(map)) ? MAP_AUTOSZ : 0;
    DEBUG(1, "i82365: GetIOMap(%d, %d) = %#2.2x, %d ns, "
	  "%#4.4x-%#4.4x\n", sock, map, io->flags, io->speed,
	  io->start, io->stop);
    return 0;
} /* i365_get_io_map */

/*====================================================================*/

static int i365_set_io_map(u_short sock, struct pccard_io_map *io)
{
    u_char map, ioctl;
    
    DEBUG(1, "i82365: SetIOMap(%d, %d, %#2.2x, %d ns, "
	  "%#4.4x-%#4.4x)\n", sock, io->map, io->flags,
	  io->speed, io->start, io->stop);
    map = io->map;
    if ((map > 1) || (io->start > 0xffff) || (io->stop > 0xffff) ||
	(io->stop < io->start)) return -EINVAL;
    /* Turn off the window before changing anything */
    if (i365_get(sock, I365_ADDRWIN) & I365_ENA_IO(map))
	i365_bclr(sock, I365_ADDRWIN, I365_ENA_IO(map));
    i365_set_pair(sock, I365_IO(map)+I365_W_START, io->start);
    i365_set_pair(sock, I365_IO(map)+I365_W_STOP, io->stop);
    ioctl = i365_get(sock, I365_IOCTL) & ~I365_IOCTL_MASK(map);
    if (io->speed) ioctl |= I365_IOCTL_WAIT(map);
    if (io->flags & MAP_0WS) ioctl |= I365_IOCTL_0WS(map);
    if (io->flags & MAP_16BIT) ioctl |= I365_IOCTL_16BIT(map);
    if (io->flags & MAP_AUTOSZ) ioctl |= I365_IOCTL_IOCS16(map);
    i365_set(sock, I365_IOCTL, ioctl);
    /* Turn on the window if necessary */
    if (io->flags & MAP_ACTIVE)
	i365_bset(sock, I365_ADDRWIN, I365_ENA_IO(map));
    return 0;
} /* i365_set_io_map */

/*====================================================================*/

static int i365_get_mem_map(u_short sock, struct pccard_mem_map *mem)
{
    u_short base, i;
    u_char map, addr;
    
    map = mem->map;
    if (map > 4) return -EINVAL;
    addr = i365_get(sock, I365_ADDRWIN);
    mem->flags = (addr & I365_ENA_MEM(map)) ? MAP_ACTIVE : 0;
    base = I365_MEM(map);
    
    i = i365_get_pair(sock, base+I365_W_START);
    mem->flags |= (i & I365_MEM_16BIT) ? MAP_16BIT : 0;
    mem->flags |= (i & I365_MEM_0WS) ? MAP_0WS : 0;
    mem->sys_start += ((u_long)(i & 0x0fff) << 12);
    
    i = i365_get_pair(sock, base+I365_W_STOP);
    mem->speed  = (i & I365_MEM_WS0) ? 1 : 0;
    mem->speed += (i & I365_MEM_WS1) ? 2 : 0;
    mem->speed = to_ns(mem->speed);
    mem->sys_stop = ((u_long)(i & 0x0fff) << 12) + 0x0fff;
    
    i = i365_get_pair(sock, base+I365_W_OFF);
    mem->flags |= (i & I365_MEM_WRPROT) ? MAP_WRPROT : 0;
    mem->flags |= (i & I365_MEM_REG) ? MAP_ATTRIB : 0;
    mem->card_start = ((u_int)(i & 0x3fff) << 12) + mem->sys_start;
    mem->card_start &= 0x3ffffff;

#ifdef CONFIG_PCI
    /* Take care of high byte, for PCI controllers */
    if (socket[sock].type == IS_PD6729) {
	i365_set(sock, PD67_EXT_INDEX, PD67_MEM_PAGE(map));
	addr = i365_get(sock, PD67_EXT_DATA) << 24;
    } else if (socket[sock].flags & IS_CARDBUS) {
	addr = i365_get(sock, CB_MEM_PAGE(map)) << 24;
	mem->sys_stop += addr; mem->sys_start += addr;
    }
#endif
    
    DEBUG(1, "i82365: GetMemMap(%d, %d) = %#2.2x, %d ns, "
	  "%#5.5lx-%#5.5lx, %#5.5x\n", sock, mem->map, mem->flags,
	  mem->speed, mem->sys_start, mem->sys_stop, mem->card_start);
    return 0;
} /* i365_get_mem_map */

/*====================================================================*/
  
static int i365_set_mem_map(u_short sock, struct pccard_mem_map *mem)
{
    u_short base, i;
    u_char map;
    
    DEBUG(1, "i82365: SetMemMap(%d, %d, %#2.2x, %d ns, "
	  "%#5.5lx-%#5.5lx, %#5.5x)\n", sock, mem->map, mem->flags,
	  mem->speed, mem->sys_start, mem->sys_stop, mem->card_start);

    map = mem->map;
    if ((map > 4) || (mem->card_start > 0x3ffffff) ||
	(mem->sys_start > mem->sys_stop) || (mem->speed > 1000))
	return -EINVAL;
    if (!(socket[sock].flags & (IS_PCI | IS_CARDBUS)) &&
	((mem->sys_start > 0xffffff) || (mem->sys_stop > 0xffffff)))
	return -EINVAL;
	
    /* Turn off the window before changing anything */
    if (i365_get(sock, I365_ADDRWIN) & I365_ENA_MEM(map))
	i365_bclr(sock, I365_ADDRWIN, I365_ENA_MEM(map));

#ifdef CONFIG_PCI
    /* Take care of high byte, for PCI controllers */
    if (socket[sock].type == IS_PD6729) {
	i365_set(sock, PD67_EXT_INDEX, PD67_MEM_PAGE(map));
	i365_set(sock, PD67_EXT_DATA, (mem->sys_start >> 24));
    } else if (socket[sock].flags & IS_CARDBUS)
	i365_set(sock, CB_MEM_PAGE(map), mem->sys_start >> 24);
#endif
    
    base = I365_MEM(map);
    i = (mem->sys_start >> 12) & 0x0fff;
    if (mem->flags & MAP_16BIT) i |= I365_MEM_16BIT;
    if (mem->flags & MAP_0WS) i |= I365_MEM_0WS;
    i365_set_pair(sock, base+I365_W_START, i);
    
    i = (mem->sys_stop >> 12) & 0x0fff;
    switch (to_cycles(mem->speed)) {
    case 0:	break;
    case 1:	i |= I365_MEM_WS0; break;
    case 2:	i |= I365_MEM_WS1; break;
    default:	i |= I365_MEM_WS1 | I365_MEM_WS0; break;
    }
    i365_set_pair(sock, base+I365_W_STOP, i);
    
    i = ((mem->card_start - mem->sys_start) >> 12) & 0x3fff;
    if (mem->flags & MAP_WRPROT) i |= I365_MEM_WRPROT;
    if (mem->flags & MAP_ATTRIB) i |= I365_MEM_REG;
    i365_set_pair(sock, base+I365_W_OFF, i);
    
    /* Turn on the window if necessary */
    if (mem->flags & MAP_ACTIVE)
	i365_bset(sock, I365_ADDRWIN, I365_ENA_MEM(map));
    return 0;
} /* i365_set_mem_map */

/*======================================================================

    Power control for Cardbus controllers: used both for 16-bit and
    Cardbus cards.
    
======================================================================*/

#ifdef CONFIG_PCI

static void cb_get_power(u_short sock, socket_state_t *state)
{
    u_int reg = cb_readl(sock, CB_SOCKET_CONTROL);
    state->Vcc = state->Vpp = 0;
    switch (reg & CB_SC_VCC_MASK) {
    case CB_SC_VCC_3V:		state->Vcc = 33; break;
    case CB_SC_VCC_5V:		state->Vcc = 50; break;
    }
    switch (reg & CB_SC_VCC_MASK) {
    case CB_SC_VPP_3V:		state->Vpp = 33; break;
    case CB_SC_VPP_5V:		state->Vpp = 50; break;
    case CB_SC_VPP_12V:		state->Vpp = 120; break;
    }
}

static void cb_set_power(u_short sock, socket_state_t *state)
{
    u_int reg = 0;
    switch (state->Vcc) {
    case 33:		reg = CB_SC_VCC_3V; break;
    case 50:		reg = CB_SC_VCC_5V; break;
    default:		reg = 0; break;
    }
    switch (state->Vpp) {
    case 33:		reg |= CB_SC_VPP_3V; break;
    case 50:		reg |= CB_SC_VPP_5V; break;
    case 120:		reg |= CB_SC_VPP_12V; break;
    }
    if (reg != cb_readl(sock, CB_SOCKET_CONTROL))
	cb_writel(sock, CB_SOCKET_CONTROL, reg);
}

#endif

/*======================================================================

    All the stuff that is strictly for Cardbus cards goes here.
    
======================================================================*/

#ifdef CONFIG_CARDBUS

static int cb_get_status(u_short sock, u_int *value)
{
    u_int s;
    s = cb_readl(sock, CB_SOCKET_STATE);
    *value = ((s & CB_SS_32BIT) ? SS_CARDBUS : 0);
    *value |= ((s & CB_SS_CCD1) || (s & CB_SS_CCD2)) ? 0 : SS_DETECT;
    *value |= (s & CB_SS_CSTSCHG) ? SS_STSCHG : 0;
    *value |= (s & CB_SS_PWRCYCLE) ? (SS_POWERON|SS_READY) : 0;
    *value |= (s & CB_SS_3VCARD) ? SS_3VCARD : 0;
    *value |= (s & CB_SS_XVCARD) ? SS_XVCARD : 0;
    DEBUG(1, "yenta: GetStatus(%d) = %#4.4x\n", sock, *value);
    return 0;
} /* cb_get_status */

static int cb_get_socket(u_short sock, socket_state_t *state)
{
    socket_info_t *s = &socket[sock];
    u_short bcr;

    cb_get_power(sock, state);
    pci_readw(s->bus, s->devfn, CB_BRIDGE_CONTROL, &bcr);
    state->flags |= (bcr & CB_BCR_CB_RESET) ? SS_RESET : 0;
    state->io_irq = i365_get(sock, I365_INTCTL) & I365_IRQ_MASK;
    DEBUG(1, "yenta: GetSocket(%d) = flags %#3.3x, Vcc %d, Vpp %d, "
	  "io_irq %d, csc_mask %#2.2x\n", sock, state->flags,
	  state->Vcc, state->Vpp, state->io_irq, state->csc_mask);
    return 0;
} /* cb_get_socket */

static int cb_set_socket(u_short sock, socket_state_t *state)
{
    socket_info_t *s = &socket[sock];
    u_int reg;
    u_short bcr;
    
    DEBUG(1, "yenta: SetSocket(%d, flags %#3.3x, Vcc %d, Vpp %d, "
	  "io_irq %d, csc_mask %#2.2x)\n", sock, state->flags,
	  state->Vcc, state->Vpp, state->io_irq, state->csc_mask);
    cb_set_power(sock, state);
    pci_readw(s->bus, s->devfn, CB_BRIDGE_CONTROL, &bcr);
    bcr &= ~CB_BCR_CB_RESET;
    bcr |= (state->flags & SS_RESET) ? CB_BCR_CB_RESET : 0;
    pci_writew(s->bus, s->devfn, CB_BRIDGE_CONTROL, bcr);

    /* Handle IO interrupt using ISA routing */
    reg = i365_get(sock, I365_INTCTL) & ~I365_IRQ_MASK;
    reg |= state->io_irq;
    i365_set(sock, I365_INTCTL, reg);

    /* Handle CSC mask */
    reg = (socket[sock].cs_irq << 4);
    if (state->csc_mask & SS_DETECT) reg |= I365_CSC_DETECT;
    i365_set(sock, I365_CSCINT, reg);
    i365_get(sock, I365_CSC);
    
    return 0;
} /* cb_set_socket */

static int cb_get_bridge(u_short sock, struct cb_bridge_map *m)
{
    socket_info_t *s = &socket[sock];
    u_char map;
    
    map = m->map;
    if (map > 1) return -EINVAL;
    m->flags &= MAP_IOSPACE;
    map += (m->flags & MAP_IOSPACE) ? 2 : 0;
    pci_readl(s->bus, s->devfn, CB_MEM_BASE(map), &m->start);
    pci_readl(s->bus, s->devfn, CB_MEM_LIMIT(map), &m->stop);
    if (m->start || m->stop) {
	m->flags |= MAP_ACTIVE;
	m->stop |= (map > 1) ? 3 : 0x0fff;
    }
    if (map > 1) {
	u_short bcr;
	pci_readw(s->bus, s->devfn, CB_BRIDGE_CONTROL, &bcr);
	m->flags |= (bcr & CB_BCR_PREFETCH(map)) ? MAP_PREFETCH : 0;
    }
    DEBUG(1, "yenta: GetBridge(%d, %d) = %#2.2x, %#4.4x-%#4.4x\n",
	  sock, map, m->flags, m->start, m->stop);
    return 0;
}

static int cb_set_bridge(u_short sock, struct cb_bridge_map *m)
{
    socket_info_t *s = &socket[sock];
    u_char map;
    
    DEBUG(1, "yenta: SetBridge(%d, %d, %#2.2x, %#4.4x-%#4.4x)\n",
	  sock, m->map, m->flags, m->start, m->stop);
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
	pci_readw(s->bus, s->devfn, CB_BRIDGE_CONTROL, &bcr);
	bcr &= ~CB_BCR_PREFETCH(map);
	bcr |= (m->flags & MAP_PREFETCH) ? CB_BCR_PREFETCH(map) : 0;
	pci_writew(s->bus, s->devfn, CB_BRIDGE_CONTROL, bcr);
    }
    if (m->flags & MAP_ACTIVE) {
	pci_writel(s->bus, s->devfn, CB_MEM_BASE(map), m->start);
	pci_writel(s->bus, s->devfn, CB_MEM_LIMIT(map), m->stop);
    } else {
	pci_writel(s->bus, s->devfn, CB_IO_BASE(map), 0);
	pci_writel(s->bus, s->devfn, CB_IO_LIMIT(map), 0);
    }
    return 0;
}

#endif /* CONFIG_CARDBUS */

/*====================================================================*/

#ifdef CONFIG_PROC_FS

static int proc_read_info(char *buf, char **start, off_t pos,
			  int count, int *eof, void *data)
{
    socket_info_t *s = data;
    char *p = buf;
    p += sprintf(p, "type:     %s\npsock:    %d\n",
		 pcic_info[s->type].name, s->psock);
#ifdef CONFIG_PCI
    if (s->flags & (IS_PCI|IS_CARDBUS))
	p += sprintf(p, "bus:      %02x\ndevfn:    %02x\n",
		     s->bus, s->devfn);
#endif
#ifdef CONFIG_CARDBUS
    if (s->flags & IS_CARDBUS)
	p += sprintf(p, "cardbus:  %02x\n", s->cap.cardbus);
#endif
    return (p - buf);
}

static int proc_read_exca(char *buf, char **start, off_t pos,
			  int count, int *eof, void *data)
{
    u_short sock = (socket_info_t *)data - socket;
    char *p = buf;
    int i;
    
    for (i = 0; i < 0x40; i += 4) {
	p += sprintf(p, "%02x %02x %02x %02x%s",
		     i365_get(sock,i), i365_get(sock,i+1),
		     i365_get(sock,i+2), i365_get(sock,i+3),
		     ((i % 16) == 12) ? "\n" : " ");
    }
    return (p - buf);
}


#ifdef CONFIG_PCI
static int proc_read_pci(char *buf, char **start, off_t pos,
			 int count, int *eof, void *data)
{
    socket_info_t *s = data;
    u_char bus = s->bus, devfn = s->devfn;
    char *p = buf;
    u_int a, b, c, d;
    int i;
    
    for (i = 0; i < 0xa0; i += 0x10) {
	pci_readl(bus, devfn, i, &a);
	pci_readl(bus, devfn, i+4, &b);
	pci_readl(bus, devfn, i+8, &c);
	pci_readl(bus, devfn, i+12, &d);
	p += sprintf(p, "%08x %08x %08x %08x\n", a, b, c, d);
    }
    return (p - buf);
}
#endif

#ifdef CONFIG_CARDBUS
static int proc_read_cardbus(char *buf, char **start, off_t pos,
			     int count, int *eof, void *data)
{
    u_short sock = (socket_info_t *)data - socket;
    int len;
    
    len = sprintf(buf, "%08x %08x %08x %08x %08x %08x\n",
		  cb_readl(sock,0), cb_readl(sock,4),
		  cb_readl(sock,8), cb_readl(sock,12),
		  cb_readl(sock,16), cb_readl(sock,32));
    return len;
}
#endif

static void pcic_proc_setup(u_short sock, struct proc_dir_entry *base)
{
    socket_info_t *s = &socket[sock];
    struct proc_dir_entry *ent;
    ent = create_proc_entry("info", 0, base);
    ent->read_proc = proc_read_info;
    ent->data = s;
    ent = create_proc_entry("exca", 0, base);
    ent->read_proc = proc_read_exca;
    ent->data = s;
#ifdef CONFIG_PCI
    if (s->flags & (IS_PCI|IS_CARDBUS)) {
	ent = create_proc_entry("pci", 0, base);
	ent->read_proc = proc_read_pci;
	ent->data = s;
    }
#endif
#ifdef CONFIG_CARDBUS
    if (s->flags & IS_CARDBUS) {
	ent = create_proc_entry("cardbus", 0, base);
	ent->read_proc = proc_read_cardbus;
	ent->data = s;
    }
#endif
    s->proc = base;
}

static void pcic_proc_remove(u_short sock)
{
    struct proc_dir_entry *base = socket[sock].proc;
    if (base == NULL) return;
    remove_proc_entry("info", base);
    remove_proc_entry("exca", base);
#ifdef CONFIG_PCI
    remove_proc_entry("exca", base);
#endif
#ifdef CONFIG_CARDBUS
    remove_proc_entry("exca", base);
#endif
}

#endif /* CONFIG_PROC_FS */

/*====================================================================*/

typedef int (*subfn_t)(u_short, void *);

static subfn_t i365_service_table[] = {
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
#ifdef CONFIG_PROC_FS
    (subfn_t)&pcic_proc_setup
#endif
};

#define NFUNC (sizeof(i365_service_table)/sizeof(subfn_t))

static int pcic_service(u_int sock, u_int cmd, void *arg)
{
    subfn_t fn;

    DEBUG(2, "pcic_ioctl(%d, %d, 0x%p)\n", sock, cmd, arg);

    if (cmd >= NFUNC)
	return -EINVAL;

    fn = i365_service_table[cmd];
#ifdef CONFIG_CARDBUS
    if ((socket[sock].flags & IS_CARDBUS) &&
	(cb_readl(sock, CB_SOCKET_STATE) & CB_SS_32BIT)) {
	if (cmd == SS_GetStatus)
	    fn = (subfn_t)&cb_get_status;
	else if (cmd == SS_GetSocket)
	    fn = (subfn_t)&cb_get_socket;
	else if (cmd == SS_SetSocket)
	    fn = (subfn_t)&cb_set_socket;
    }
#endif

#ifdef CONFIG_ISA
    if (!(socket[sock].flags & IS_CARDBUS)) {
	int ret;
	u_long flags;
	spin_lock_irqsave(&isa_lock, flags);
	ret = (fn == NULL) ? -EINVAL : fn(sock, arg);
	spin_unlock_irqrestore(&isa_lock, flags);
	return ret;
    }
#endif
    return (fn == NULL) ? -EINVAL : fn(sock, arg);
} /* pcic_service */

/*====================================================================*/

int init_module(void)
{
    servinfo_t serv;
    CardServices(GetCardServicesInfo, &serv);
    if (serv.Revision != CS_RELEASE_CODE) {
	printk(KERN_NOTICE "i82365: Card Services release "
	       "does not match!\n");
	return -1;
    }
    return pcic_init();
}

void cleanup_module(void)
{
    pcic_finish();
}
