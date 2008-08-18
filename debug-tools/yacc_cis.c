
/*  A Bison parser, made from yacc_cis.y
 by  GNU Bison version 1.25
  */

#define YYBISON 1  /* Identify Bison output.  */

#define	STRING	258
#define	NUMBER	259
#define	FLOAT	260
#define	VOLTAGE	261
#define	CURRENT	262
#define	SIZE	263
#define	VERS_1	264
#define	MANFID	265
#define	FUNCID	266
#define	CONFIG	267
#define	CFTABLE	268
#define	MFC	269
#define	CHECKSUM	270
#define	POST	271
#define	ROM	272
#define	BASE	273
#define	LAST_INDEX	274
#define	DEV_INFO	275
#define	ATTR_DEV_INFO	276
#define	NO_INFO	277
#define	TIME	278
#define	TIMING	279
#define	WAIT	280
#define	READY	281
#define	RESERVED	282
#define	VNOM	283
#define	VMIN	284
#define	VMAX	285
#define	ISTATIC	286
#define	IAVG	287
#define	IPEAK	288
#define	IDOWN	289
#define	VCC	290
#define	VPP1	291
#define	VPP2	292
#define	IO	293
#define	MEM	294
#define	DEFAULT	295
#define	BVD	296
#define	WP	297
#define	RDYBSY	298
#define	MWAIT	299
#define	AUDIO	300
#define	READONLY	301
#define	PWRDOWN	302
#define	BIT8	303
#define	BIT16	304
#define	LINES	305
#define	RANGE	306
#define	IRQ_NO	307
#define	MASK	308
#define	LEVEL	309
#define	PULSE	310
#define	SHARED	311

#line 1 "yacc_cis.y"

/*
 * yacc_cis.y 1.7 1999/02/20 06:35:21
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
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>

#include "pack_cis.h"

/* If bison: generate nicer error messages */ 
#define YYERROR_VERBOSE 1
 
extern int current_lineno;

void yyerror(char *msg, ...);
static tuple_info_t *new_tuple(u_char type, cisparse_t *parse);


#line 54 "yacc_cis.y"
typedef union {
    char *str;
    u_long num;
    float flt;
    cistpl_power_t pwr;
    cisparse_t *parse;
    tuple_info_t *tuple;
} YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		138
#define	YYFLAG		-32768
#define	YYNTBASE	64

#define YYTRANSLATE(x) ((unsigned)(x) <= 311 ? yytranslate[x] : 82)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,    59,    60,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
    61,     2,     2,    63,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    62,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    57,     2,    58,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
    26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
    46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
    56
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     2,     5,     6,     9,    14,    20,    22,    24,    26,
    28,    30,    32,    34,    36,    38,    40,    46,    49,    51,
    57,    60,    63,    67,    72,    75,    78,    81,    89,    92,
    95,    98,   101,   104,   107,   110,   111,   114,   117,   121,
   125,   129,   135,   141,   144,   147,   153,   156,   164,   172,
   175,   178,   182,   187,   190,   193,   196,   199,   202,   205,
   208,   211,   214,   217,   220,   223,   227,   231,   235,   237,
   239,   241,   243
};

