/*
 * pd6832.h 1.3 1997/12/17 18:26:11 (David Hinds)
 */

#ifndef _LINUX_PD6832_H
#define _LINUX_PD6832_H

/* Register definitions for Cirrus PD6832 PCI-to-CardBus bridge */

/* PD6832 extension registers -- indexed in PD67_EXT_INDEX */
#define PD68_EXT_CTL_2			0x0b
#define PD68_PCI_SPACE			0x22
#define PD68_PCCARD_SPACE		0x23
#define PD68_WINDOW_TYPE		0x24
#define PD68_EXT_CSC			0x2e
#define PD68_MISC_CTL_4			0x2f
#define PD68_MISC_CTL_5			0x30
#define PD68_MISC_CTL_6			0x31

/* Extra flags in PD67_MISC_CTL_3 */
#define PD68_MC3_HW_SUSP		0x10
#define PD68_MC3_MM_EXPAND		0x40
#define PD68_MC3_MM_ARM			0x80

/* Bridge Control Register */
#define  PD6832_BCR_MGMT_IRQ_ENA	0x0800

/* Socket Number Register */
#define PD6832_SOCKET_NUMBER		0x004c	/* 8 bit */

#endif /* _LINUX_PD6832_H */
