#ifndef PCMCIA_PAGE_H
#define PCMCIA_PAGE_H

#include_next <asm/page.h>

#ifndef BUG
#define BUG() \
    do { printk("kernel BUG at %s:%d!\n", __FILE__, __LINE__); \
	 *(int *)0=0; } while (0)
#endif

#endif /* PCMCIA_PAGE_H */