static const short yyrhs[] = {    65,
     0,    65,    66,     0,     0,    65,    67,     0,    14,    57,
    65,    58,     0,    66,    59,    57,    65,    58,     0,    68,
     0,    69,     0,    70,     0,    71,     0,    72,     0,    73,
     0,    80,     0,    81,     0,     1,     0,    20,     0,    68,
     4,    23,    59,     8,     0,    68,    22,     0,    21,     0,
    69,     4,    23,    59,     8,     0,    69,    22,     0,     9,
     5,     0,    70,    59,     3,     0,    10,     4,    59,     4,
     0,    11,     4,     0,    72,    16,     0,    72,    17,     0,
    12,    18,     4,    53,     4,    19,     4,     0,    28,     6,
     0,    29,     6,     0,    30,     6,     0,    31,     7,     0,
    32,     7,     0,    33,     7,     0,    34,     7,     0,     0,
    75,    74,     0,    80,    24,     0,    76,    25,    23,     0,
    76,    26,    23,     0,    76,    27,    23,     0,    80,    38,
     4,    60,     4,     0,    77,    59,     4,    60,     4,     0,
    77,    48,     0,    77,    49,     0,    77,    50,    61,     4,
    62,     0,    77,    51,     0,    80,    39,     4,    60,     4,
    63,     4,     0,    78,    59,     4,    60,     4,    63,     4,
     0,    78,    48,     0,    78,    49,     0,    80,    52,     4,
     0,    80,    52,    53,     4,     0,    79,    55,     0,    79,
    54,     0,    79,    56,     0,    13,     4,     0,    80,    40,
     0,    80,    41,     0,    80,    42,     0,    80,    43,     0,
    80,    44,     0,    80,    45,     0,    80,    46,     0,    80,
    47,     0,    80,    35,    75,     0,    80,    36,    75,     0,
    80,    37,    75,     0,    77,     0,    78,     0,    79,     0,
    76,     0,    15,     4,    60,     4,    61,     4,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
    72,    74,    78,    80,    95,    97,   101,   103,   105,   107,
   109,   111,   113,   115,   117,   121,   123,   130,   133,   135,
   142,   145,   155,   169,   177,   182,   184,   188,   197,   202,
   207,   212,   217,   222,   227,   234,   238,   245,   246,   247,
   248,   251,   258,   265,   267,   269,   271,   274,   282,   290,
   292,   296,   298,   303,   305,   307,   311,   316,   318,   320,
   322,   324,   326,   328,   330,   332,   334,   336,   338,   339,
   340,   341,   344
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","STRING",
"NUMBER","FLOAT","VOLTAGE","CURRENT","SIZE","VERS_1","MANFID","FUNCID","CONFIG",
"CFTABLE","MFC","CHECKSUM","POST","ROM","BASE","LAST_INDEX","DEV_INFO","ATTR_DEV_INFO",
"NO_INFO","TIME","TIMING","WAIT","READY","RESERVED","VNOM","VMIN","VMAX","ISTATIC",
"IAVG","IPEAK","IDOWN","VCC","VPP1","VPP2","IO","MEM","DEFAULT","BVD","WP","RDYBSY",
"MWAIT","AUDIO","READONLY","PWRDOWN","BIT8","BIT16","LINES","RANGE","IRQ_NO",
"MASK","LEVEL","PULSE","SHARED","'{'","'}'","','","'-'","'='","']'","'@'","cis",
"chain","mfc","tuple","dev_info","attr_dev_info","vers_1","manfid","funcid",
"config","pwr","pwrlist","timing","io","mem","irq","cftab","checksum", NULL
};
#endif

static const short yyr1[] = {     0,
    64,    64,    65,    65,    66,    66,    67,    67,    67,    67,
    67,    67,    67,    67,    67,    68,    68,    68,    69,    69,
    69,    70,    70,    71,    72,    72,    72,    73,    74,    74,
    74,    74,    74,    74,    74,    75,    75,    76,    76,    76,
    76,    77,    77,    77,    77,    77,    77,    78,    78,    78,
    78,    79,    79,    79,    79,    79,    80,    80,    80,    80,
    80,    80,    80,    80,    80,    80,    80,    80,    80,    80,
    80,    80,    81
};

static const short yyr2[] = {     0,
     1,     2,     0,     2,     4,     5,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     5,     2,     1,     5,
     2,     2,     3,     4,     2,     2,     2,     7,     2,     2,
     2,     2,     2,     2,     2,     0,     2,     2,     3,     3,
     3,     5,     5,     2,     2,     5,     2,     7,     7,     2,
     2,     3,     4,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     3,     3,     3,     1,     1,
     1,     1,     6
};

