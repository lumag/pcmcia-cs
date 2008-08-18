#ifndef _COMPAT_MODULE_H
#define _COMPAT_MODULE_H

#include <linux/version.h>
#include <linux/kernel.h>
#include_next <linux/module.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,1,18))
#define MODULE_PARM(a,b)	extern int __bogus_decl
#define MODULE_AUTHOR(a)	extern int __bogus_decl
#define MODULE_DESCRIPTION(a)	extern int __bogus_decl
#define MODULE_SUPPORTED_DEVICE(a) extern int __bogus_decl
#undef  GET_USE_COUNT
#define GET_USE_COUNT(m)	mod_use_count_
#endif

#ifndef SET_MODULE_OWNER
#define SET_MODULE_OWNER(m)	do { } while (0)
#endif

#ifndef MODULE_LICENSE
#define MODULE_LICENSE(m)	extern int __bogus_decl
#endif

#endif /* _COMPAT_MODULE_H */
