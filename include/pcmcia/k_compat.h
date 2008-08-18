/*
 * k_compat.h 1.74 1999/01/07 03:46:29
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License
 * at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and
 * limitations under the License. 
 *
 * The initial developer of the original code is David A. Hinds
 * <dhinds@hyper.stanford.edu>.  Portions created by David A. Hinds
 * are Copyright (C) 1998 David A. Hinds.  All Rights Reserved.
 */

#ifndef _LINUX_K_COMPAT_H
#define _LINUX_K_COMPAT_H

#define __LINUX__
#define VERSION(v,p,s)		(((v)<<16)+(p<<8)+s)

#define RUN_AT(x)		(jiffies+(x))
#define CONST			const
#define ALLOC_SKB(len)		dev_alloc_skb(len+2)
#define DEVICE(req)		((req)->rq_dev)
#define GET_PACKET(dev, skb, count) \
		skb_reserve((skb), 2); \
		BLOCK_INPUT(skb_put((skb), (count)), (count)); \
		(skb)->protocol = eth_type_trans((skb), (dev))

#define BLK_DEV_HDR		"linux/blk.h"
#define NEW_MULTICAST

#define FREE_IRQ(i,d)		free_irq(i, d)
#define REQUEST_IRQ(i,h,f,n,d)	request_irq(i,h,f,n,d)
#define IRQ(a,b,c)		(a,b,c)
#define DEV_ID			dev_id

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

#if (LINUX_VERSION_CODE > VERSION(2,1,16))
#define AUTOCONF_INCLUDED
#define EXPORT_SYMTAB
#endif
#ifdef CONFIG_MODVERSIONS
#define MODVERSIONS 1
#include <linux/modversions.h>
#endif
#include <linux/module.h>

#if (LINUX_VERSION_CODE < VERSION(2,1,18))
#define MODULE_PARM(a,b)	extern int __bogus_decl
#undef  GET_USE_COUNT
#define GET_USE_COUNT(m)	mod_use_count_
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
#include <asm/uaccess.h>
#endif

#if (LINUX_VERSION_CODE < VERSION(2,1,45))
#define F_INODE(file)		((file)->f_inode)
#else
#define F_INODE(file)		((file)->f_dentry->d_inode)
#endif

#if (LINUX_VERSION_CODE < VERSION(2,1,51))
#define INVALIDATE_INODES(r)	invalidate_inodes(r)
#else
#define INVALIDATE_INODES(r) \
		do { struct super_block *sb = get_super(r); \
		if (sb) invalidate_inodes(sb); } while (0)
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
#define DEV_KFREE_SKB(skb)	dev_kfree_skb(skb, FREE_WRITE)
#else
#define DEV_KFREE_SKB(skb)	dev_kfree_skb(skb)
#endif

#if (LINUX_VERSION_CODE < VERSION(2,1,89))
#define POLL_WAIT(f, q, w)	poll_wait(q, w)
#else
#define POLL_WAIT(f, q, w)	poll_wait(f, q, w)
#endif

#include <asm/byteorder.h>
#ifndef le16_to_cpu
#define le16_to_cpu(x)		(x)
#define le32_to_cpu(x)		(x)
#define cpu_to_le16(x)		(x)
#define cpu_to_le32(x)		(x)
#endif

#if (LINUX_VERSION_CODE < VERSION(2,1,90))
#define spin_lock(l) do { } while (0)
#define spin_unlock(l) do { } while (0)
#define spin_lock_irqsave(l,f) do { save_flags(f); cli(); } while (0)
#define spin_unlock_irqrestore(l,f) do { restore_flags(f); } while (0)
#else
#include <asm/spinlock.h>
#endif

#if (LINUX_VERSION_CODE < VERSION(2,1,104))
#define mdelay(x) { int i; for (i=0;i<x;i++) udelay(1000); }
#endif

#define wacquire(w)		do { } while (0)
#define wrelease(w)		do { } while (0)
#define wsleep(w)		interruptible_sleep_on(w)
#define wakeup(w)		wake_up_interruptible(w)
#define wsleeptimeout(w,t)	interruptible_sleep_on_timeout(w,t)
#if (LINUX_VERSION_CODE < VERSION(2,1,127))
#define interruptible_sleep_on_timeout(w,t) \
    ({(current->timeout=jiffies+(t));wsleep(w);current->timeout;})
#define schedule_timeout(t) \
    do { current->timeout = jiffies+(t); schedule(); } while (0)
#endif

#include <asm/io.h>
#ifndef readw_ns
#ifdef __powerpc__
#define readw_ns(p)		ld_be16((volatile unsigned short *)(p))
#define readl_ns(p)		ld_be32((volatile unsigned *)(p))
#define writew_ns(v,p)		st_le16((volatile unsigned short *)(p),(v))
#define writel_ns(v,p)		st_le32((volatile unsigned *)(p),(v))
#define inw_ns(p)		in_be16((unsigned short *)((p)+_IO_BASE))
#define inl_ns(p)		in_be32((unsigned *)((p)+_IO_BASE))
#define outw_ns(v,p)		out_be16((unsigned short *)((p)+_IO_BASE),(v))
#define outl_ns(v,p)		out_be32((unsigned *)((p)+_IO_BASE),(v))
#else
#define readw_ns(p)		readw(p)
#define readl_ns(p)		readl(p)
#define writew_ns(v,p)		writew(v,p)
#define writel_ns(v,p)		writel(v,p)
#define inw_ns(p)		inw(p)
#define inl_ns(p)		inl(p)
#define outw_ns(v,p)		outw(v,p)
#define outl_ns(v,p)		outl(v,p)
#endif
#endif
#ifndef insw_ns
#define insw_ns(p,b,l)		insw(p,b,l)
#define insl_ns(p,b,l)		insl(p,b,l)
#define outsw_ns(p,b,l)		outsw(p,b,l)
#define outsl_ns(p,b,l)		outsl(p,b,l)
#endif

#if (LINUX_VERSION_CODE < VERSION(2,1,93))
#include <linux/bios32.h>
#endif
#include <linux/pci.h>
#ifndef PCI_FUNC
#define PCI_FUNC(devfn)		((devfn)&7)
#define PCI_SLOT(devfn)		((devfn)>>3)
#define PCI_DEVFN(dev,fn)	(((dev)<<3)|((fn)&7))
#endif

#if (LINUX_VERSION_CODE > VERSION(2,1,117))
#define NULL_FLUSH		NULL,
#else
#define NULL_FLUSH
#endif

#if (LINUX_VERSION_CODE < VERSION(2,1,126))
#define SCSI_DISK0_MAJOR	SCSI_DISK_MAJOR
#endif

typedef unsigned long k_time_t;
#define ACQUIRE_RESOURCE_LOCK do {} while (0)
#define RELEASE_RESOURCE_LOCK do {} while (0)

#endif /* _LINUX_K_COMPAT_H */
