#ifndef _COMPAT_SPINLOCK_H
#define _COMPAT_SPINLOCK_H

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,1,90))

#define spin_lock(l)		do { } while (0)
#define spin_unlock(l)		do { } while (0)
#define spin_lock_irqsave(l,f)	do { save_flags(f); cli(); } while (0)
#define spin_unlock_irqrestore(l,f) do { restore_flags(f); } while (0)
#define spin_lock_init(s)	do { } while (0)
#define spin_trylock(l)		(1)
typedef int spinlock_t;

#else

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,3,17))
#include_next <asm/spinlock.h>
#else
#include_next <linux/spinlock.h>
#endif

#endif

#if defined(CONFIG_SMP) || (LINUX_VERSION_CODE > KERNEL_VERSION(2,3,6)) || \
    (defined(__powerpc__) && (LINUX_VERSION_CODE > KERNEL_VERSION(2,2,12)))
#define USE_SPIN_LOCKS
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,2,12)) && \
    !defined(CONFIG_SMP) && defined(__alpha__)
#undef spin_trylock
#define spin_trylock(l)		(1)
#endif

#ifndef spin_is_locked
#define spin_is_locked(l)	(0)
#endif

#endif /* _COMPAT_SPINLOCK_H */
