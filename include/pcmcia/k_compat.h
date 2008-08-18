/*
 * k_compat.h 1.42 1998/02/12 00:01:20 (David Hinds)
 */

#ifndef _LINUX_K_COMPAT_H
#define _LINUX_K_COMPAT_H

#define VERSION(v,p,s)		(((v)<<16)+(p<<8)+s)

#if (LINUX_VERSION_CODE < VERSION(1,3,0))

#define RUN_AT(x) 		(x)
#define CONST
#define ALLOC_SKB(len)		alloc_skb(len, GFP_ATOMIC)
#define DEVICE(req)		((req)->dev)
#define GET_PACKET(dev, skb, count) \
		(skb)->len = (count); \
		BLOCK_INPUT((skb)->data, (count))
#undef GET_SCSI_INFO

#define readb(p)		(*(volatile u_char *)(p))
#define readw(p)		(*(volatile u_short *)(p))
#define readl(p)		(*(volatile u_int *)(p))
#define writeb(b, p)		(*(volatile u_char *)(p) = b)
#define writew(w, p)		(*(volatile u_short *)(p) = w)
#define writel(l, p)		(*(volatile u_int *)(p) = l)
#define memcpy_fromio(a, b, c)	memcpy((a), (void *)(b), (c))
#define memcpy_toio(a, b, c)	memcpy((void *)(a), (b), (c))

#else /* 1.3.0 */

#define RUN_AT(x)		(jiffies+(x))
#define CONST			const
#define ALLOC_SKB(len)		dev_alloc_skb(len+2)
#define DEVICE(req)		((req)->rq_dev)
#define GET_PACKET(dev, skb, count) \
		skb_reserve((skb), 2); \
		BLOCK_INPUT(skb_put((skb), (count)), (count)); \
		(skb)->protocol = eth_type_trans((skb), (dev))
#define GET_SCSI_INFO

#endif /* 1.3.0 */

#if (LINUX_VERSION_CODE >= VERSION(1,3,31))
#define GET_8390_HDR		1
#endif /* 1.3.31 */

#if (LINUX_VERSION_CODE < VERSION(1,3,36))
#define BLK_DEV_HDR		"drivers/block/blk.h"
#else /* 1.3.36 */
#define BLK_DEV_HDR		"linux/blk.h"
#endif /* 1.3.36 */

#if (LINUX_VERSION_CODE >= VERSION(1,3,44))
#define NEW_MULTICAST
#endif /* 1.3.44 */

#if (LINUX_VERSION_CODE >= VERSION(1,3,70))
#define FREE_IRQ(i,d)		free_irq(i, d)
#define REQUEST_IRQ(i,h,f,n,d)	request_irq(i,h,f,n,d)
#define IRQ(a,b,c)		(a,b,c)
#define DEV_ID			dev_id
#else
#define SA_SHIRQ		0
#define FREE_IRQ(i,d)		free_irq(i)
#define REQUEST_IRQ(i,h,f,n,d)	request_irq(i,h,f,n)
#define IRQ(a,b,c)		(a,c)
#define DEV_ID			irq2dev_map[irq]
#endif

#if (LINUX_VERSION_CODE < VERSION(2,0,16))
#define init_waitqueue(p)	(*(p) = NULL)
#endif

#if (LINUX_VERSION_CODE < VERSION(2,1,4)) && !defined(__alpha__)
#define FS_SIZE_T		int
#define U_FS_SIZE_T		int
#else
#if (LINUX_VERSION_CODE < VERSION(2,1,60))
#define FS_SIZE_T		long
#define U_FS_SIZE_T		unsigned long
#else
#define FS_SIZE_T		ssize_t
#define U_FS_SIZE_T		size_t
#endif
#endif

#if (LINUX_VERSION_CODE < VERSION(2,1,25))
#define net_device_stats	enet_statistics
#endif

#if (LINUX_VERSION_CODE < VERSION(2,1,31))
#define FS_RELEASE_T		void
#else
#define FS_RELEASE_T		int
#endif

#if (LINUX_VERSION_CODE < VERSION(2,1,38))
#define test_and_set_bit	set_bit
#endif

#if (LINUX_VERSION_CODE < VERSION(1,3,38))

#ifdef MODULE
#include <linux/module.h>
#if !defined(CONFIG_MODVERSIONS) && !defined(__NO_VERSION__)
char kernel_version[] = UTS_RELEASE;
#endif
#else
#define MOD_DEC_USE_COUNT
#define MOD_INC_USE_COUNT
#endif

#else /* 1.3.38 */

#if (LINUX_VERSION_CODE > VERSION(2,1,16))
#define AUTOCONF_INCLUDED
#define EXPORT_SYMTAB
#endif
#ifdef CONFIG_MODVERSIONS
#define MODVERSIONS 1
#if (LINUX_VERSION_CODE >= VERSION(1,3,40))
#include <linux/modversions.h>
#endif
#endif
#include <linux/module.h>

#endif /* 1.3.38 */

#if (LINUX_VERSION_CODE < VERSION(2,1,18))
#define MOD_USE_COUNT		mod_use_count_
#define MODULE_PARM(a,b)	extern int __bogus_decl
#else
#define MOD_USE_COUNT 		__this_module.usecount
#endif

#if (LINUX_VERSION_CODE < VERSION(2,1,0))
#define copy_from_user		memcpy_fromfs
#define copy_to_user		memcpy_tofs

#if (LINUX_VERSION_CODE < VERSION(1,3,0))
#define kdev_t			int
#define ioremap(a, b)		((char *)(a))
#define iounmap(a)
#define put_user(x, ptr) \
		((sizeof(*ptr) == 4) ? put_fs_long(x, ptr) : \
		 (sizeof(*ptr) == 2) ? put_fs_word(x, ptr) : \
		 put_fs_byte(x, ptr))
#else
#define ioremap			vremap
#define iounmap			vfree
/* This is evil... throw away the built-in get_user in 1.3, 2.0 */
#include <asm/segment.h>
#undef get_user
#endif

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
#include <asm/uaccess.h>
#endif

#if (LINUX_VERSION_CODE < VERSION(2,1,45))
#define F_INODE(file)		((file)->f_inode)
#else
#define F_INODE(file)		((file)->f_dentry->d_inode)
#endif

#if (LINUX_VERSION_CODE < VERSION(1,3,0))
#define INVALIDATE_INODES(r)	while (0)
#else
#if (LINUX_VERSION_CODE < VERSION(2,1,51))
#define INVALIDATE_INODES(r)	invalidate_inodes(r)
#else
#define INVALIDATE_INODES(r) \
		do { struct super_block *sb = get_super(r); \
		if (sb) invalidate_inodes(sb); } while (0)
#endif
#endif

#if (LINUX_VERSION_CODE < VERSION(2,1,60))
#define IRQ_MAP(irq, dev)	irq2dev_map[irq] = dev
#define FOPS(i,f,b,c,p)		(i,f,b,c)
#define FPOS			(file->f_pos)
#else
#define IRQ_MAP(irq, dev)	while (0)
#define FOPS(i,f,b,c,p)		(f,b,c,p)
#define FPOS			(*ppos)
#endif

#if (LINUX_VERSION_CODE < VERSION(2,1,68))
#define signal_pending(cur)	((cur)->signal & ~(cur)->blocked)
#endif

#if (LINUX_VERSION_CODE < VERSION(2,1,86))
#define DEV_KFREE_SKB(skb) dev_kfree_skb(skb, FREE_WRITE)
#else
#define DEV_KFREE_SKB(skb) dev_kfree_skb(skb)
#endif

#endif /* _LINUX_K_COMPAT_H */
