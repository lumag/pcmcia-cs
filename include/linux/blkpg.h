#ifndef _COMPAT_BLKPG_H
#define _COMPAT_BLKPG_H

#include <linux/version.h>

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,3,3))
#include_next <linux/blkpg.h>
#endif

#endif /* _COMPAT_BLKPG_H */
