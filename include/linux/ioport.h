#ifndef _COMPAT_IOPORT_H
#define _COMPAT_IOPORT_H

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0))

#define check_mem_region	__check_mem_region
#define request_mem_region	__request_mem_region
#define release_mem_region	__release_mem_region

#include_next <linux/ioport.h>

#undef check_mem_region
#undef request_mem_region
#undef release_mem_region

#else

#include_next <linux/ioport.h>

#endif

#if defined(check_mem_region) && !defined(HAVE_MEMRESERVE)
#define HAVE_MEMRESERVE
#endif

#ifndef HAVE_MEMRESERVE
#define vacate_region		release_region
#define vacate_mem_region	release_mem_region
extern int check_mem_region(unsigned long, unsigned long);
extern void request_mem_region(unsigned long, unsigned long, char *);
extern void release_mem_region(unsigned long, unsigned long);
#endif

#endif /* _COMPAT_IOPORT_H */
