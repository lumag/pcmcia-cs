/*
 * rsrc_mgr.h 1.11 1998/05/10 11:59:46
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

#ifndef _RSRC_MGR_H
#define _RSRC_MGR_H

#ifdef __KERNEL__

int check_mem_region(u_long base, u_long num);
int register_mem_region(u_long base, u_long num, char *name);
int release_mem_region(u_long base, u_long num);

void validate_mem(int (*is_valid)(u_long), int (*do_cksum)(u_long));

int find_io_region(ioaddr_t *base, ioaddr_t num, char *name);
int find_mem_region(u_long *base, u_long num, char *name, int low);
int try_irq(u_int Attributes, int irq, int specific);
void undo_irq(u_int Attributes, int irq);

int adjust_resource_info(client_handle_t handle, adjust_t *adj);

void release_resource_db(void);

#endif /* __KERNEL__ */

#endif	/* _RSRC_MGR_H */
