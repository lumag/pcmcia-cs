#ifndef _COMPAT_IDE_H
#define _COMPAT_IDE_H

#define CONFIG_BLK_DEV_IDE

#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0))
#include_next <linux/ide.h>
#endif

#endif /* _COMPAT_IDE_H */
