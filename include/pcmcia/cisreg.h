/*
 * cisreg.h 1.8 1997/12/15 07:51:34 (David Hinds)
 */

#ifndef _LINUX_CISREG_H
#define _LINUX_CISREG_H

/* Offsets from ConfigBase for CIS registers */
#define CISREG_COR		0x00
#define CISREG_CCSR		0x02
#define CISREG_PRR		0x04
#define CISREG_SCR		0x06
#define CISREG_ESR		0x08
#define CISREG_IOBASE_0		0x0a
#define CISREG_IOBASE_1		0x0c
#define CISREG_IOBASE_2		0x0e
#define CISREG_IOBASE_3		0x10
#define CISREG_IOSIZE		0x12

/*
 * Configuration Option Register
 */
#define COR_CONFIG_MASK		0x3f
#define COR_MFC_CONFIG_MASK	0x38
#define COR_FUNC_ENA		0x01
#define COR_ADDR_DECODE		0x02
#define COR_IREQ_ENA		0x04
#define COR_LEVEL_REQ		0x40
#define COR_SOFT_RESET		0x80

/*
 * Card Configuration and Status Register
 */
#define CCSR_INTR_ACK		0x01
#define CCSR_INTR_PENDING	0x02
#define CCSR_POWER_DOWN		0x04
#define CCSR_AUDIO_ENA		0x08
#define CCSR_IOIS8		0x20
#define CCSR_SIGCHG_ENA		0x40
#define CCSR_CHANGED		0x80

/*
 * Pin Replacement Register
 */
#define PRR_WP_STATUS		0x01
#define PRR_READY_STATUS	0x02
#define PRR_BVD2_STATUS		0x04
#define PRR_BVD1_STATUS		0x08
#define PRR_WP_EVENT		0x10
#define PRR_READY_EVENT		0x20
#define PRR_BVD2_EVENT		0x40
#define PRR_BVD1_EVENT		0x80

/*
 * Socket and Copy Register
 */
#define SCR_SOCKET_NUM		0x0f
#define SCR_COPY_NUM		0x70

/*
 * Extended Status Register
 */
#define ESR_REQ_ATTN_ENA	0x01
#define ESR_REQ_ATTN		0x10

#endif /* _LINUX_CISREG_H */
