/*
 * yenta.h 1.8 1998/01/16 07:31:25 (David Hinds)
 */

#ifndef _LINUX_YENTA_H
#define _LINUX_YENTA_H

#ifndef PCI_VENDOR_ID_RICOH
#define PCI_VENDOR_ID_RICOH		0x1180
#endif

#ifndef PCI_DEVICE_ID_RICOH_RL5C466
#define PCI_DEVICE_ID_RICOH_RL5C466	0x0466
#endif

#ifndef PCI_VENDOR_ID_SMC
#define PCI_VENDOR_ID_SMC		0x10b3
#endif

#ifndef PCI_DEVICE_ID_SMC_34C90
#define PCI_DEVICE_ID_SMC_34C90		0xb106
#endif

#ifndef PCI_DEVICE_ID_CIRRUS_6832
#define PCI_DEVICE_ID_CIRRUS_6832	0x1110
#endif

#ifndef PCI_VENDOR_ID_TI
#define PCI_VENDOR_ID_TI		0x104c
#endif

#ifndef PCI_DEVICE_ID_TI_1130
#define PCI_DEVICE_ID_TI_1130		0xac12
#endif

#ifndef PCI_DEVICE_ID_TI_1131
#define PCI_DEVICE_ID_TI_1131		0xac15
#endif

#ifndef PCI_VENDOR_ID_O2_MICRO
#define PCI_VENDOR_ID_O2_MICRO		0x1217
#endif

#ifndef PCI_DEVICE_ID_O2_6832
#define PCI_DEVICE_ID_O2_6832		0x6832
#endif

/* PCI Configuration Registers */

#define CB_CONTROL_REG_BASE		0x0010

#define CB_STATUS			0x0016	/* 16 bit */
#define  CB_STATUS_FAST_BTB		0x0080
#define  CB_STATUS_DATA_PARITY_ERR	0x0100
#define  CB_STATUS_CDEVSEL		0x0600
#define  CB_STATUS_SIGNAL_ABORT		0x0800
#define  CB_STATUS_RECV_ABORT		0x1000
#define  CB_STATUS_MASTER_ABORT		0x2000
#define  CB_STATUS_SYSTEM_ERROR		0x4000
#define  CB_STATUS_PARITY_ERROR		0x8000

#define CB_PCI_BUS			0x0018	/* 8 bit */
#define CB_CARDBUS_BUS			0x0019	/* 8 bit */
#define CB_SUBORD_BUS			0x001a	/* 8 bit */
#define CB_LATENCY_TIMER		0x001b	/* 8 bit */

#define CB_MEM_BASE(m)			(0x001c + 8*(m))
#define CB_MEM_LIMIT(m)			(0x0020 + 8*(m))
#define CB_IO_BASE(m)			(0x002c + 8*(m))
#define CB_IO_LIMIT(m)			(0x0030 + 8*(m))

#define CB_BRIDGE_CONTROL		0x003e	/* 16 bit */
#define  CB_BCR_PARITY_ENA		0x0001
#define  CB_BCR_SERR_ENA		0x0002
#define  CB_BCR_ISA_ENA			0x0004
#define  CB_BCR_VGA_ENA			0x0008
#define  CB_BCR_MABORT			0x0020
#define  CB_BCR_CB_RESET		0x0040
#define  CB_BCR_ISA_IRQ			0x0080
#define  CB_BCR_PREFETCH(m)		(0x0100 << (m))
#define  CB_BCR_WRITE_POST		0x0400

#define CB_SUBSYSTEM_VENDOR_ID	0x0040	/* 16 bit */
#define CB_SUBSYSTEM_ID		0x0042	/* 16 bit */
#define CB_LEGACY_MODE_BASE	0x0044

/* Memory mapped registers */

#define CB_SOCKET_EVENT			0x0000
#define  CB_SE_CSTSCHG			0x00000001
#define  CB_SE_CCD1			0x00000002
#define  CB_SE_CCD2			0x00000004
#define  CB_SE_PWRCYCLE			0x00000008

#define CB_SOCKET_MASK			0x0004
#define  CB_SM_CSTSCHG			0x00000001
#define  CB_SM_CCD			0x00000006
#define  CB_SM_PWRCYCLE			0x00000008

#define CB_SOCKET_STATE			0x0008
#define  CB_SS_CSTSCHG			0x00000001
#define  CB_SS_CCD1			0x00000002
#define  CB_SS_CCD2			0x00000004
#define  CB_SS_PWRCYCLE			0x00000008
#define  CB_SS_16BIT			0x00000010
#define  CB_SS_32BIT			0x00000020
#define  CB_SS_CINT			0x00000040
#define  CB_SS_BADCARD			0x00000080
#define  CB_SS_DATALOST			0x00000100
#define  CB_SS_BADVCC			0x00000200
#define  CB_SS_5VCARD			0x00000400
#define  CB_SS_3VCARD			0x00000800
#define  CB_SS_XVCARD			0x00001000
#define  CB_SS_YVCARD			0x00002000
#define  CB_SS_5VSOCKET			0x10000000
#define  CB_SS_3VSOCKET			0x20000000
#define  CB_SS_XVSOCKET			0x40000000
#define  CB_SS_YVSOCKET			0x80000000

#define CB_SOCKET_FORCE			0x000c
#define  CB_SF_CVSTEST			0x00004000

#define CB_SOCKET_CONTROL		0x0010
#define  CB_SC_VPP_MASK			0x00000007
#define   CB_SC_VPP_OFF			0x00000000
#define   CB_SC_VPP_12V			0x00000001
#define   CB_SC_VPP_5V			0x00000002
#define   CB_SC_VPP_3V			0x00000003
#define   CB_SC_VPP_XV			0x00000004
#define   CB_SC_VPP_YV			0x00000005
#define  CB_SC_VCC_MASK			0x00000070
#define   CB_SC_VCC_OFF			0x00000000
#define   CB_SC_VCC_5V			0x00000020
#define   CB_SC_VCC_3V			0x00000030
#define   CB_SC_VCC_XV			0x00000040
#define   CB_SC_VCC_YV			0x00000050
#define  CB_SC_CCLK_STOP		0x00000080

#define CB_SOCKET_POWER			0x0020
#define  CB_SP_CLK_CTRL			0x00000001
#define  CB_SP_CLK_CTRL_ENA		0x00010000
#define  CB_SP_CLK_MODE			0x01000000
#define  CB_SP_ACCESS			0x02000000

/* Address bits 31..24 for memory windows for 16-bit cards,
   accessable only by memory mapping the 16-bit register set */
#define CB_MEM_PAGE(map)		(0x0840 + (map))

#endif /* _LINUX_YENTA_H */
