#ifndef _PCMCIA_UACCESS_H
#define _PCMCIA_UACCESS_H

#include <linux/version.h>
#ifndef VERSION
#define VERSION(a,b,c) (((a)<<16) + ((b)<<8) + (c))
#endif

#if (LINUX_VERSION_CODE < VERSION(2,1,0))
#define copy_from_user		memcpy_fromfs
#define copy_to_user		memcpy_tofs

#if (!defined(__alpha__) || (LINUX_VERSION_CODE < VERSION(2,0,34)))
#define ioremap(a,b) \
    (((a) < 0x100000) ? (void *)((u_long)(a)) : vremap(a,b))
#define iounmap(v) \
    do { if ((u_long)(v) > 0x100000) vfree(v); } while (0)
#endif
/* This is evil... throw away the built-in get_user in 2.0 */
#include <asm/segment.h>
#undef get_user

#ifdef __alpha__
#define get_user(x, ptr) 	((x) = __get_user((ptr), sizeof(*(ptr))))
#undef get_fs_long
#undef put_fs_long
#define get_fs_long(ptr)	__get_user((ptr), sizeof(int))
#define put_fs_long(x, ptr)	__put_user((x), (ptr), sizeof(int))
#else
#define get_user(x, ptr) \
		((sizeof(*ptr) == 4) ? (x = get_fs_long(ptr)) : \
		 (sizeof(*ptr) == 2) ? (x = get_fs_word(ptr)) : \
		 (x = get_fs_byte(ptr)))
#endif

#else /* 2.1.X */
#include_next <asm/uaccess.h>
#endif

#endif /* _PCMCIA_UACCESS_H */
