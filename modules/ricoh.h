/* ricoh.h 1.1 1995/04/08 16:42:05 (David Hinds) */

#ifndef _LINUX_RICOH_H
#define _LINUX_RICOH_H

#define RF5C_MODE_CTL		0x1f	/* Mode control */
#define RF5C_PWR_CTL		0x2f	/* Mixed voltage control */
#define RF5C_CHIP_ID		0x3a	/* Chip identification */
#define RF5C_MODE_CTL_3		0x3b	/* Mode control 3 */

/* I/O window address offset */
#define RF5C_IO_OFF(w)		(0x36+((w)<<1))

/* Flags for RF5C_MODE_CTL */
#define RF5C_MODE_ATA		0x01	/* ATA mode */
#define RF5C_MODE_LED_ENA	0x02	/* IRQ 12 is LED */
#define RF5C_MODE_CA21		0x04
#define RF5C_MODE_CA22		0x08
#define RF5C_MODE_CA23		0x10
#define RF5C_MODE_CA24		0x20
#define RF5C_MODE_CA25		0x40
#define RF5C_MODE_3STATE_BIT7	0x80

/* Flags for RF5C_PWR_CTL */
#define RF5C_PWR_VCC_3V		0x01
#define RF5C_PWR_IREQ_HIGH	0x02
#define RF5C_PWR_INPACK_ENA	0x04
#define RF5C_PWR_5V_DET		0x08
#define RF5C_PWR_TC_SEL		0x10	/* Terminal Count: irq 11 or 15 */
#define RF5C_PWR_DREQ_LOW	0x20
#define RF5C_PWR_DREQ_OFF	0x00	/* DREQ steering control */
#define RF5C_PWR_DREQ_INPACK	0x40
#define RF5C_PWR_DREQ_SPKR	0x80
#define RF5C_PWR_DREQ_IOIS16	0xc0

/* Values for RF5C_CHIP_ID */
#define RF5C_CHIP_RF5C296	0x32
#define RF5C_CHIP_RF5C396	0xb2

/* Flags for RF5C_MODE_CTL_3 */
#define RF5C_MCTL3_DISABLE	0x01	/* Disable PCMCIA interface */
#define RF5C_MCTL3_DMA_ENA	0x02

#endif /* _LINUX_RICOH_H */
