#ifndef _COMPAT_SCHED_H
#define _COMPAT_SCHED_H

#include <linux/version.h>
#include_next <linux/sched.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,1,0))
#define __set_current_state(n) \
    do { current->state = TASK_INTERRUPTIBLE; } while (0)
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(2,2,18))
#ifndef __set_current_state
#define __set_current_state(n)	do { current->state = (n); } while (0)
#endif
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,1,68))
#define signal_pending(cur)	((cur)->signal & ~(cur)->blocked)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,1,127))
#define interruptible_sleep_on_timeout(w,t) \
    ({(current->timeout=jiffies+(t)); \
      interruptible_sleep_on(w);current->timeout;})
#define schedule_timeout(t) \
    do { current->timeout = jiffies+(t); schedule(); } while (0)
#endif

#ifndef CAP_SYS_ADMIN
#define capable(x)		(suser())
#endif

#endif /* _COMPAT_SCHED_H */
