/*
 * rsrc_mgr.h 1.15 1999/06/18 17:48:15
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

#if defined(__LINUX__) && !defined(HAVE_MEMRESERVE)
int check_mem_region(u_long base, u_long num);
void request_mem_region(u_long base, u_long num, char *name);
void release_mem_region(u_long base, u_long num);
#endif

#ifdef __BEOS__
int check_resource(int type, u_long base, u_long num);
int register_resource(int type, u_long base, u_long num);
int release_resource(int type, u_long base, u_long num);
#endif

#endif	/* _RSRC_MGR_H */
