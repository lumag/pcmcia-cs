%{
/*
 * yacc_cis.y 1.2 1998/05/10 12:17:01
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>

#include "pack_cis.h"

extern int current_lineno;
%}

%token STRING NUMBER FLOAT
%token VERS_1 MANFID FUNCID CONFIG CFTABLE MFC
%token POST ROM
%token VNOM VMIN VMAX ISTATIC IAVG IMAX IDOWN
%token VCC VPP1 VPP2 IO MEM
%token DEFAULT BVD WP RDYBSY MWAIT AUDIO READONLY PWRDOWN
%token BIT8 BIT16
%token IRQ MASK LEVEL PULSE SHARED

%union {
    char *str;
    u_long num;
    float flt;
    cistpl_power_t pwr;
    cisparse_t *parse;
    tuple_info_t *tuple;
}

%type <str> STRING
%type <num> NUMBER
%type <flt> FLOAT
%type <pwr> pwr pwrlist
%type <parse> vers_1 manfid funcid config cftab io irq
%type <tuple> tuple chain cis;
%%

cis:	  chain
		{ cis_root = $1; }
	| chain mfc
		{ cis_root = $1; }
	;

chain:	  /* nothing */
		{ $$ = NULL; }
	| chain tuple
		{
		    if ($1 == NULL)
			$$ = $2;
		    else {
			tuple_info_t *tail = $1;
			while (tail->next != NULL) tail = tail->next;
			tail->next = $2;
			$$ = $1;
		    }
		} 
	;

mfc:	  MFC '{' chain '}'
		{ mfc[nf++] = $3; }
	| mfc ',' '{' chain '}'
		{ mfc[nf++] = $4; }
	;
	
tuple:	  vers_1
		{ $$ = new_tuple(CISTPL_VERS_1, $1); }
	| manfid
		{ $$ = new_tuple(CISTPL_MANFID, $1); }
	| funcid
		{ $$ = new_tuple(CISTPL_FUNCID, $1); }
	| config
		{ $$ = new_tuple(CISTPL_CONFIG, $1); }
	| cftab
		{ $$ = new_tuple(CISTPL_CFTABLE_ENTRY, $1); }
	;

vers_1:	  VERS_1 FLOAT
		{
		    $$ = calloc(1, sizeof(cisparse_t));
		    $$->version_1.major = $2;
		    $2 -= floor($2+0.01);
		    while (fabs($2 - floor($2+0.5)) > 0.01) {
			$2 *= 10;
		    }
		    $$->version_1.minor = $2+0.01;
		}
	| vers_1 ',' STRING
		{
		    u_int pos = strlen($$->version_1.str);
		    if (pos > 0) pos++;
		    $$->version_1.ofs[$$->version_1.ns] = pos;
		    strcpy($$->version_1.str+pos, $3);
		    $$->version_1.ns++;
		}
	;

manfid:	  MANFID NUMBER ',' NUMBER
		{
		    $$ = calloc(1, sizeof(cisparse_t));
		    $$->manfid.manf = $2;
		    $$->manfid.card = $4;
		}
	;

funcid:	  FUNCID NUMBER 
		{
		    $$ = calloc(1, sizeof(cisparse_t));
		    $$->funcid.func = $2;
		}
	| funcid POST
		{ $$->funcid.sysinit |= CISTPL_SYSINIT_POST; }
	| funcid ROM
		{ $$->funcid.sysinit |= CISTPL_SYSINIT_ROM; }
	;

config:	  CONFIG NUMBER ',' NUMBER ',' NUMBER
		{
		    $$ = calloc(1, sizeof(cisparse_t));
		    $$->config.last_idx = $2;
		    $$->config.base = $4;
		    $$->config.rmask[0] = $6;
		}
	;

pwr:	  VNOM FLOAT
		{
		    $$.present |= 1<<CISTPL_POWER_VNOM;
		    $$.param[CISTPL_POWER_VNOM] = $2 * 100000;
		}
	| VMIN FLOAT
		{
		    $$.present |= 1<<CISTPL_POWER_VMIN;
		    $$.param[CISTPL_POWER_VMIN] = $2 * 100000;
		}
	| VMAX FLOAT
		{
		    $$.present |= 1<<CISTPL_POWER_VMAX;
		    $$.param[CISTPL_POWER_VMAX] = $2 * 100000;
		}
	;

pwrlist:  /* nothing */
		{
		    $$.present = 0;
		}
	| pwrlist pwr
	;

io:	  cftab IO NUMBER '-' NUMBER
		{
		    int n = $$->cftable_entry.io.nwin;
		    $$->cftable_entry.io.win[n].base = $3;
		    $$->cftable_entry.io.win[n].len = $5-$3+1;
		    $$->cftable_entry.io.nwin++;
		}
	| io ',' NUMBER '-' NUMBER
		{
		    int n = $$->cftable_entry.io.nwin;
		    $$->cftable_entry.io.win[n].base = $3;
		    $$->cftable_entry.io.win[n].len = $5-$3+1;
		    $$->cftable_entry.io.nwin++;
		}
	| io BIT8
		{ $$->cftable_entry.io.flags |= CISTPL_IO_8BIT; }
	| io BIT16
		{ $$->cftable_entry.io.flags |= CISTPL_IO_16BIT; }
	;	

irq:	  cftab IRQ NUMBER
		{ $$->cftable_entry.irq.IRQInfo1 = ($3 & 0x0f); }
	| cftab IRQ MASK NUMBER
		{
		    $$->cftable_entry.irq.IRQInfo1 = IRQ_INFO2_VALID;
		    $$->cftable_entry.irq.IRQInfo2 = $4;
		}
	| irq PULSE
		{ $$->cftable_entry.irq.IRQInfo1 |= IRQ_PULSE_ID; }
	| irq LEVEL
		{ $$->cftable_entry.irq.IRQInfo1 |= IRQ_LEVEL_ID; }
	| irq SHARED
		{ $$->cftable_entry.irq.IRQInfo1 |= IRQ_SHARE_ID; }
	;

cftab:	  CFTABLE NUMBER
		{
		    $$ = calloc(1, sizeof(cisparse_t));
		    $$->cftable_entry.index = $2;
		}
	| cftab DEFAULT
		{ $$->cftable_entry.flags |= CISTPL_CFTABLE_DEFAULT; }
	| cftab BVD
		{ $$->cftable_entry.flags |= CISTPL_CFTABLE_BVDS; }
	| cftab WP
		{ $$->cftable_entry.flags |= CISTPL_CFTABLE_WP; }
	| cftab RDYBSY
		{ $$->cftable_entry.flags |= CISTPL_CFTABLE_RDYBSY; }
	| cftab MWAIT
		{ $$->cftable_entry.flags |= CISTPL_CFTABLE_MWAIT; }
	| cftab AUDIO
		{ $$->cftable_entry.flags |= CISTPL_CFTABLE_AUDIO; }
	| cftab READONLY
		{ $$->cftable_entry.flags |= CISTPL_CFTABLE_READONLY; }
	| cftab PWRDOWN
		{ $$->cftable_entry.flags |= CISTPL_CFTABLE_PWRDOWN; }
	| cftab VCC pwrlist
		{ $$->cftable_entry.vcc = $3; }
	| cftab VPP1 pwrlist
		{ $$->cftable_entry.vpp1 = $3; }
	| cftab VPP2 pwrlist
		{ $$->cftable_entry.vpp2 = $3; }
	| io
	| irq
	;

%%

tuple_info_t *new_tuple(u_char type, cisparse_t *parse)
{
    tuple_info_t *t = calloc(1, sizeof(tuple_info_t));
    t->type = type;
    t->parse = parse;
    t->next = NULL;
}

void yyerror(char *msg, ...)
{
    va_list ap;
    char str[256];

    va_start(ap, msg);
    sprintf(str, "error at line %d: ", current_lineno);
    vsprintf(str+strlen(str), msg, ap);
    fprintf(stderr, "%s\n", str);
    va_end(ap);
}

#ifdef DEBUG
void main(int argc, char *argv[])
{
    if (argc > 1)
	parse_cis(argv[1]);
}
#endif
