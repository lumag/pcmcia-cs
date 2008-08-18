/*
 * pd6832.h 1.7 1998/05/10 11:59:46
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

#ifndef _LINUX_PD6832_H
#define _LINUX_PD6832_H

#ifndef PCI_DEVICE_ID_CIRRUS_6832
#define PCI_DEVICE_ID_CIRRUS_6832	0x1110
#endif

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
