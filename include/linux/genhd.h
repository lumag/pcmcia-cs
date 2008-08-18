#ifndef _COMPAT_GENHD_H
#define _COMPAT_GENHD_H

#include <linux/version.h>
#include_next <linux/genhd.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,4,10))

static inline void add_gendisk(struct gendisk *gd)
{
    gd->next = gendisk_head;
    gendisk_head = gd;
}

static inline void del_gendisk(struct gendisk *gd)
{
    struct gendisk *od, **gdp;
    for (gdp = &gendisk_head; *gdp; gdp = &((*gdp)->next))
	if (*gdp == gd) {
	    od = *gdp; *gdp = od->next;
	    break;
	}
}

#endif

#endif /* _COMPAT_GENHD_H */
