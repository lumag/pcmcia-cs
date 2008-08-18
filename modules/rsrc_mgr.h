/*
 *  rsrc_mgr.h 1.8 1997/12/29 17:37:07 (David Hinds)
 */

#ifndef _RSRC_MGR_H
#define _RSRC_MGR_H

#ifdef __KERNEL__

int check_mem_region(u_long base, u_long num);
int register_mem_region(u_long base, u_long num, char *name);
int release_mem_region(u_long base, u_long num);

void validate_mem(int (*is_valid)(u_long), int (*do_cksum)(u_long));

int find_io_region(ioaddr_t *base, ioaddr_t num, char *name);
int find_mem_region(u_long *base, u_long num, char *name);
int try_irq(u_int Attributes, int irq, int specific);
void undo_irq(u_int Attributes, int irq);

int adjust_resource_info(client_handle_t handle, adjust_t *adj);

void release_resource_db(void);

#endif /* __KERNEL__ */

#endif	/* _RSRC_MGR_H */
