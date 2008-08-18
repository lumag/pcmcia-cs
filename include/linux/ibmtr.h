#ifndef _COMPAT_IBMTR_H
#define _COMPAT_IBMTR_H

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,21))
#include <../drivers/net/tokenring/ibmtr.h>
#else
#include <../drivers/net/ibmtr.h>
#endif

#endif /* _COMPAT_IBMTR_H */
