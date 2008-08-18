/*
 * rl5c466.h 1.2 1997/05/05 01:22:17 (David Hinds)
 */

#ifndef _LINUX_RL5C466_H
#define _LINUX_RL5C466_H

/* Register definitions for Ricoh RL5C466 PCI-to-CardBus bridge */

/* Extra bits in CB_BRIDGE_CONTROL */
#define RL5C466_BCR_3E0_ENA		0x0800
#define RL5C466_BCR_3E2_ENA		0x1000

/* Misc Control 0 Register */
#define RL5C466_MISC0			0x0082	/* 16 bit */
#define  RL5C466_MISC0_SUSPEND		0x0001
#define  RL5C466_MISC0_HW_SUSPEND_ENA	0x0002
#define  RL5C466_MISC0_PWR_SAVE_2	0x0004
#define  RL5C466_MISC0_IFACE_BUSY	0x0008
#define  RL5C466_MISC0_B_LOCK		0x0010
#define  RL5C466_MISC0_A_LOCK		0x0020
#define  RL5C466_MISC0_PCI_LOCK		0x0040
#define  RL5C466_MISC0_VCCEN_POL	0x0100
#define  RL5C466_MISC0_VPPEN_POL	0x0200

/* 16-bit Interface Control Register */
#define RL5C466_16BIT_CTL		0x0084	/* 16 bit */
#define  RL5C466_16CTL_INDEX_SEL	0x0008
#define  RL5C466_16CTL_LEVEL_1		0x0010
#define  RL5C466_16CTL_LEVEL_2		0x0020
#define  RL5C466_16CTL_IO_TIMING	0x0100
#define  RL5C466_16CTL_MEM_TIMING	0x0200

/* 16-bit IO and memory timing registers */
#define RL5C466_16BIT_IO_0		0x0088	/* 16 bit */
#define RL5C466_16BIT_MEM_0		0x0088	/* 16 bit */
#define  RL5C466_SETUP_MASK		0x0007
#define  RL5C466_SETUP_SHIFT		0
#define  RL5C466_CMD_MASK		0x01f0
#define  RL5C466_CMD_SHIFT		4
#define  RL5C466_HOLD_MASK		0x1c00
#define  RL5C466_HOLD_SHIFT		10

#endif /* _LINUX_RL5C466_H */
