/*
 * smc34c90.h 1.1 1996/11/20 07:49:24 (David Hinds)
 */

#ifndef _LINUX_RL5C466_H
#define _LINUX_RL5C466_H

/* Register definitions for SMC 34C90 PCI-to-CardBus bridge */

/* EEPROM Information Register */
#define SMC34C90_EEINFO			0x0088
#define SMC34C90_EEINFO_ONE_SOCKET	0x0001
#define SMC34C90_EEINFO_5V_ONLY		0x0002
#define SMC34C90_EEINFO_ISA_IRQ		0x0004
#define SMC34C90_EEINFO_ZV_PORT		0x0008
#define SMC34C90_EEINFO_RING		0x0010
#define SMC34C90_EEINFO_LED		0x0020

#endif /* _LINUX_SMC34C90_H */
