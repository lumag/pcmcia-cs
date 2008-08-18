/*======================================================================

    This utility checks to see if any of a list of hosts are routed
    through a specified interface.  Hosts may be specified either by
    IP address or by name.
    
    usage: ifuser [-v] [interface] [host ...]

    The exit code is 0 if any host is using the specified interface,
    and 1 if the interface is not in use (just like fuser).
    
    ifuser.c 1.2 1998/05/10 12:12:59

    The contents of this file are subject to the Mozilla Public
    License Version 1.0 (the "License"); you may not use this file
    except in compliance with the License. You may obtain a copy of
    the License at http://www.mozilla.org/MPL/

    Software distributed under the License is distributed on an "AS
    IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
    implied. See the License for the specific language governing
    rights and limitations under the License.

    The initial developer of the original code is David A. Hinds
    <dhinds@hyper.stanford.edu>.  Portions created by David A. Hinds
    are Copyright (C) 1998 David A. Hinds.  All Rights Reserved.

======================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <ctype.h>
#include <netinet/in.h>

typedef struct route_t {
    u_int dest, mask;
    struct route_t *next;
} route_t;

/*====================================================================*/

static u_int pack_addr(char *s)
{
    int a, b, c, d;
    if (sscanf(s, "%d.%d.%d.%d", &a, &b, &c, &d) == 4)
	return ((a<<24) + (b<<16) + (c<<8) + d);
    else
	return 0;
}

int main(int argc, char *argv[])
{
    char *dev, s[129], d[16], m[16];
    route_t *r, *tbl = NULL;
    int i, verbose = 0, busy = 0;
    FILE *f;

    i = 1;
    if (argc < 2) return -1;
    if (strcmp(argv[1], "-v") == 0) {
	verbose = 1; i++;
    }
    if (argc < i+1) return -1;
    dev = argv[i]; i++;
    
    /* Get routing table */
    f = popen("netstat -nr", "r");
    if (f == NULL) {
	fprintf(stderr, "%s: could not get routing table: %s\n",
		argv[0], strerror(errno));
	return 2;
    }
    fgets(s, 128, f);
    while (fgets(s, 128, f) != NULL) {
	if (strstr(s, dev) == NULL)
	    continue;
	r = malloc(sizeof(route_t));
	sscanf(s, "%s %*s %s", d, m);
	r->dest = pack_addr(d);
	r->mask = pack_addr(m);
	r->next = tbl; tbl = r;
    }
    pclose(f);

    /* Check each host on command line */
    for (; i < argc; i++) {
	u_int a;
	if (isdigit(*(argv[i]))) {
	    a = pack_addr(argv[i]);
	    if (a == 0) {
		fprintf(stderr, "%s: bad address: %s\n",
			argv[0], argv[i]);
		continue;
	    }
	} else {
	    struct hostent *host = gethostbyname(argv[i]);
	    if (host == NULL) {
		fprintf(stderr, "%s: lookup failed: %s\n",
			argv[0], argv[i]);
		continue;
	    }
	    a = ntohl(*(u_int *)host->h_addr_list[0]);
	}
	for (r = tbl; r; r = r->next) {
	    if ((a & r->mask) == r->dest) {
		if (verbose) {
		    if (!busy) printf("%s:", dev);
		    printf(" %s", argv[i]);
		}
		busy = 1;
	    }
	}
    }
    
    if (busy && verbose)
	printf("\n");
    return (!busy);
}