static const short yydefact[] = {     3,
     0,    15,     0,     0,     0,     0,     0,     0,     0,    16,
    19,     2,     4,     7,     8,     9,    10,    11,    12,    72,
    69,    70,    71,    13,    14,    22,     0,    25,     0,    57,
     3,     0,     0,     0,    18,     0,    21,     0,    26,    27,
     0,     0,     0,    44,    45,     0,    47,     0,    50,    51,
     0,    55,    54,    56,    38,    36,    36,    36,     0,     0,
    58,    59,    60,    61,    62,    63,    64,    65,     0,     0,
     0,     0,     0,     3,     0,     0,    23,    39,    40,    41,
     0,     0,     0,    66,    67,    68,     0,     0,    52,     0,
    24,     0,     5,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,    37,     0,     0,
    53,     0,     0,     6,    17,    20,    46,    43,     0,    29,
    30,    31,    32,    33,    34,    35,    42,     0,     0,    73,
     0,     0,    28,    49,    48,     0,     0,     0
};

static const short yydefgoto[] = {   136,
     1,    12,    13,    14,    15,    16,    17,    18,    19,   108,
    84,    20,    21,    22,    23,    24,    25
};

static const short yypact[] = {-32768,
    28,-32768,    -2,    22,    40,    16,    43,   -41,    49,-32768,
-32768,   -13,-32768,    13,    14,     2,-32768,    42,-32768,    29,
   -44,     3,    30,    36,-32768,-32768,    31,-32768,    83,-32768,
-32768,    32,    34,    66,-32768,    70,-32768,    91,-32768,-32768,
    72,    73,    74,-32768,-32768,    37,-32768,    95,-32768,-32768,
    96,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    97,    98,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    -3,    99,
    51,    -1,   101,-32768,    47,    48,-32768,-32768,-32768,-32768,
   104,    50,    52,    35,    35,    35,    53,    54,-32768,   105,
-32768,   107,-32768,    55,    12,   109,   110,    57,   111,   116,
   115,   117,   118,   119,   120,   121,   122,-32768,   126,   127,
-32768,   103,   128,-32768,-32768,-32768,-32768,-32768,    62,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,    71,   129,-32768,
   131,   132,-32768,-32768,-32768,   137,   138,-32768
};

static const short yypgoto[] = {-32768,
   -29,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
   -27,-32768,-32768,-32768,-32768,-32768,-32768
};


#define	YYLAST		138


static const short yytable[] = {     2,
    89,    72,    26,    44,    45,    46,    47,     3,     4,     5,
     6,     7,     2,     9,    48,    31,    34,    36,    10,    11,
     3,     4,     5,     6,     7,    27,     9,    -1,     2,    85,
    86,    10,    11,    29,    35,    37,     3,     4,     5,     6,
     7,     8,     9,    28,    95,    33,    30,    10,    11,    90,
    49,    50,    32,    41,    42,    43,    93,    39,    40,    55,
    38,    51,   101,   102,   103,   104,   105,   106,   107,   114,
    56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
    66,    67,    68,    52,    53,    54,    71,    69,    75,    70,
    74,    73,    76,    77,    78,    79,    80,    81,    82,    83,
    87,    88,    91,    92,    94,    96,    97,    98,   111,    99,
   112,   100,   109,   110,   118,   113,   115,   116,   117,   119,
   120,   129,   121,   122,   131,   123,   124,   125,   126,   127,
   128,   130,   133,   132,   134,   135,   137,   138
};

