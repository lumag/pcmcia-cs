/*======================================================================

    A utility to convert a plain text description of a Card
    Information Structure into its packed binary representation.

    pack_cis.c 1.5 1998/05/10 12:17:39

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

    Usage:

    pack_cis [-o outfile] [infile]

    [infile] defaults to stdin, and [outfile] defaults to stdout.
    
======================================================================*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>

#include "pack_cis.h"

tuple_info_t *cis_root = NULL, *mfc[8] = { NULL };
int nf = 0;

/*======================================================================

    Support routines for packing parts of configuration table entries
    
======================================================================*/

static u_int mantissa[] = {
    10, 12, 13, 15, 20, 25, 30, 35,
    40, 45, 50, 55, 60, 70, 80, 90
};
static int pack_power(cistpl_power_t *pwr, u_char *b)
{
    u_int tmp, i;
    u_char m, e, x, *c = b;
    *c = pwr->present; c++;
    for (i = 0; i < 7; i++) {
	tmp = pwr->param[i];
	for (e = 0; ((tmp % 10) == 0) || (tmp > 999); e++)
	    tmp /= 10;
	if (tmp < 100) {
	    if (tmp < 10) { tmp *= 10; e--; }
	    for (m = 0; m < 16; m++)
		if (mantissa[m] == tmp) break;
	    if (m == 16) { tmp *= 10; e--; }
	    x = 0;
	}
	if (tmp >= 100) {
	    x = (tmp/10) - ((tmp/10) % 10);
	    for (m = 0; m < 16; m++)
		if (mantissa[m] == x) break;
	    x = (u_char)(tmp - 10*(u_int)x);
	}
	*c = (m<<3) | e | (x ? 0x80 : 0); c++;
	if (x) { *c = x; c++; }
    }
    return c-b;
}

static int pack_io(cistpl_io_t *p, u_char *b)
{
    u_char *c = b;
    u_int i, j, ml, ma;
    *c = 0;
    *c |= (p->flags & CISTPL_IO_8BIT) ? 0x20 : 0;
    *c |= (p->flags & CISTPL_IO_16BIT) ? 0x40 : 0;
    if ((p->nwin == 1) && (p->win[0].base == 0)) {
	for (i = 1, j = 0; i < p->win[0].len; i *= 2, j++) ;
	*c |= j; c++;
    } else {
	for (i = ma = ml = 0; i < p->nwin; i++) {
	    ma |= p->win[i].base;
	    ml |= p->win[i].len-1;
	}
	ma = (ma > 0xffff) ? 3 : ((ma > 0xff) ? 2 : 1);
	ml = (ml > 0xffff) ? 3 : ((ml > 0xff) ? 2 : 1);
	*c |= 0x80; c++;
	*c = (p->nwin-1) | (ma<<4) | (ml<<6); c++;
	if (ma == 3) ma++; if (ml == 3) ml++;
	for (i = 0; i < p->nwin; i++) {
	    for (j = 0; j < ma; j++) {
		*c = (p->win[i].base >> (8*j)) & 0xff; c++;
	    }
	    for (j = 0; j < ml; j++) {
		*c = ((p->win[i].len-1) >> (8*j)) & 0xff; c++;
	    }
	}
    }
    return c-b;
}

static int pack_irq(cistpl_irq_t *p, u_char *b)
{
    b[0] = p->IRQInfo1;
    if (p->IRQInfo1 & IRQ_INFO2_VALID) {
	b[1] = p->IRQInfo2 & 0xff;
	b[2] = (p->IRQInfo2 >> 8) & 0xff;
	return 3;
    }
    return 1;
}

static void pack_cftable(cistpl_cftable_entry_t *p, u_char *b)
{
    u_char *c;
    b[0] = CISTPL_CFTABLE_ENTRY;
    b[2] = p->index | 0x80;
    if (p->flags & CISTPL_CFTABLE_DEFAULT)
	b[2] |= 0x40;
    b[3] = 0x01;
    b[3] |= (p->flags & CISTPL_CFTABLE_BVDS) ? 0x10 : 0;
    b[3] |= (p->flags & CISTPL_CFTABLE_WP) ? 0x20 : 0;
    b[3] |= (p->flags & CISTPL_CFTABLE_RDYBSY) ? 0x40 : 0;
    b[3] |= (p->flags & CISTPL_CFTABLE_MWAIT) ? 0x80 : 0;
    b[4] = 0;
    c = b+5;
    if (p->vcc.present) {
	b[4]++; c += pack_power(&p->vcc, c);
	if (p->vpp1.present) {
	    b[4]++; c += pack_power(&p->vpp1, c);
	    if (p->vpp2.present) {
		b[4]++; c += pack_power(&p->vpp2, c);
	    }
	}
    }
    if (p->io.nwin > 0) {
	b[4] |= 0x08;
	c += pack_io(&p->io, c);
    }
    if (p->irq.IRQInfo1 > 0) {
	b[4] |= 0x10;
	c += pack_irq(&p->irq, c);
    }
    b[1] = c-b-2;
}

/*======================================================================

    For now, I only implement a subset of tuples types, intended to be
    enough to handle most IO-oriented cards.
    
======================================================================*/

