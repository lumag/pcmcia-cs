#ifndef _COMPAT_TIMER_H
#define _COMPAT_TIMER_H

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,2,0))
#define timer_pending(a)	(((a)->prev) != NULL)
#define mod_timer(a, b)	\
    do { del_timer(a); (a)->expires = (b); add_timer(a); } while (0)
#endif

#include_next <linux/timer.h>

#endif /* _COMPAT_TIMER_H */