static const short yycheck[] = {     1,
     4,    31,     5,    48,    49,    50,    51,     9,    10,    11,
    12,    13,     1,    15,    59,    57,     4,     4,    20,    21,
     9,    10,    11,    12,    13,     4,    15,     0,     1,    57,
    58,    20,    21,    18,    22,    22,     9,    10,    11,    12,
    13,    14,    15,     4,    74,    59,     4,    20,    21,    53,
    48,    49,     4,    25,    26,    27,    58,    16,    17,    24,
    59,    59,    28,    29,    30,    31,    32,    33,    34,    58,
    35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
    45,    46,    47,    54,    55,    56,     4,    52,    23,    59,
    57,    60,    23,     3,    23,    23,    23,    61,     4,     4,
     4,     4,     4,    53,     4,    59,    59,     4,     4,    60,
     4,    60,    60,    60,     4,    61,     8,     8,    62,     4,
     6,    19,     6,     6,    63,     7,     7,     7,     7,     4,
     4,     4,     4,    63,     4,     4,     0,     0
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/lib/bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#ifdef __cplusplus
extern "C" {
void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca ();
#endif /* not __cplusplus */
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
int yyparse (void);
#endif

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 196 "/usr/lib/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 1:
#line 73 "yacc_cis.y"
{ cis_root = yyvsp[0].tuple; ;
    break;}
case 2:
#line 75 "yacc_cis.y"
{ cis_root = yyvsp[-1].tuple; ;
    break;}
case 3:
#line 79 "yacc_cis.y"
{ yyval.tuple = NULL; ;
    break;}
case 4:
#line 81 "yacc_cis.y"
{
		    if (yyvsp[-1].tuple == NULL) {
			yyval.tuple = yyvsp[0].tuple;
		    } else if (yyvsp[0].tuple == NULL) {
			yyval.tuple = yyvsp[-1].tuple;
		    } else {
			tuple_info_t *tail = yyvsp[-1].tuple;
			while (tail->next != NULL) tail = tail->next;
			tail->next = yyvsp[0].tuple;
			yyval.tuple = yyvsp[-1].tuple;
		    }
		;
    break;}
case 5:
#line 96 "yacc_cis.y"
{ mfc[nf++] = yyvsp[-1].tuple; ;
    break;}
case 6:
#line 98 "yacc_cis.y"
{ mfc[nf++] = yyvsp[-1].tuple; ;
    break;}
case 7:
#line 102 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_DEVICE, yyvsp[0].parse); ;
    break;}
case 8:
#line 104 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_DEVICE_A, yyvsp[0].parse); ;
    break;}
case 9:
#line 106 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_VERS_1, yyvsp[0].parse); ;
    break;}
case 10:
#line 108 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_MANFID, yyvsp[0].parse); ;
    break;}
case 11:
#line 110 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_FUNCID, yyvsp[0].parse); ;
    break;}
case 12:
#line 112 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_CONFIG, yyvsp[0].parse); ;
    break;}
case 13:
#line 114 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_CFTABLE_ENTRY, yyvsp[0].parse); ;
    break;}
case 14:
#line 116 "yacc_cis.y"
{ yyval.tuple = NULL; ;
    break;}
case 15:
#line 118 "yacc_cis.y"
{ yyval.tuple = NULL; ;
    break;}
case 16:
#line 122 "yacc_cis.y"
{ yyval.parse = calloc(1, sizeof(cisparse_t)); ;
    break;}
case 17:
#line 124 "yacc_cis.y"
{
		    yyval.parse->device.dev[yyval.parse->device.ndev].type = yyvsp[-3].num;
		    yyval.parse->device.dev[yyval.parse->device.ndev].speed = yyvsp[-2].num;
		    yyval.parse->device.dev[yyval.parse->device.ndev].size = yyvsp[0].num;
		    yyval.parse->device.ndev++;
		;
    break;}
case 19:
#line 134 "yacc_cis.y"
{ yyval.parse = calloc(1, sizeof(cisparse_t)); ;
    break;}
case 20:
#line 136 "yacc_cis.y"
{
		    yyval.parse->device.dev[yyval.parse->device.ndev].type = yyvsp[-3].num;
		    yyval.parse->device.dev[yyval.parse->device.ndev].speed = yyvsp[-2].num;
		    yyval.parse->device.dev[yyval.parse->device.ndev].size = yyvsp[0].num;
		    yyval.parse->device.ndev++;
		;
    break;}
case 22:
#line 146 "yacc_cis.y"
{
		    yyval.parse = calloc(1, sizeof(cisparse_t));
		    yyval.parse->version_1.major = yyvsp[0].flt;
		    yyvsp[0].flt -= floor(yyvsp[0].flt+0.01);
		    while (fabs(yyvsp[0].flt - floor(yyvsp[0].flt+0.5)) > 0.01) {
			yyvsp[0].flt *= 10;
		    }
		    yyval.parse->version_1.minor = yyvsp[0].flt+0.01;
		;
    break;}
case 23:
#line 156 "yacc_cis.y"
{
		    cistpl_vers_1_t *v = &yyval.parse->version_1;
		    u_int pos = 0;
		    if (v->ns) {
			pos = v->ofs[v->ns-1];
			pos += strlen(v->str+pos)+1;
		    }
		    v->ofs[v->ns] = pos;
		    strcpy(v->str+pos, yyvsp[0].str);
		    v->ns++;
		;
    break;}