static int pack_tuple(tuple_info_t *t, u_char *b)
{
    cisparse_t *p = t->parse;
    u_int i, m;
    u_char *c;

    *b = t->type;
    switch (t->type) {
    case CISTPL_DEVICE:
	/* Fake null device tuple */
	b[1] = 3; b[2] = 0; b[3] = 0; b[4] = 0xff;
	break;
    case CISTPL_MANFID:
	b[1] = 4;
	b[2] = p->manfid.manf & 0xff;
	b[3] = p->manfid.manf >> 8;
	b[4] = p->manfid.card & 0xff;
	b[5] = p->manfid.card >> 8;
	break;
    case CISTPL_FUNCID:
	b[1] = 2;
	b[2] = p->funcid.func;
	b[3] = p->funcid.sysinit;
	break;
    case CISTPL_CONFIG:
	b[3] = p->config.last_idx;
	i = p->config.base;
	for (c = b+4, m = 0; (i > 0) || !m; i >>= 8, m++) {
	    c[m] = i & 0xff;
	}
	b[2] = m-1;
	i = p->config.rmask[0];
	for (c = c+m, m = 0; (i > 0) || !m; i >>= 8, m++) {
	    c[m] = i & 0xff;
	}
	b[2] |= ((m-1) << 2);
	b[1] = c+m-b-2;
	break;
    case CISTPL_VERS_1:
	b[2] = p->version_1.major;
	b[3] = p->version_1.minor;
	c = b+4;
	for (i = 0; i < p->version_1.ns; i++) {
	    strcpy(c, p->version_1.str+p->version_1.ofs[i]);
	    c += strlen(c) + 1;
	}
	for (; i < 4; i++) { *c = 0; c++; }
	*c = 0xff; c++;
	b[1] = c-b-2;
	break;
    case CISTPL_CFTABLE_ENTRY:
	pack_cftable(&p->cftable_entry, b);
	break;
    case CISTPL_LINKTARGET:
	b[1] = 3; b[2] = 'C'; b[3] = 'I'; b[4] = 'S';
	break;
    case CISTPL_NO_LINK:
    case CISTPL_END:
	b[1] = 0;
	break;
    }
    return b[1]+2;
}

/*======================================================================

    The following routines handle parsing of aggregates of tuples.
    pack_chain() is the simplest: just return a string of tuples and
    terminate with an END tuple.  pack_mfc() is used to tie the
    function-specific tuple chains for a multifunction card together
    using a LONGLINK_MFC tuple.  And pack_cis() handles a complete
    CIS, whether it is multifunction or not.
    
======================================================================*/

static int pack_chain(tuple_info_t *t, u_char *b)
{
    int n = 0;
    tuple_info_t end = { CISTPL_END, NULL, NULL };
    while (t) {
	n += pack_tuple(t, b+n);
	t = t->next;
    }
    n += pack_tuple(&end, b+n);
    return n;
}

static int pack_mfc(u_int ofs, u_char *b)
{
    u_int i, j, pos;
    tuple_info_t target = { CISTPL_LINKTARGET, NULL, NULL };
    
    b[0] = CISTPL_LONGLINK_MFC;
    b[1] = 5*nf + 1;
    b[2] = nf;
    b[5*nf+3] = CISTPL_END;
    b[5*nf+4] = 0;
    /* Leave space for this tuple and the CISTPL_END tuple */
    pos = 5*nf+5;
    for (i = 0; i < nf; i++) {
	b[3+i*5] = 0;
	for (j = 0; j < 4; j++)
	    b[4+i*5+j] = ((ofs+pos) >> (8*j)) & 0xff;
	pos += pack_tuple(&target, b+pos);
	pos += pack_chain(mfc[i], b+pos);
    }
    return ofs+pos;
}

static int pack_cis(tuple_info_t *t, u_char *b)
{
    int n = 0;
    tuple_info_t device = { CISTPL_DEVICE, NULL, NULL };
    tuple_info_t nolink = { CISTPL_NO_LINK, NULL, NULL };
    tuple_info_t end = { CISTPL_END, NULL, NULL };
    n = pack_tuple(&device, b);
    while (t) {
	n += pack_tuple(t, b+n);
	t = t->next;
    }
    if (nf > 0) {
	n = pack_mfc(n, b+n);
    } else {
	n += pack_tuple(&nolink, b+n);
	n += pack_tuple(&end, b+n);
    }
     return n;
}

/*====================================================================*/

int main(int argc, char *argv[])
{
    int optch, errflg = 0;
    char *out = NULL;
    extern char *optarg;
    char buf[1024];
    int n;
    FILE *f;

    while ((optch = getopt(argc, argv, "vo:")) != -1) {
	switch (optch) {
	case 'v':
	    break;
	case 'o':
	    out = strdup(optarg); break;
	default:
	    errflg = 1; break;
	}
    }
    if (errflg || (optind < argc-1)) {
	fprintf(stderr, "usage: %s [-v] [-o outfile] [infile]",
		argv[0]);
	exit(EXIT_FAILURE);
    }
    if (optind < argc) {
	f = fopen(argv[optind], "r");
	if (!f) {
	    fprintf(stderr, "could not open '%s': %s\n", argv[optind],
		    strerror(errno));
	    return -1;
	}
    } else
	f = stdin;
    parse_cis(f);
    fclose(f);
    n = pack_cis(cis_root, buf);
    if (out) {
	f = fopen(out, "w");
	if (!f) {
	    fprintf(stderr, "could not open '%s': %s\n", out,
		    strerror(errno));
	    return -1;
	}
    } else f = stdout;
    fwrite(buf, n, 1, f);
    fclose(f);
    
    return 0;
}
