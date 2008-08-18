#ifndef _COMPAT_THREADS_H
#define _COMPAT_THREADS_H

#include <linux/version.h>

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,4,0))
#include_next <linux/threads.h>
#endif

#endif /* _COMPAT_THREADS_H */
