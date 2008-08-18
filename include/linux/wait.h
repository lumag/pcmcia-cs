#ifndef _COMPAT_WAIT_H
#define _COMPAT_WAIT_H

#include <linux/version.h>
#include_next <linux/wait.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,2,18))
#ifndef init_waitqueue_head
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,0,16))
#define init_waitqueue_head(p)  (*(p) = NULL)
#else
#define init_waitqueue_head(p)  init_waitqueue(p)
#endif
typedef struct wait_queue *wait_queue_head_t;
#endif
#endif

#endif /* _COMPAT_WAIT_H */
