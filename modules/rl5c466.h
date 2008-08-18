/*
 * rl5c466.h 1.5 1998/05/10 11:59:46
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License
 * at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and
 * limitations under the License. 
 *
 * The initial developer of the original code is David A. Hinds
 * <dhinds@hyper.stanford.edu>.  Portions created by David A. Hinds
 * are Copyright (C) 1998 David A. Hinds.  All Rights Reserved.
 */

#ifndef _LINUX_RL5C466_H
#define _LINUX_RL5C466_H

#ifndef PCI_VENDOR_ID_RICOH
#define PCI_VENDOR_ID_RICOH		0x1180
#endif

#ifndef PCI_DEVICE_ID_RICOH_RL5C466
#define PCI_DEVICE_ID_RICOH_RL5C466	0x0466
#endif

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
