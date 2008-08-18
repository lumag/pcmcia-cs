/*
 * ss.h 1.9 1997/12/18 07:37:47 (David Hinds)
 */

#ifndef _LINUX_SS_H
#define _LINUX_SS_H

/* For RegisterCallback */
typedef struct ss_callback_t {
    void	(*handler)(void *info, u_int events);
    void	*info;
} ss_callback_t;

/* Definitions for card status flags for GetStatus */
#define SS_WRPROT	0x0001
#define SS_CARDLOCK	0x0002
#define SS_EJECTION	0x0004
#define SS_INSERTION	0x0008
#define SS_BATDEAD	0x0010
#define SS_BATWARN	0x0020
#define SS_READY	0x0040
#define SS_DETECT	0x0080
#define SS_POWERON	0x0100
#define SS_GPI		0x0200
#define SS_STSCHG	0x0400
#define SS_CARDBUS	0x0800

/* for InquireSocket */
typedef struct socket_cap_t {
    u_int	irq_mask;
    u_char	cardbus;
} socket_cap_t;

/* for GetSocket, SetSocket */
typedef struct socket_state_t {
    u_int	flags;
    u_int	csc_mask;
    u_char	Vcc, Vpp;
    int		io_irq;
} socket_state_t;

/* Various card configuration flags */
#define SS_PWR_AUTO	0x0010
#define SS_IOCARD	0x0020
#define SS_RESET	0x0040
#define SS_DMA_MODE	0x0080
#define SS_SPKR_ENA	0x0100
#define SS_OUTPUT_ENA	0x0200

/* Flags for I/O port and memory windows */
#define MAP_ACTIVE	0x01
#define MAP_16BIT	0x02
#define MAP_AUTOSZ	0x04
#define MAP_0WS		0x08
#define MAP_WRPROT	0x10
#define MAP_ATTRIB	0x20
#define MAP_USE_WAIT	0x40
#define MAP_PREFETCH	0x80

/* Use this just for bridge windows */
#define MAP_IOSPACE	0x20

typedef struct pcmcia_io_map {
    u_char	map;
    u_char	flags;
    u_short	speed;
    u_short	start, stop;
} pcmcia_io_map;

typedef struct pcmcia_mem_map {
    u_char	map;
    u_char	flags;
    u_short	speed;
    u_long	sys_start, sys_stop;
    u_int	card_start;
} pcmcia_mem_map;

typedef struct cb_bridge_map {
    u_char	map;
    u_char	flags;
    u_int	start, stop;
} cb_bridge_map;

enum ss_service {
    SS_RegisterCallback, SS_InquireSocket,
    SS_GetStatus, SS_GetSocket, SS_SetSocket,
    SS_GetIOMap, SS_SetIOMap,
    SS_GetMemMap, SS_SetMemMap,
    SS_GetBridge, SS_SetBridge
};

#endif /* _LINUX_SS_H */
