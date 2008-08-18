#ifndef lint
static char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#endif
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"
#line 2 "yacc_cis.y"
/*
 * yacc_cis.y 1.11 2000/06/12 21:34:19
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License
 * at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and
 * limitations under the License. 
 *
 * The initial developer of the original code is David A. Hinds
 * <dahinds@users.sourceforge.net>.  Portions created by David A. Hinds
 * are Copyright (C) 1999 David A. Hinds.  All Rights Reserved.
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License version 2 (the "GPL"), in which
 * case the provisions of the GPL are applicable instead of the
 * above.  If you wish to allow the use of your version of this file
 * only under the terms of the GPL and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GPL.  If you do not delete the
 * provisions above, a recipient may use your version of this file
 * under either the MPL or the GPL.
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

#line 65 "yacc_cis.y"
typedef union {
    char *str;
    u_long num;
    float flt;
    cistpl_power_t pwr;
    cisparse_t *parse;
    tuple_info_t *tuple;
} YYSTYPE;
#line 72 "y.tab.c"
#define STRING 257
#define NUMBER 258
#define FLOAT 259
#define VOLTAGE 260
#define CURRENT 261
#define SIZE 262
#define VERS_1 263
#define MANFID 264
#define FUNCID 265
#define CONFIG 266
#define CFTABLE 267
#define MFC 268
#define CHECKSUM 269
#define POST 270
#define ROM 271
#define BASE 272
#define LAST_INDEX 273
#define DEV_INFO 274
#define ATTR_DEV_INFO 275
#define NO_INFO 276
#define TIME 277
#define TIMING 278
#define WAIT 279
#define READY 280
#define RESERVED 281
#define VNOM 282
#define VMIN 283
#define VMAX 284
#define ISTATIC 285
#define IAVG 286
#define IPEAK 287
#define IDOWN 288
#define VCC 289
#define VPP1 290
#define VPP2 291
#define IO 292
#define MEM 293
#define DEFAULT 294
#define BVD 295
#define WP 296
#define RDYBSY 297
#define MWAIT 298
#define AUDIO 299
#define READONLY 300
#define PWRDOWN 301
#define BIT8 302
#define BIT16 303
#define LINES 304
#define RANGE 305
#define IRQ_NO 306
#define MASK 307
#define LEVEL 308
#define PULSE 309
#define SHARED 310
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,   16,   16,   17,   17,   15,   15,   15,   15,
   15,   15,   15,   15,   15,   12,   12,   12,   13,   13,
   13,    3,    3,    4,    5,    5,    5,    6,    1,    1,
    1,    1,    1,    1,    1,    2,    2,   11,   11,   11,
   11,    8,    8,    8,    8,    8,    8,    9,    9,    9,
    9,   10,   10,   10,   10,   10,    7,    7,    7,    7,
    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,
    7,    7,   14,
};
short yylen[] = {                                         2,
    1,    2,    0,    2,    4,    5,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    5,    2,    1,    5,
    2,    2,    3,    4,    2,    2,    2,    7,    2,    2,
    2,    2,    2,    2,    2,    0,    2,    2,    3,    3,
    3,    5,    5,    2,    2,    5,    2,    7,    7,    2,
    2,    3,    4,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    2,    2,    3,    3,    3,    1,    1,
    1,    1,    6,
};
short yydefred[] = {                                      3,
    0,    0,   15,    0,    0,    0,    0,    0,    0,    0,
   16,   19,    0,   10,    0,   12,    0,    0,    0,    0,
    0,    0,    0,   14,    4,    0,   22,    0,   25,    0,
   57,    3,    0,    0,   26,   27,   38,   36,   36,   36,
    0,    0,   58,   59,   60,   61,   62,   63,   64,   65,
    0,   44,   45,    0,   47,    0,   50,   51,    0,   55,
   54,   56,    0,    0,    0,    0,   18,    0,   21,    0,
    0,    0,    0,    0,   23,    0,    0,    0,    0,    0,
   52,    0,    0,    0,    0,   39,   40,   41,    0,    0,
    3,   24,    0,    5,    0,    0,    0,    0,    0,    0,
    0,    0,   37,    0,    0,   53,    0,    0,    0,    0,
    0,    0,    0,    0,   29,   30,   31,   32,   33,   34,
   35,   42,    0,   46,   43,    0,   17,   20,    6,    0,
   73,    0,    0,   28,   48,   49,
};
short yydgoto[] = {                                       1,
  103,   76,   13,   14,   15,   16,   17,   18,   19,   20,
   21,   22,   23,   24,   25,    2,   26,
};
short yysindex[] = {                                      0,
    0, -225,    0, -211, -252, -242, -238, -207,  -71, -205,
    0,    0,   11,    0, -234,    0, -271,  -44,  -40, -300,
 -268, -244, -243,    0,    0,   12,    0,   13,    0, -200,
    0,    0,   14, -197,    0,    0,    0,    0,    0,    0,
 -189, -188,    0,    0,    0,    0,    0,    0,    0,    0,
 -253,    0,    0,   10,    0, -186,    0,    0, -185,    0,
    0,    0, -203, -202, -201, -199,    0, -198,    0,  -46,
 -178, -226, -122, -176,    0, -220, -220, -220,   38,   39,
    0, -173, -172,   42,   43,    0,    0,    0,   45,   46,
    0,    0, -167,    0,   31, -166, -165, -164, -163, -162,
 -161, -160,    0, -156, -155,    0,    4, -154, -153, -152,
 -151, -108, -159, -150,    0,    0,    0,    0,    0,    0,
    0,    0,   48,    0,    0,   49,    0,    0,    0, -149,
    0, -143, -142,    0,    0,    0,
};
short yyrindex[] = {                                      0,
    0,  106,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  350,    0,  370,    0,  395,    1,   47,   93,
  146,  416,  430,    0,    0,  107,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  192,  238,  304,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,
};
short yygindex[] = {                                      0,
    0,    6,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  -30,    0,
};
#define YYTABLESIZE 705
short yytable[] = {                                      56,
   69,   73,   94,   59,   81,   28,   37,   60,   61,   62,
   63,   64,   65,   66,   68,   29,  129,   38,   39,   40,
   41,   42,   43,   44,   45,   46,   47,   48,   49,   50,
    3,   67,   69,   30,   51,   35,   36,    4,    5,    6,
    7,    8,    9,   10,   77,   78,   70,   27,   11,   12,
   31,   32,   33,   82,   34,   70,   71,   72,   74,   75,
  112,   96,   97,   98,   99,  100,  101,  102,   79,   80,
   83,   84,   85,   86,   87,   88,   91,   89,   90,   92,
   93,   95,  104,  105,  106,  107,  108,  109,  110,  111,
  113,  114,   71,  115,  116,  117,  124,  118,  119,  120,
  121,  122,  123,  125,  126,    1,    2,  131,  134,  127,
  128,  132,  133,  130,  135,  136,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   69,    0,    0,    0,    0,
    0,    0,    0,    3,    0,    0,    0,    0,    0,    0,
    4,    5,    6,    7,    8,   72,   10,    3,    0,    0,
    0,   11,   12,    0,    4,    5,    6,    7,    8,    0,
   10,    0,    0,    0,    0,   11,   12,    0,    0,    0,
    0,   70,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   66,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   71,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   67,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   69,   52,   53,   54,
   55,   57,   58,   69,   69,   69,   69,   69,   69,   69,
   72,    0,    0,    0,   69,   69,    0,    0,   69,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   69,
   69,   69,   69,   69,   69,   69,   69,   69,   69,   69,
   69,   69,   70,   68,    0,    0,   69,    0,    0,   70,
   70,   70,   70,   70,   70,   70,   66,    0,    0,    0,
   70,   70,    0,    0,   70,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   70,   70,   70,   70,   70,
   70,   70,   70,   70,   70,   70,   70,   70,   71,    9,
    0,    0,   70,    0,    0,   71,   71,   71,   71,   71,
   71,   71,   67,    0,    0,    0,   71,   71,    0,   11,
   71,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   71,   71,   71,   71,   71,   71,   71,   71,   71,
   71,   71,   71,   71,   13,    0,    0,    0,   71,    0,
    0,   72,    0,    0,    0,    0,    0,    0,   72,   72,
   72,   72,   72,   72,   72,    7,    0,    0,    0,   72,
   72,    0,    0,   72,    0,    0,    0,    0,   68,    8,
    0,    0,    0,    0,   72,   72,   72,   72,   72,   72,
   72,   72,   72,   72,   72,   72,   72,   66,    0,    0,
    0,   72,    0,    0,   66,   66,   66,   66,   66,   66,
   66,    0,    0,    0,    0,   66,   66,    0,    0,   66,
    0,    0,    0,    0,    9,    0,    0,    0,    0,    0,
   66,   66,   66,   66,   66,   66,   66,   66,   66,   66,
   66,   66,   66,   67,   11,    0,    0,   66,    0,    0,
   67,   67,   67,   67,   67,   67,   67,    0,    0,    0,
    0,   67,   67,    0,    0,   67,    0,    0,    0,   13,
    0,    0,    0,    0,    0,    0,   67,   67,   67,   67,
   67,   67,   67,   67,   67,   67,   67,   67,   67,    0,
    7,    0,    0,   67,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    8,    0,    0,    0,    0,   68,
    0,    0,    0,    0,    0,    0,   68,   68,   68,   68,
   68,   68,   68,    0,    0,    0,    0,   68,   68,    0,
    0,   68,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   68,   68,   68,   68,   68,   68,   68,   68,
   68,   68,   68,   68,   68,    9,    0,    0,    0,   68,
    0,    0,    9,    9,    9,    9,    9,    9,    9,    0,
    0,    0,    0,    9,    9,   11,    0,    0,    0,    0,
    0,    0,   11,   11,   11,   11,   11,   11,   11,    0,
    0,    0,    0,   11,   11,    0,    0,    0,    0,    0,
   13,    0,    0,    0,    0,    0,    0,   13,   13,   13,
   13,   13,   13,   13,    0,    0,    0,    0,   13,   13,
    0,    7,    0,    0,    0,    0,    0,    0,    7,    7,
    7,    7,    7,    7,    7,    8,    0,    0,    0,    7,
    7,    0,    8,    8,    8,    8,    8,    8,    8,    0,
    0,    0,    0,    8,    8,
};
short yycheck[] = {                                      44,
    0,   32,  125,   44,  258,  258,  278,  308,  309,  310,
  279,  280,  281,  258,  258,  258,  125,  289,  290,  291,
  292,  293,  294,  295,  296,  297,  298,  299,  300,  301,
  256,  276,  276,  272,  306,  270,  271,  263,  264,  265,
  266,  267,  268,  269,   39,   40,    0,  259,  274,  275,
  258,  123,  258,  307,   44,   44,   44,  258,   45,  257,
   91,  282,  283,  284,  285,  286,  287,  288,  258,  258,
   61,  258,  258,  277,  277,  277,  123,  277,  277,  258,
  307,  258,   45,   45,  258,  258,   45,   45,   44,   44,
  258,   61,    0,  260,  260,  260,   93,  261,  261,  261,
  261,  258,  258,  258,  258,    0,    0,  258,  258,  262,
  262,   64,   64,  273,  258,  258,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  125,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  256,   -1,   -1,   -1,   -1,   -1,   -1,
  263,  264,  265,  266,  267,    0,  269,  256,   -1,   -1,
   -1,  274,  275,   -1,  263,  264,  265,  266,  267,   -1,
  269,   -1,   -1,   -1,   -1,  274,  275,   -1,   -1,   -1,
   -1,  125,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,    0,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  125,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,    0,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  256,  302,  303,  304,
  305,  302,  303,  263,  264,  265,  266,  267,  268,  269,
  125,   -1,   -1,   -1,  274,  275,   -1,   -1,  278,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  289,
  290,  291,  292,  293,  294,  295,  296,  297,  298,  299,
  300,  301,  256,    0,   -1,   -1,  306,   -1,   -1,  263,
  264,  265,  266,  267,  268,  269,  125,   -1,   -1,   -1,
  274,  275,   -1,   -1,  278,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  289,  290,  291,  292,  293,
  294,  295,  296,  297,  298,  299,  300,  301,  256,    0,
   -1,   -1,  306,   -1,   -1,  263,  264,  265,  266,  267,
  268,  269,  125,   -1,   -1,   -1,  274,  275,   -1,    0,
  278,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  289,  290,  291,  292,  293,  294,  295,  296,  297,
  298,  299,  300,  301,    0,   -1,   -1,   -1,  306,   -1,
   -1,  256,   -1,   -1,   -1,   -1,   -1,   -1,  263,  264,
  265,  266,  267,  268,  269,    0,   -1,   -1,   -1,  274,
  275,   -1,   -1,  278,   -1,   -1,   -1,   -1,  125,    0,
   -1,   -1,   -1,   -1,  289,  290,  291,  292,  293,  294,
  295,  296,  297,  298,  299,  300,  301,  256,   -1,   -1,
   -1,  306,   -1,   -1,  263,  264,  265,  266,  267,  268,
  269,   -1,   -1,   -1,   -1,  274,  275,   -1,   -1,  278,
   -1,   -1,   -1,   -1,  125,   -1,   -1,   -1,   -1,   -1,
  289,  290,  291,  292,  293,  294,  295,  296,  297,  298,
  299,  300,  301,  256,  125,   -1,   -1,  306,   -1,   -1,
  263,  264,  265,  266,  267,  268,  269,   -1,   -1,   -1,
   -1,  274,  275,   -1,   -1,  278,   -1,   -1,   -1,  125,
   -1,   -1,   -1,   -1,   -1,   -1,  289,  290,  291,  292,
  293,  294,  295,  296,  297,  298,  299,  300,  301,   -1,
  125,   -1,   -1,  306,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  125,   -1,   -1,   -1,   -1,  256,
   -1,   -1,   -1,   -1,   -1,   -1,  263,  264,  265,  266,
  267,  268,  269,   -1,   -1,   -1,   -1,  274,  275,   -1,
   -1,  278,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  289,  290,  291,  292,  293,  294,  295,  296,
  297,  298,  299,  300,  301,  256,   -1,   -1,   -1,  306,
   -1,   -1,  263,  264,  265,  266,  267,  268,  269,   -1,
   -1,   -1,   -1,  274,  275,  256,   -1,   -1,   -1,   -1,
   -1,   -1,  263,  264,  265,  266,  267,  268,  269,   -1,
   -1,   -1,   -1,  274,  275,   -1,   -1,   -1,   -1,   -1,
  256,   -1,   -1,   -1,   -1,   -1,   -1,  263,  264,  265,
  266,  267,  268,  269,   -1,   -1,   -1,   -1,  274,  275,
   -1,  256,   -1,   -1,   -1,   -1,   -1,   -1,  263,  264,
  265,  266,  267,  268,  269,  256,   -1,   -1,   -1,  274,
  275,   -1,  263,  264,  265,  266,  267,  268,  269,   -1,
   -1,   -1,   -1,  274,  275,
};
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 310
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,"','","'-'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'='",0,0,"'@'",0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"']'",0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'{'",0,"'}'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"STRING","NUMBER","FLOAT","VOLTAGE","CURRENT","SIZE","VERS_1","MANFID","FUNCID",
"CONFIG","CFTABLE","MFC","CHECKSUM","POST","ROM","BASE","LAST_INDEX","DEV_INFO",
"ATTR_DEV_INFO","NO_INFO","TIME","TIMING","WAIT","READY","RESERVED","VNOM",
"VMIN","VMAX","ISTATIC","IAVG","IPEAK","IDOWN","VCC","VPP1","VPP2","IO","MEM",
"DEFAULT","BVD","WP","RDYBSY","MWAIT","AUDIO","READONLY","PWRDOWN","BIT8",
"BIT16","LINES","RANGE","IRQ_NO","MASK","LEVEL","PULSE","SHARED",
};
char *yyrule[] = {
"$accept : cis",
"cis : chain",
"cis : chain mfc",
"chain :",
"chain : chain tuple",
"mfc : MFC '{' chain '}'",
"mfc : mfc ',' '{' chain '}'",
"tuple : dev_info",
"tuple : attr_dev_info",
"tuple : vers_1",
"tuple : manfid",
"tuple : funcid",
"tuple : config",
"tuple : cftab",
"tuple : checksum",
"tuple : error",
"dev_info : DEV_INFO",
"dev_info : dev_info NUMBER TIME ',' SIZE",
"dev_info : dev_info NO_INFO",
"attr_dev_info : ATTR_DEV_INFO",
"attr_dev_info : attr_dev_info NUMBER TIME ',' SIZE",
"attr_dev_info : attr_dev_info NO_INFO",
"vers_1 : VERS_1 FLOAT",
"vers_1 : vers_1 ',' STRING",
"manfid : MANFID NUMBER ',' NUMBER",
"funcid : FUNCID NUMBER",
"funcid : funcid POST",
"funcid : funcid ROM",
"config : CONFIG BASE NUMBER MASK NUMBER LAST_INDEX NUMBER",
"pwr : VNOM VOLTAGE",
"pwr : VMIN VOLTAGE",
"pwr : VMAX VOLTAGE",
"pwr : ISTATIC CURRENT",
"pwr : IAVG CURRENT",
"pwr : IPEAK CURRENT",
"pwr : IDOWN CURRENT",
"pwrlist :",
"pwrlist : pwrlist pwr",
"timing : cftab TIMING",
"timing : timing WAIT TIME",
"timing : timing READY TIME",
"timing : timing RESERVED TIME",
"io : cftab IO NUMBER '-' NUMBER",
"io : io ',' NUMBER '-' NUMBER",
"io : io BIT8",
"io : io BIT16",
"io : io LINES '=' NUMBER ']'",
"io : io RANGE",
"mem : cftab MEM NUMBER '-' NUMBER '@' NUMBER",
"mem : mem ',' NUMBER '-' NUMBER '@' NUMBER",
"mem : mem BIT8",
"mem : mem BIT16",
"irq : cftab IRQ_NO NUMBER",
"irq : cftab IRQ_NO MASK NUMBER",
"irq : irq PULSE",
"irq : irq LEVEL",
"irq : irq SHARED",
"cftab : CFTABLE NUMBER",
"cftab : cftab DEFAULT",
"cftab : cftab BVD",
"cftab : cftab WP",
"cftab : cftab RDYBSY",
"cftab : cftab MWAIT",
"cftab : cftab AUDIO",
"cftab : cftab READONLY",
"cftab : cftab PWRDOWN",
"cftab : cftab VCC pwrlist",
"cftab : cftab VPP1 pwrlist",
"cftab : cftab VPP2 pwrlist",
"cftab : io",
"cftab : mem",
"cftab : irq",
"cftab : timing",
"checksum : CHECKSUM NUMBER '-' NUMBER '=' NUMBER",
};
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH 500
#endif
#endif
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short yyss[YYSTACKSIZE];
YYSTYPE yyvs[YYSTACKSIZE];
#define yystacksize YYSTACKSIZE
#line 359 "yacc_cis.y"

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
#line 500 "y.tab.c"
#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
yyparse()
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register char *yys;
    extern char *getenv();

    if (yys = getenv("YYDEBUG"))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if (yyn = yydefred[yystate]) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yyss + yystacksize - 1)
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#ifdef lint
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#ifdef lint
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yyss + yystacksize - 1)
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 1:
#line 84 "yacc_cis.y"
{ cis_root = yyvsp[0].tuple; }
break;
case 2:
#line 86 "yacc_cis.y"
{ cis_root = yyvsp[-1].tuple; }
break;
case 3:
#line 90 "yacc_cis.y"
{ yyval.tuple = NULL; }
break;
case 4:
#line 92 "yacc_cis.y"
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
		}
break;
case 5:
#line 107 "yacc_cis.y"
{ mfc[nf++] = yyvsp[-1].tuple; }
break;
case 6:
#line 109 "yacc_cis.y"
{ mfc[nf++] = yyvsp[-1].tuple; }
break;
case 7:
#line 113 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_DEVICE, yyvsp[0].parse); }
break;
case 8:
#line 115 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_DEVICE_A, yyvsp[0].parse); }
break;
case 9:
#line 117 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_VERS_1, yyvsp[0].parse); }
break;
case 10:
#line 119 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_MANFID, yyvsp[0].parse); }
break;
case 11:
#line 121 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_FUNCID, yyvsp[0].parse); }
break;
case 12:
#line 123 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_CONFIG, yyvsp[0].parse); }
break;
case 13:
#line 125 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_CFTABLE_ENTRY, yyvsp[0].parse); }
break;
case 14:
#line 127 "yacc_cis.y"
{ yyval.tuple = NULL; }
break;
case 15:
#line 129 "yacc_cis.y"
{ yyval.tuple = NULL; }
break;
case 16:
#line 133 "yacc_cis.y"
{ yyval.parse = calloc(1, sizeof(cisparse_t)); }
break;
case 17:
#line 135 "yacc_cis.y"
{
		    yyval.parse->device.dev[yyval.parse->device.ndev].type = yyvsp[-3].num;
		    yyval.parse->device.dev[yyval.parse->device.ndev].speed = yyvsp[-2].num;
		    yyval.parse->device.dev[yyval.parse->device.ndev].size = yyvsp[0].num;
		    yyval.parse->device.ndev++;
		}
break;
case 19:
#line 145 "yacc_cis.y"
{ yyval.parse = calloc(1, sizeof(cisparse_t)); }
break;
case 20:
#line 147 "yacc_cis.y"
{
		    yyval.parse->device.dev[yyval.parse->device.ndev].type = yyvsp[-3].num;
		    yyval.parse->device.dev[yyval.parse->device.ndev].speed = yyvsp[-2].num;
		    yyval.parse->device.dev[yyval.parse->device.ndev].size = yyvsp[0].num;
		    yyval.parse->device.ndev++;
		}
break;
case 22:
#line 157 "yacc_cis.y"
{
		    yyval.parse = calloc(1, sizeof(cisparse_t));
		    yyval.parse->version_1.major = yyvsp[0].flt;
		    yyvsp[0].flt -= floor(yyvsp[0].flt+0.01);
		    while (fabs(yyvsp[0].flt - floor(yyvsp[0].flt+0.5)) > 0.01) {
			yyvsp[0].flt *= 10;
		    }
		    yyval.parse->version_1.minor = yyvsp[0].flt+0.01;
		}
break;
case 23:
#line 167 "yacc_cis.y"
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
		}
break;
case 24:
#line 181 "yacc_cis.y"
{
		    yyval.parse = calloc(1, sizeof(cisparse_t));
		    yyval.parse->manfid.manf = yyvsp[-2].num;
		    yyval.parse->manfid.card = yyvsp[0].num;
		}
break;
case 25:
#line 189 "yacc_cis.y"
{
		    yyval.parse = calloc(1, sizeof(cisparse_t));
		    yyval.parse->funcid.func = yyvsp[0].num;
		}
break;
case 26:
#line 194 "yacc_cis.y"
{ yyval.parse->funcid.sysinit |= CISTPL_SYSINIT_POST; }
break;
case 27:
#line 196 "yacc_cis.y"
{ yyval.parse->funcid.sysinit |= CISTPL_SYSINIT_ROM; }
break;
case 28:
#line 200 "yacc_cis.y"
{
		    yyval.parse = calloc(1, sizeof(cisparse_t));
		    yyval.parse->config.base = yyvsp[-4].num;
		    yyval.parse->config.rmask[0] = yyvsp[-2].num;
		    yyval.parse->config.last_idx = yyvsp[0].num;
		}
break;
case 29:
#line 209 "yacc_cis.y"
{
		    yyval.pwr.present = CISTPL_POWER_VNOM;
		    yyval.pwr.param[0] = yyvsp[0].num;
		}
break;
case 30:
#line 214 "yacc_cis.y"
{
		    yyval.pwr.present = CISTPL_POWER_VMIN;
		    yyval.pwr.param[0] = yyvsp[0].num;
		}
break;
case 31:
#line 219 "yacc_cis.y"
{
		    yyval.pwr.present = CISTPL_POWER_VMAX;
		    yyval.pwr.param[0] = yyvsp[0].num;
		}
break;
case 32:
#line 224 "yacc_cis.y"
{
		    yyval.pwr.present = CISTPL_POWER_ISTATIC;
		    yyval.pwr.param[0] = yyvsp[0].num;
		}
break;
case 33:
#line 229 "yacc_cis.y"
{
		    yyval.pwr.present = CISTPL_POWER_IAVG;
		    yyval.pwr.param[0] = yyvsp[0].num;
		}
break;
case 34:
#line 234 "yacc_cis.y"
{
		    yyval.pwr.present = CISTPL_POWER_IPEAK;
		    yyval.pwr.param[0] = yyvsp[0].num;
		}
break;
case 35:
#line 239 "yacc_cis.y"
{
		    yyval.pwr.present = CISTPL_POWER_IDOWN;
		    yyval.pwr.param[0] = yyvsp[0].num;
		}
break;
case 36:
#line 246 "yacc_cis.y"
{
		    yyval.pwr.present = 0;
		}
break;
case 37:
#line 250 "yacc_cis.y"
{
		    yyval.pwr.present |= 1<<(yyvsp[0].pwr.present);
		    yyval.pwr.param[yyvsp[0].pwr.present] = yyvsp[0].pwr.param[0];
		}
break;
case 42:
#line 263 "yacc_cis.y"
{
		    int n = yyval.parse->cftable_entry.io.nwin;
		    yyval.parse->cftable_entry.io.win[n].base = yyvsp[-2].num;
		    yyval.parse->cftable_entry.io.win[n].len = yyvsp[0].num-yyvsp[-2].num+1;
		    yyval.parse->cftable_entry.io.nwin++;
		}
break;
case 43:
#line 270 "yacc_cis.y"
{
		    int n = yyval.parse->cftable_entry.io.nwin;
		    yyval.parse->cftable_entry.io.win[n].base = yyvsp[-2].num;
		    yyval.parse->cftable_entry.io.win[n].len = yyvsp[0].num-yyvsp[-2].num+1;
		    yyval.parse->cftable_entry.io.nwin++;
		}
break;
case 44:
#line 277 "yacc_cis.y"
{ yyval.parse->cftable_entry.io.flags |= CISTPL_IO_8BIT; }
break;
case 45:
#line 279 "yacc_cis.y"
{ yyval.parse->cftable_entry.io.flags |= CISTPL_IO_16BIT; }
break;
case 46:
#line 281 "yacc_cis.y"
{ yyval.parse->cftable_entry.io.flags |= yyvsp[-1].num; }
break;
case 48:
#line 286 "yacc_cis.y"
{
		    int n = yyval.parse->cftable_entry.mem.nwin;
		    yyval.parse->cftable_entry.mem.win[n].card_addr = yyvsp[-4].num;
		    yyval.parse->cftable_entry.mem.win[n].host_addr = yyvsp[0].num;
		    yyval.parse->cftable_entry.mem.win[n].len = yyvsp[-2].num-yyvsp[-4].num+1;
		    yyval.parse->cftable_entry.mem.nwin++;
		}
break;
case 49:
#line 294 "yacc_cis.y"
{
		    int n = yyval.parse->cftable_entry.mem.nwin;
		    yyval.parse->cftable_entry.mem.win[n].card_addr = yyvsp[-4].num;
		    yyval.parse->cftable_entry.mem.win[n].host_addr = yyvsp[0].num;
		    yyval.parse->cftable_entry.mem.win[n].len = yyvsp[-2].num-yyvsp[-4].num+1;
		    yyval.parse->cftable_entry.mem.nwin++;
		}
break;
case 50:
#line 302 "yacc_cis.y"
{ yyval.parse->cftable_entry.io.flags |= CISTPL_IO_8BIT; }
break;
case 51:
#line 304 "yacc_cis.y"
{ yyval.parse->cftable_entry.io.flags |= CISTPL_IO_16BIT; }
break;
case 52:
#line 308 "yacc_cis.y"
{ yyval.parse->cftable_entry.irq.IRQInfo1 = (yyvsp[0].num & 0x0f); }
break;
case 53:
#line 310 "yacc_cis.y"
{
		    yyval.parse->cftable_entry.irq.IRQInfo1 = IRQ_INFO2_VALID;
		    yyval.parse->cftable_entry.irq.IRQInfo2 = yyvsp[0].num;
		}
break;
case 54:
#line 315 "yacc_cis.y"
{ yyval.parse->cftable_entry.irq.IRQInfo1 |= IRQ_PULSE_ID; }
break;
case 55:
#line 317 "yacc_cis.y"
{ yyval.parse->cftable_entry.irq.IRQInfo1 |= IRQ_LEVEL_ID; }
break;
case 56:
#line 319 "yacc_cis.y"
{ yyval.parse->cftable_entry.irq.IRQInfo1 |= IRQ_SHARE_ID; }
break;
case 57:
#line 323 "yacc_cis.y"
{
		    yyval.parse = calloc(1, sizeof(cisparse_t));
		    yyval.parse->cftable_entry.index = yyvsp[0].num;
		}
break;
case 58:
#line 328 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_DEFAULT; }
break;
case 59:
#line 330 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_BVDS; }
break;
case 60:
#line 332 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_WP; }
break;
case 61:
#line 334 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_RDYBSY; }
break;
case 62:
#line 336 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_MWAIT; }
break;
case 63:
#line 338 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_AUDIO; }
break;
case 64:
#line 340 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_READONLY; }
break;
case 65:
#line 342 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_PWRDOWN; }
break;
case 66:
#line 344 "yacc_cis.y"
{ yyval.parse->cftable_entry.vcc = yyvsp[0].pwr; }
break;
case 67:
#line 346 "yacc_cis.y"
{ yyval.parse->cftable_entry.vpp1 = yyvsp[0].pwr; }
break;
case 68:
#line 348 "yacc_cis.y"
{ yyval.parse->cftable_entry.vpp2 = yyvsp[0].pwr; }
break;
case 73:
#line 356 "yacc_cis.y"
{ yyval.parse = NULL; }
break;
#line 994 "y.tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yyss + yystacksize - 1)
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
