/*
 * cs_types.h 1.7 1997/12/29 18:28:52 (David Hinds)
 */

#ifndef _LINUX_CS_TYPES_H
#define _LINUX_CS_TYPES_H

#include <linux/types.h>

typedef u_short socket_t;
typedef u_short ioaddr_t;
typedef u_int	event_t;
typedef u_char  cisdata_t;
typedef u_short page_t;

struct client_t;
typedef struct client_t *client_handle_t;

struct window_t;
typedef struct window_t *window_handle_t;

struct region_t;
typedef struct region_t *memory_handle_t;

struct eraseq_t;
typedef struct eraseq_t *eraseq_handle_t;

typedef char dev_info_t[16];

#endif /* _LINUX_CS_TYPES_H */
