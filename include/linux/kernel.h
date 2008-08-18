#ifndef _COMPAT_KERNEL_H
#define _COMPAT_KERNEL_H

#include_next <linux/kernel.h>

#undef min
#undef max
#define min(x,y) (((x)<(y)) ? (x) : (y))
#define max(x,y) (((x)>(y)) ? (y) : (x))

#ifndef min_t
#define min_t(type,x,y) \
	({ type __x = (x), __y = (y); __x < __y ? __x: __y; })
#define max_t(type,x,y) \
	({ type __x = (x), __y = (y); __x > __y ? __x: __y; })
#endif

#endif /* _COMPAT_KERNEL_H */
