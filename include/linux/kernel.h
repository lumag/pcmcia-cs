#ifndef _COMPAT_KERNEL_H
#define _COMPAT_KERNEL_H

#include <linux/version.h>

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,1,16))
#define AUTOCONF_INCLUDED
#define EXPORT_SYMTAB
#define register_symtab(x)
#endif
#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
#define MODVERSIONS		1
#include <linux/modversions.h>
#endif

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

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,2,0))
#define in_interrupt()		(intr_count)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,1,31))
#define FS_RELEASE_T		void
#else
#define FS_RELEASE_T		int
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,1,38))
#define test_and_set_bit	set_bit
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,1,45))
#define F_INODE(file)		((file)->f_inode)
#else
#define F_INODE(file)		((file)->f_dentry->d_inode)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,1,51))
#define INVALIDATE_INODES(r)	invalidate_inodes(r)
#else
#define INVALIDATE_INODES(r) \
	do { struct super_block *sb = get_super(r); \
	if (sb) invalidate_inodes(sb); } while (0)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,1,60))
#define FOPS(i,f,b,c,p)		(i,f,b,c)
#define FPOS			(file->f_pos)
#else
#define FOPS(i,f,b,c,p)		(f,b,c,p)
#define FPOS			(*ppos)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,1,89))
#define POLL_WAIT(f, q, w)	poll_wait(q, w)
#else
#define POLL_WAIT(f, q, w)	poll_wait(f, q, w)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,1,104))
#define mdelay(x) { int i; for (i=0;i<x;i++) udelay(1000); }
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,1,126))
#define SCSI_DISK0_MAJOR	SCSI_DISK_MAJOR
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,3,32))
#define BLK_DEFAULT_QUEUE(n)	blk_dev[n].request_fn
#define blk_init_queue(q, req)	do { (q) = (req); } while (0)
#define blk_cleanup_queue(q)	do { (q) = NULL; } while (0)
#define request_arg_t		void
#else
#define request_arg_t		request_queue_t *q
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,3,38))
#define block_device_operations file_operations
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,3,40))
#define register_disk(dev, drive, minors, ops, size) \
    do { (dev)->part[(drive)*(minors)].nr_sects = size; \
	if (size == 0) (dev)->part[(drive)*(minors)].start_sect = -1; \
	resetup_one_dev(dev, drive); } while (0);
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,3,13))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,4,3))
#define PREPARE_TQUEUE(_tq, _routine, _data)			\
	do {							\
		(_tq)->routine = _routine;			\
		(_tq)->data = _data;				\
	} while (0)
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,3,99))
#define INIT_TQUEUE(_tq, _routine, _data)			\
	do {							\
		INIT_LIST_HEAD(&(_tq)->list);			\
		(_tq)->sync = 0;				\
		PREPARE_TQUEUE((_tq), (_routine), (_data));	\
	} while (0)
#else
#define INIT_TQUEUE(_tq, _routine, _data)			\
	do {							\
		(_tq)->next = 0;				\
		(_tq)->sync = 0;				\
		PREPARE_TQUEUE((_tq), (_routine), (_data));	\
	} while (0)
#endif
#endif

#endif /* _COMPAT_KERNEL_H */
