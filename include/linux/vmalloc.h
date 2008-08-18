#ifndef _COMPAT_VMALLOC_H
#define _COMPAT_VMALLOC_H

#include <linux/version.h>

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,1,0))
#include_next <linux/vmalloc.h>
#endif

#endif /* _COMPAT_VMALLOC_H */
