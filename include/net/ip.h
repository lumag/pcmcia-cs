#ifndef _COMPAT_IP_H
#define _COMPAT_IP_H

#include <linux/version.h>

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,4,9))
#include <linux/kernel.h>
#undef min
#undef max
#endif

#include_next <net/ip.h>

#endif /* _COMPAT_IP_H */
