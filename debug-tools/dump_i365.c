/*======================================================================

    Register dump for the Intel 82365SL controller family

    dump_i365.c 1.21 1998/07/26 23:10:48

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

#include <string.h>
#include <errno.h>
#include <stdio.h>

#ifdef __MSDOS__

#include <dos.h>
typedef unsigned char u_char;
typedef unsigned short u_short;
#define INB(a) inportb(a)
#define OUTB(d, a) outportb(a, d)

#else /* __MSDOS__ */

#include <sys/types.h>
#ifdef __GLIBC__
#include <sys/io.h>
#else
#include <asm/io.h>
#endif
#include <unistd.h>
#define INB(a) inb_p(a)
#define OUTB(d, a) outb_p(d, a)

#endif /* __MSDOS__ */

#include "i82365.h"
#include "cirrus.h"
#include "vg468.h"

static int i365_base = 0x03e0;
static int type = 0;
#define IS_CIRRUS	1
#define IS_VG468	2
#define IS_VG469	3

/*====================================================================*/

static u_char i365_get(u_short sock, u_short reg)
{
    u_char val = I365_REG(sock, reg);
    OUTB(val, i365_base); val = INB(i365_base+1);
    return val;
}

static void i365_set(u_short sock, u_short reg, u_char data)
{
    u_char val = I365_REG(sock, reg);
    OUTB(val, i365_base); OUTB(data, i365_base+1);
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

static u_short i365_get_pair(u_short sock, u_short reg)
{
    u_short a, b;
    a = i365_get(sock, reg);
    b = i365_get(sock, reg+1);
    return (a + (b<<8));
}

/*====================================================================*/

static int i365_probe(void)
{
    int val, sock, done;
    char *name = "", tmp[20];

    printf("Intel PCIC probe: ");
    
    sock = done = 0;
    
#ifndef __MSDOS__
    ioperm(i365_base, 4, 1);
    ioperm(0x80, 1, 1);
#endif
    
    for (; sock < 8; sock++) {
	val = i365_get(sock, I365_IDENT);
	switch (val) {
	case 0x82:
	    name = "i82365sl A step";
	    break;
	case 0x83:
	    name = "i82365sl B step";
	    break;
	case 0x84:
	    name = "VLSI 82C146";
	    break;
	case 0x88: case 0x89:
	    name = "IBM Clone";
	    break;
	case 0x8b: case 0x8c:
	    /* Special: fall into Vadem test */
	    break;
	default:
	    done = 1;
	}
	if (done) break;
	i365_set(sock, I365_MEM(3)+I365_W_OFF, sock);
    }
    
    if (sock == 0) {
	printf("not found.\n");
	return -ENODEV;
    }
    
    /* Check for bogus clones that ignore top bit of index register */
    if ((sock == 4) && (i365_get(0, I365_MEM(3)+I365_W_OFF) == 2))
	sock = 2;

    /* Check for Vadem chips */
    OUTB(0x0e, i365_base);
    OUTB(0x37, i365_base);
    i365_bset(0, VG468_MISC, VG468_MISC_VADEMREV);
    val = i365_get(0, I365_IDENT);
    if (val & I365_IDENT_VADEM) {
	switch (val & 7) {
	case 3:
	    name = "Vadem VG-468"; type = IS_VG468; break;
	case 4:
	    name = "Vadem VG-469"; type = IS_VG469; break;
	default:
	    sprintf(tmp, "Vadem rev %d", val & 7);
	    name = tmp; type = IS_VG469; break;
	}
	i365_bclr(0, VG468_MISC, VG468_MISC_VADEMREV);
    }
    
    /* Check for Cirrus CL-PD67xx chips */
    i365_set(0, PD67_CHIP_INFO, 0);
    val = i365_get(0, PD67_CHIP_INFO);
    if ((val & PD67_INFO_CHIP_ID) == PD67_INFO_CHIP_ID) {
	val = i365_get(0, PD67_CHIP_INFO);
	if ((val & PD67_INFO_CHIP_ID) == 0) {
	    type = IS_CIRRUS;
	    if (val & PD67_INFO_SLOTS)
		name = "Cirrus CL-PD672x";
	    else {
		name = "Cirrus CL-PD6710";
		sock = 1;
	    }
	}
    }

    printf("%s found, %d sockets\n", name, sock);
    return sock;
    
} /* i365_probe */
  
/*====================================================================*/

static void dump_status(int s)
{
    int v = i365_get(s, I365_STATUS);
    printf("  Interface status = %#2.2x\n", v);
    printf("   ");
    if (v & I365_CS_BVD1) printf(" [bvd1/stschg]");
    if (v & I365_CS_BVD2) printf(" [bvd2/spkr]");
    if (v & I365_CS_DETECT) printf(" [detect]");
    if (v & I365_CS_WRPROT) printf(" [wrprot]");
    if (v & I365_CS_READY) printf(" [ready]");
    if (v & I365_CS_POWERON) printf(" [poweron]");
    if (v & I365_CS_GPI) printf(" [gpi]");
    printf("\n");
}

static void dump_power(int s)
{
    int v = i365_get(s, I365_POWER);
    printf("  Power control = %#2.2x\n", v);
    printf("   ");
    if (v & I365_PWR_OUT) printf(" [output]");
    if (!(v & I365_PWR_NORESET)) printf(" [resetdrv]");
    if (v & I365_PWR_AUTO) printf(" [auto]");
    switch (v & I365_VCC_MASK) {
    case I365_VCC_5V:
	printf(" [Vcc=5v]"); break;
    case I365_VCC_3V:
	printf(" [Vcc=3.3v]"); break;
    case 0:
	printf(" [Vcc off]"); break;
    }
    switch (v & I365_VPP1_MASK) {
    case I365_VPP1_5V:
	printf(" [Vpp=5v]"); break;
    case I365_VPP1_12V:
	printf(" [Vpp=12v]"); break;
    case 0:
	printf(" [Vpp off]"); break;
    }
    printf("\n");
}

static void dump_intctl(int s)
{
    int v = i365_get(s, I365_INTCTL);
    printf("  Interrupt and general control = %#2.2x\n", v);
    printf("   ");
    if (v & I365_RING_ENA) printf(" [ring ena]");
    if (!(v & I365_PC_RESET)) printf(" [reset]");
    if (v & I365_PC_IOCARD) printf(" [iocard]");
    if (v & I365_INTR_ENA) printf(" [intr ena]");
    printf(" [irq=%d]\n", v & I365_IRQ_MASK);
}

static void dump_csc(int s)
{
    int v = i365_get(s, I365_CSC);
    printf("  Card status change = %#2.2x\n", v);
    printf("   ");
    if (v & I365_CSC_BVD1) printf(" [bvd1/stschg]");
    if (v & I365_CSC_BVD2) printf(" [bvd2]");
    if (v & I365_CSC_DETECT) printf(" [detect]");
    if (v & I365_CSC_READY) printf(" [ready]");
    if (v & I365_CSC_GPI) printf(" [gpi]");
    printf("\n");
}

static void dump_cscint(int s)
{
    int v = i365_get(s, I365_CSCINT);
    printf("  Card status change interrupt control = %#2.2x\n", v);
    printf("   ");
    if (v & I365_CSC_BVD1) printf(" [bvd1/stschg]");
    if (v & I365_CSC_BVD2) printf(" [bvd2]");
    if (v & I365_CSC_DETECT) printf(" [detect]");
    if (v & I365_CSC_READY) printf(" [ready]");
    printf(" [irq = %d]\n", v >> 4);
}

static void dump_genctl(int s)
{
    int v = i365_get(s, I365_GENCTL);
    printf("  Card detect and general control = %#2.2x\n", v);
    printf("   ");
    if (v & I365_CTL_16DELAY) printf(" [16delay]");
    if (v & I365_CTL_RESET) printf(" [reset]");
    if (v & I365_CTL_GPI_ENA) printf(" [gpi ena]");
    if (v & I365_CTL_GPI_CTL) printf(" [gpi ctl]");
    if (v & I365_CTL_RESUME) printf(" [resume]");
    printf("\n");
}

static void dump_gblctl(int s)
{
    int v = i365_get(s, I365_GBLCTL);
    printf("  Global control = %#2.2x\n", v);
    printf("   ");
    if (v & I365_GBL_PWRDOWN) printf(" [pwrdown]");
    if (v & I365_GBL_CSC_LEV) printf(" [csc level]");
    if (v & I365_GBL_WRBACK) printf(" [wrback]");
    if (v & I365_GBL_IRQ_0_LEV) printf(" [irq 0 lev]");
    if (v & I365_GBL_IRQ_1_LEV) printf(" [irq 1 lev]");
    printf("\n");
}

/*====================================================================*/

/* Cirrus-specific registers */

static void dump_misc1(int s)
{
    int v = i365_get(s, PD67_MISC_CTL_1);
    printf("  Misc control 1 = %#2.2x\n", v);
    printf("   ");
    if (v & PD67_MC1_5V_DET) printf(" [5v detect]");
    if (v & PD67_MC1_VCC_3V) printf(" [Vcc 3.3v]");
    if (v & PD67_MC1_PULSE_MGMT) printf(" [pulse mgmt]");
    if (v & PD67_MC1_PULSE_IRQ) printf(" [pulse irq]");
    if (v & PD67_MC1_SPKR_ENA) printf(" [spkr]");
    if (v & PD67_MC1_INPACK_ENA) printf(" [inpack]");
    printf("\n");
}

static void dump_misc2(int s)
{
    int v = i365_get(s, PD67_MISC_CTL_2);
    printf("  Misc control 2 = %#2.2x\n", v);
    printf("   ");
    if (v & PD67_MC2_FREQ_BYPASS) printf(" [freq bypass]");
    if (v & PD67_MC2_DYNAMIC_MODE) printf(" [dynamic mode]");
    if (v & PD67_MC2_SUSPEND) printf(" [suspend]");
    if (v & PD67_MC2_5V_CORE) printf(" [5v core]");
    if (v & PD67_MC2_LED_ENA) printf(" [LED ena]");
    if (v & PD67_MC2_3STATE_BIT7) printf(" [3state bit 7]");
    if (v & PD67_MC2_DMA_MODE) printf(" [DMA mode]");
    if (v & PD67_MC2_IRQ15_RI) printf(" [irq 15 is RI]");
    printf("\n");
}

static void print_time(char *s, int v)
{
    printf("%s = %d", s, v & PD67_TIME_MULT);
    switch (v & PD67_TIME_SCALE) {
    case PD67_TIME_SCALE_16:
	printf(" [*16]"); break;
    case PD67_TIME_SCALE_256:
	printf(" [*256]"); break;
    case PD67_TIME_SCALE_4096:
	printf(" [*4096]"); break;
    }
}

static void dump_timing(int s, int i)
{
    printf("  Timing set %d: ", i);
    print_time("setup", i365_get(s, PD67_TIME_SETUP(i)));
    print_time(", command", i365_get(s, PD67_TIME_CMD(i)));
    print_time(", recovery", i365_get(s, PD67_TIME_RECOV(i)));
    printf("\n");
}

void dump_ext(int s)
{
    u_char v;
    printf("  Extension registers:");
    printf("    ");
    i365_set(s, PD67_EXT_INDEX, PD67_DATA_MASK0);
    v = i365_get(s, PD67_EXT_DATA);
    printf("mask 0 = %#2.2x", v);
    i365_set(s, PD67_EXT_INDEX, PD67_DATA_MASK1);
    v = i365_get(s, PD67_EXT_DATA);
    printf(", mask 1 = %#2.2x", v);
    i365_set(s, PD67_EXT_INDEX, PD67_DMA_CTL);
    v = i365_get(s, PD67_EXT_DATA);
    printf(", DMA ctl = %#2.2x", v);
    switch (v & PD67_DMA_MODE) {
    case PD67_DMA_OFF:
	printf(" [off]"); break;
    case PD67_DMA_DREQ_INPACK:
	printf(" [dreq is inpack]"); break;
    case PD67_DMA_DREQ_WP:
	printf(" [dreq is wp]"); break;
    case PD67_DMA_DREQ_BVD2:
	printf(" [dreq is bvd2]"); break;
    }
    if (v & PD67_DMA_PULLUP)
	printf(" [pullup]");
    printf("\n");
}

/*====================================================================*/

/* Vadem-specific registers */

static void dump_vsense(int s)
{
    int v = i365_get(s, VG469_VSENSE);
    printf("  Card voltage sense = %#2.2x\n", v);
    printf("   ");
    if (v & VG469_VSENSE_A_VS1) printf(" [a_vs1]");
    if (v & VG469_VSENSE_A_VS2) printf(" [a_vs2]");
    if (v & VG469_VSENSE_B_VS1) printf(" [b_vs1]");
    if (v & VG469_VSENSE_B_VS2) printf(" [b_vs2]");
    printf("\n");
}

static void dump_vselect(int s)
{
    int v = i365_get(s, VG469_VSELECT);
    printf("  Card voltage select = %#2.2x\n", v);
    printf("   ");
    switch (v & VG469_VSEL_VCC) {
    case 0: printf(" [Vcc=5v]"); break;
    case 1: printf(" [Vcc=3.3v]"); break;
    case 2: printf(" [Vcc=X.Xv]"); break;
    case 3: printf(" [Vcc=3.3v]"); break;
    }
    switch (v & VG469_VSEL_MAX) {
    case 0: printf(" [Vmax=5v]"); break;
    case 1: printf(" [Vmax=3.3v]"); break;
    case 2: printf(" [Vmax=X.Xv]"); break;
    case 3: printf(" [Vcc=3.3v]"); break;
    }
    if (v & VG469_VSEL_EXT_STAT) printf(" [extended]");
    if (v & VG469_VSEL_EXT_BUS) printf(" [buffer]");
    if (v & VG469_VSEL_MIXED)
	printf(" [mixed]");
    else
	printf(" [5v only]");
    if (v & VG469_VSEL_ISA)
	printf(" [3v bus]");
    else
	printf(" [5v bus]");
    printf("\n");
}

static void dump_control(int s)
{
    int v = i365_get(s, VG468_CTL);
    printf("  Control register = %#2.2x\n", v);
    printf("   ");
    if (v & VG468_CTL_SLOW) printf(" [slow]");
    if (v & VG468_CTL_ASYNC) printf(" [async]");
    if (v & VG468_CTL_TSSI) printf(" [tri-state]");
    if (v & VG468_CTL_DELAY) printf(" [debounce]");
    if (v & VG469_CTL_STRETCH) printf(" [stretch]");
    if (v & VG468_CTL_INPACK) printf(" [inpack]");
    if (v & VG468_CTL_POLARITY)
	printf(" [active high]");
    else
	printf(" [active low]");
    if (v & VG468_CTL_COMPAT) printf(" [compat]");
    printf("\n");
}

static void dump_misc(int s)
{
    int v = i365_get(s, VG468_MISC);
    printf("  Misc register = %#2.2x\n", v);
    printf("   ");
    if (v & VG468_MISC_GPIO) printf(" [gpio]");
    if (v & VG468_MISC_DMAWSB) printf(" [DMA ws]");
    if (v & VG469_MISC_LEDENA) printf(" [LED ena]");
    if (v & VG468_MISC_VADEMREV) printf(" [Vadem rev]");
    if (v & VG468_MISC_UNLOCK) printf(" [unlock]");
    printf("\n");
}

static void dump_ext_mode(int s)
{
    int v = i365_get(s, VG469_EXT_MODE);
    printf("  Extended mode %c = %#2.2x\n", (s ? 'B' : 'A'), v);
    printf("   ");
    if (s) {
	if (v & VG469_MODE_B_3V) printf(" [3.3v sock B]");
    }
    else {
	if (v & VG469_MODE_INT_SENSE) printf(" [int sense]");
	if (v & VG469_MODE_CABLE) printf(" [cable mode]");
	if (v & VG469_MODE_COMPAT) printf(" [DF compat]");
	if (v & VG469_MODE_TEST) printf(" [test]");
	if (v & VG469_MODE_RIO) printf(" [RIO to INTR]");
    }
    printf("\n");
}

/*====================================================================*/

static void dump_memwin(int s, int w)
{
    u_short start, stop, off;
    printf("  Memory window %d:", w);
    if (i365_get(s, I365_ADDRWIN) & I365_ENA_MEM(w))
	printf(" [ON]");
    else
	printf(" [OFF]");
    start = i365_get_pair(s, I365_MEM(w)+I365_W_START);
    stop = i365_get_pair(s, I365_MEM(w)+I365_W_STOP);
    off = i365_get_pair(s, I365_MEM(w)+I365_W_OFF);
    if (start & I365_MEM_16BIT) printf(" [16BIT]");
    if (type == IS_CIRRUS) {
	if (stop & I365_MEM_WS1)
	    printf(" [TIME1]");
	else
	    printf(" [TIME0]");
    }
    else {
	if (start & I365_MEM_0WS) printf(" [0WS]");
	if (stop & I365_MEM_WS1) printf(" [WS1]");
	if (stop & I365_MEM_WS0) printf(" [WS0]");
    }
    if (off & I365_MEM_WRPROT) printf(" [WRPROT]");
    if (off & I365_MEM_REG) printf(" [REG]");
    printf("\n    start = %#4.4x", start & 0x3fff);
    printf(", stop = %#4.4x", stop & 0x3fff);
    printf(", offset = %#4.4x\n", off & 0x3fff);
}

static void dump_iowin(int s, int w)
{
    u_short ctl, start, stop, off;
    printf("  I/O window %d:", w);
    if (i365_get(s, I365_ADDRWIN) & I365_ENA_IO(w))
	printf(" [ON]");
    else
	printf(" [OFF]");
    
    ctl = i365_get(s, I365_IOCTL);
    if (type == IS_CIRRUS) {
	if (ctl & I365_IOCTL_WAIT(w))
	    printf(" [TIME1]");
	else
	    printf(" [TIME0]");
    }
    else {
	if (ctl & I365_IOCTL_WAIT(w)) printf(" [WAIT]");
	if (ctl & I365_IOCTL_0WS(w)) printf(" [0WS]");
    }
    if (ctl & I365_IOCTL_IOCS16(w)) printf(" [IOCS16]");
    if (ctl & I365_IOCTL_16BIT(w)) printf(" [16BIT]");
    
    start = i365_get_pair(s, I365_IO(w)+I365_W_START);
    stop = i365_get_pair(s, I365_IO(w)+I365_W_STOP);
    printf("\n    start = %#4.4x, stop = %#4.4x", start, stop);

    if (type == IS_CIRRUS) {
	off = i365_get_pair(s, PD67_IO_OFF(w));
	printf(", offset = %#4.4x", off);
    }
    printf("\n");
}

/*====================================================================*/

void dump_sock(int s)
{
    int i;
    printf("  Identification and revision = %#2.2x\n",
	   i365_get(s, I365_IDENT));
    if (type == IS_CIRRUS)
	printf("  Chip information = %#2.2x\n",
	       i365_get(s, PD67_CHIP_INFO));
    dump_status(s);
    dump_power(s);
    if (type == IS_VG469)
	dump_vselect(s);
    dump_intctl(s);
    dump_csc(s);
    dump_cscint(s);
    
    switch (type) {
    case IS_CIRRUS:
	dump_misc1(s);
	dump_misc2(s);
	break;
    case IS_VG469:
	dump_vsense(s);
	dump_ext_mode(s);
	/* fall through */
    case IS_VG468:
	dump_control(s);
	dump_misc(s);
	/* fall through */
    default:
	dump_genctl(s);
	dump_gblctl(s);
    }
    
    for (i = 0; i < 5; i++)
	dump_memwin(s, i);
    for (i = 0; i < 2; i++)
	dump_iowin(s, i);
    
    if (type == IS_CIRRUS) {
	for (i = 0; i < 2; i++)
	    dump_timing(s, i);
	dump_ext(s);
    }
    printf("\n");
} /* dump_sock */

/*====================================================================*/

void main(int argc, char *argv[])
{
    int sock, i;
    
    sock = i365_probe();
    for (i = 0; i < sock; i++) {
	printf("Socket %d:\n", i);
	dump_sock(i);
    }
}
