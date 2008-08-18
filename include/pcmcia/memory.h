/*
 *  memory.h 1.1 1997/12/29 18:27:55 (David Hinds)
 */

#ifndef _LINUX_MEMORY_H
#define _LINUX_MEMORY_H

typedef struct erase_info_t {
    u_long	Offset;
    u_long	Size;
} erase_info_t;

#define MEMGETINFO		_IOR('M', 1, region_info_t)
#define MEMERASE		_IOW('M', 2, erase_info_t)

#endif /* _LINUX_MEMORY_H */