case 24:
#line 170 "yacc_cis.y"
{
		    yyval.parse = calloc(1, sizeof(cisparse_t));
		    yyval.parse->manfid.manf = yyvsp[-2].num;
		    yyval.parse->manfid.card = yyvsp[0].num;
		;
    break;}
case 25:
#line 178 "yacc_cis.y"
{
		    yyval.parse = calloc(1, sizeof(cisparse_t));
		    yyval.parse->funcid.func = yyvsp[0].num;
		;
    break;}
case 26:
#line 183 "yacc_cis.y"
{ yyval.parse->funcid.sysinit |= CISTPL_SYSINIT_POST; ;
    break;}
case 27:
#line 185 "yacc_cis.y"
{ yyval.parse->funcid.sysinit |= CISTPL_SYSINIT_ROM; ;
    break;}
case 28:
#line 189 "yacc_cis.y"
{
		    yyval.parse = calloc(1, sizeof(cisparse_t));
		    yyval.parse->config.base = yyvsp[-4].num;
		    yyval.parse->config.rmask[0] = yyvsp[-2].num;
		    yyval.parse->config.last_idx = yyvsp[0].num;
		;
    break;}
case 29:
#line 198 "yacc_cis.y"
{
		    yyval.pwr.present = CISTPL_POWER_VNOM;
		    yyval.pwr.param[0] = yyvsp[0].num;
		;
    break;}
case 30:
#line 203 "yacc_cis.y"
{
		    yyval.pwr.present = CISTPL_POWER_VMIN;
		    yyval.pwr.param[0] = yyvsp[0].num;
		;
    break;}
case 31:
#line 208 "yacc_cis.y"
{
		    yyval.pwr.present = CISTPL_POWER_VMAX;
		    yyval.pwr.param[0] = yyvsp[0].num;
		;
    break;}
case 32:
#line 213 "yacc_cis.y"
{
		    yyval.pwr.present = CISTPL_POWER_ISTATIC;
		    yyval.pwr.param[0] = yyvsp[0].num;
		;
    break;}
case 33:
#line 218 "yacc_cis.y"
{
		    yyval.pwr.present = CISTPL_POWER_IAVG;
		    yyval.pwr.param[0] = yyvsp[0].num;
		;
    break;}
case 34:
#line 223 "yacc_cis.y"
{
		    yyval.pwr.present = CISTPL_POWER_IPEAK;
		    yyval.pwr.param[0] = yyvsp[0].num;
		;
    break;}
case 35:
#line 228 "yacc_cis.y"
{
		    yyval.pwr.present = CISTPL_POWER_IDOWN;
		    yyval.pwr.param[0] = yyvsp[0].num;
		;
    break;}
case 36:
#line 235 "yacc_cis.y"
{
		    yyval.pwr.present = 0;
		;
    break;}
case 37:
#line 239 "yacc_cis.y"
{
		    yyval.pwr.present |= 1<<(yyvsp[0].pwr.present);
		    yyval.pwr.param[yyvsp[0].pwr.present] = yyvsp[0].pwr.param[0];
		;
    break;}
case 42:
#line 252 "yacc_cis.y"
{
		    int n = yyval.parse->cftable_entry.io.nwin;
		    yyval.parse->cftable_entry.io.win[n].base = yyvsp[-2].num;
		    yyval.parse->cftable_entry.io.win[n].len = yyvsp[0].num-yyvsp[-2].num+1;
		    yyval.parse->cftable_entry.io.nwin++;
		;
    break;}
case 43:
#line 259 "yacc_cis.y"
{
		    int n = yyval.parse->cftable_entry.io.nwin;
		    yyval.parse->cftable_entry.io.win[n].base = yyvsp[-2].num;
		    yyval.parse->cftable_entry.io.win[n].len = yyvsp[0].num-yyvsp[-2].num+1;
		    yyval.parse->cftable_entry.io.nwin++;
		;
    break;}
case 44:
#line 266 "yacc_cis.y"
{ yyval.parse->cftable_entry.io.flags |= CISTPL_IO_8BIT; ;
    break;}
