/*
 * Exit code = 0 if IPv6 is supported, 1 if not.
 *
 * has_ipv6.c 1.4 1998/05/10 12:12:59
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

#include <stdio.h>
#include <sys/socket.h>

int main(void)
{
#ifndef AF_INET6
    return 1;
#else
    int s = socket(AF_INET6, SOCK_STREAM, 0);
    if (s == -1)
	return 1;
    close(s);
    return 0;
#endif
}
