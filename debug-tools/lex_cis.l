/* Special state for handling include files */
%x src

%{
/*
 * lex_cis.l 1.4 1998/07/17 17:11:47
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

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include <pcmcia/cs_types.h>
#include <pcmcia/cistpl.h>

#include "pack_cis.h"
#include "yacc_cis.h"

/* For assembling nice error messages */
int current_lineno;

static int lex_number(char *s);
static int lex_float(char *s);
static int lex_string(char *s);

%}

int	[0-9]+
hex	0x[0-9a-fA-F]+
flt	[0-9]+\.[0-9]*
str	\"([^"]|\\.)*\"

%%

\n		current_lineno++;
[ \t]*		/* skip */ ;
[ ]*[#;].*	/* skip */ ;

mfc		return MFC;

vers_1		return VERS_1;
manfid		return MANFID;

funcid		return FUNCID;
\[post\]	return POST;
\[rom\]		return ROM;

config		return CONFIG;
cftable_entry	return CFTABLE;
\[default\]	return DEFAULT;
\[bvd\]		return BVD;
\[wp\]		return WP;
\[rdybsy\]	return RDYBSY;
\[mwait\]	return MWAIT;
\[audio\]	return AUDIO;
\[readonly\]	return READONLY;
\[pwrdown\]	return PWRDOWN;

Vnom		return VNOM;
Vmin		return VMIN;
Vmax		return VMAX;
Istatic		return ISTATIC;
Iavg		return IAVG;
Imax		return IMAX;
Idown		return IDOWN;

Vcc		return VCC;
Vpp1		return VPP1;
Vpp2		return VPP2;

io		return IO;
\[8bit\]	return BIT8;
\[16bit\]	return BIT16;

irq		return IRQ_NO;
mask		return MASK;
\[level\]	return LEVEL;
\[pulse\]	return PULSE;
\[shared\]	return SHARED;

memory_card	return lex_number("1");
serial_port	return lex_number("2");
parallel_port	return lex_number("3");
fixed_disk	return lex_number("4");
video_adapter	return lex_number("5");
network_adapter	return lex_number("6");
aims_card	return lex_number("7");
scsi_adapter	return lex_number("8");

{int}		return lex_number(yytext);

{hex}		return lex_number(yytext);

{flt}		return lex_float(yytext);

{str}		return lex_string(yytext);

.		return yytext[0];

%%

#ifndef yywrap
int yywrap() { return 1; }
#endif

/*======================================================================

    Stuff to parse basic data types

======================================================================*/

static int lex_number(char *s)
{
    yylval.num = strtoul(s, NULL, 0);
    return NUMBER;
}

static int lex_float(char *s)
{
    yylval.flt = strtod(s, NULL);
    return FLOAT;
}

static int lex_string(char *s)
{
    int n = strlen(s);
    yylval.str = malloc(n-1);
    strncpy(yylval.str, s+1, n-2);
    yylval.str[n-2] = '\0';
    return STRING;
}

/*======================================================================

    The main parser entry point

======================================================================*/

void parse_cis(FILE *f)
{
    current_lineno = 1;
    yyrestart(f);
    yyparse();
    fclose(f);
}