case 45:
#line 268 "yacc_cis.y"
{ yyval.parse->cftable_entry.io.flags |= CISTPL_IO_16BIT; ;
    break;}
case 46:
#line 270 "yacc_cis.y"
{ yyval.parse->cftable_entry.io.flags |= yyvsp[-1].num; ;
    break;}
case 48:
#line 275 "yacc_cis.y"
{
		    int n = yyval.parse->cftable_entry.mem.nwin;
		    yyval.parse->cftable_entry.mem.win[n].card_addr = yyvsp[-4].num;
		    yyval.parse->cftable_entry.mem.win[n].host_addr = yyvsp[0].num;
		    yyval.parse->cftable_entry.mem.win[n].len = yyvsp[-2].num-yyvsp[-4].num+1;
		    yyval.parse->cftable_entry.mem.nwin++;
		;
    break;}
case 49:
#line 283 "yacc_cis.y"
{
		    int n = yyval.parse->cftable_entry.mem.nwin;
		    yyval.parse->cftable_entry.mem.win[n].card_addr = yyvsp[-4].num;
		    yyval.parse->cftable_entry.mem.win[n].host_addr = yyvsp[0].num;
		    yyval.parse->cftable_entry.mem.win[n].len = yyvsp[-2].num-yyvsp[-4].num+1;
		    yyval.parse->cftable_entry.mem.nwin++;
		;
    break;}
case 50:
#line 291 "yacc_cis.y"
{ yyval.parse->cftable_entry.io.flags |= CISTPL_IO_8BIT; ;
    break;}
case 51:
#line 293 "yacc_cis.y"
{ yyval.parse->cftable_entry.io.flags |= CISTPL_IO_16BIT; ;
    break;}
case 52:
#line 297 "yacc_cis.y"
{ yyval.parse->cftable_entry.irq.IRQInfo1 = (yyvsp[0].num & 0x0f); ;
    break;}
case 53:
#line 299 "yacc_cis.y"
{
		    yyval.parse->cftable_entry.irq.IRQInfo1 = IRQ_INFO2_VALID;
		    yyval.parse->cftable_entry.irq.IRQInfo2 = yyvsp[0].num;
		;
    break;}
case 54:
#line 304 "yacc_cis.y"
{ yyval.parse->cftable_entry.irq.IRQInfo1 |= IRQ_PULSE_ID; ;
    break;}
case 55:
#line 306 "yacc_cis.y"
{ yyval.parse->cftable_entry.irq.IRQInfo1 |= IRQ_LEVEL_ID; ;
    break;}
case 56:
#line 308 "yacc_cis.y"
{ yyval.parse->cftable_entry.irq.IRQInfo1 |= IRQ_SHARE_ID; ;
    break;}
case 57:
#line 312 "yacc_cis.y"
{
		    yyval.parse = calloc(1, sizeof(cisparse_t));
		    yyval.parse->cftable_entry.index = yyvsp[0].num;
		;
    break;}
case 58:
#line 317 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_DEFAULT; ;
    break;}
case 59:
#line 319 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_BVDS; ;
    break;}
case 60:
#line 321 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_WP; ;
    break;}
case 61:
#line 323 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_RDYBSY; ;
    break;}
case 62:
#line 325 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_MWAIT; ;
    break;}
case 63:
#line 327 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_AUDIO; ;
    break;}
case 64:
#line 329 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_READONLY; ;
    break;}
case 65:
#line 331 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_PWRDOWN; ;
    break;}
case 66:
#line 333 "yacc_cis.y"
{ yyval.parse->cftable_entry.vcc = yyvsp[0].pwr; ;
    break;}
case 67:
#line 335 "yacc_cis.y"
{ yyval.parse->cftable_entry.vpp1 = yyvsp[0].pwr; ;
    break;}
case 68:
#line 337 "yacc_cis.y"
{ yyval.parse->cftable_entry.vpp2 = yyvsp[0].pwr; ;
    break;}
case 73:
#line 345 "yacc_cis.y"
{ yyval.parse = NULL; ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 498 "/usr/lib/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 347 "yacc_cis.y"


static tuple_info_t *new_tuple(u_char type, cisparse_t *parse)
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
