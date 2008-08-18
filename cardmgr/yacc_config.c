#ifndef lint
static char const 
yyrcsid[] = "$FreeBSD: src/usr.bin/yacc/skeleton.c,v 1.28 2000/01/17 02:04:06 bde Exp $";
#endif
#include <stdlib.h>
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYLEX yylex()
#define YYEMPTY -1
#define yyclearin (yychar=(YYEMPTY))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING() (yyerrflag!=0)
static int yygrowstack();
#define YYPREFIX "yy"
#line 2 "yacc_config.y"
/*
 * yacc_config.y 1.56 2002/05/16 06:07:40
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
 * terms of the GNU General Public License version 2 (the "GPL"), in
 * which case the provisions of the GPL are applicable instead of the
 * above.  If you wish to allow the use of your version of this file
 * only under the terms of the GPL and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GPL.  If you do not delete the
 * provisions above, a recipient may use your version of this file
 * under either the MPL or the GPL.
 */
    
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>

#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/ds.h>
    
#include "cardmgr.h"

/* If bison: generate nicer error messages */ 
#define YYERROR_VERBOSE 1
 
/* from lex_config, for nice error messages */
extern char *current_file;
extern int current_lineno;

void yyerror(char *msg, ...);

static int add_binding(card_info_t *card, char *name, int fn);
static int add_module(device_info_t *card, char *name);

#line 65 "yacc_config.y"
typedef union {
    char *str;
    u_long num;
    struct device_info_t *device;
    struct card_info_t *card;
    struct mtd_ident_t *mtd;
    struct adjust_list_t *adjust;
} YYSTYPE;
#line 82 "y.tab.c"
#define YYERRCODE 256
#define DEVICE 257
#define CARD 258
#define ANONYMOUS 259
#define TUPLE 260
#define MANFID 261
#define VERSION 262
#define FUNCTION 263
#define PCI 264
#define BIND 265
#define CIS 266
#define TO 267
#define NEEDS_MTD 268
#define MODULE 269
#define OPTS 270
#define CLASS 271
#define REGION 272
#define JEDEC 273
#define DTYPE 274
#define DEFAULT 275
#define MTD 276
#define INCLUDE 277
#define EXCLUDE 278
#define RESERVE 279
#define IRQ_NO 280
#define PORT 281
#define MEMORY 282
#define STRING 283
#define NUMBER 284
#define SOURCE 285
const short yylhs[] = {                                        -1,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
    1,    1,    1,    2,    2,    2,    3,    3,    3,    3,
    7,    7,    7,    7,    7,    7,    7,    7,    7,    8,
    9,   10,   11,   12,   12,   13,   15,   14,   14,   14,
   14,    4,   21,    5,    5,    5,    6,   16,   16,   16,
   16,   18,   17,   19,   20,   20,   22,
};
const short yylen[] = {                                         2,
    0,    2,    2,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    3,    2,    4,    4,    2,    1,    1,    1,
    2,    1,    1,    1,    1,    1,    1,    1,    1,    2,
    7,    5,    5,    3,    3,    3,    3,    3,    5,    3,
    5,    2,    4,    3,    3,    3,    3,    2,    1,    1,
    1,    3,    4,    2,    3,    3,    4,
};
const short yydefred[] = {                                      1,
    0,    9,    0,    0,    0,    0,    0,    0,    0,    0,
    8,    0,    0,   18,    0,   20,    0,   22,   23,   24,
   25,    0,   27,    0,   29,    0,   50,   49,   51,    0,
    6,    7,   17,   21,    0,   48,    0,    0,    0,    0,
   10,   11,   12,    0,   42,    0,    0,    0,    0,   30,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   54,    0,    0,    0,    0,   14,    0,    0,   13,
   44,   47,   45,   46,    0,    0,   34,   36,    0,    0,
   37,   35,    0,    0,   52,   55,   56,   43,   57,    0,
    0,    0,    0,    0,    0,    0,   53,   15,   16,    0,
   32,   33,   39,   41,    0,   31,
};
const short yydgoto[] = {                                       1,
   12,   41,   13,   14,   15,   16,   17,   18,   19,   20,
   21,   22,   23,   24,   25,   26,   27,   28,   29,   30,
   31,   32,
};
const short yysindex[] = {                                      0,
 -252,    0, -276, -267, -261, -260, -255, -246, -246, -246,
    0,  -15, -250,    0,  -44,    0, -251,    0,    0,    0,
    0,  -14,    0,  -12,    0, -235,    0,    0,    0, -228,
    0,    0,    0,    0, -227,    0, -226, -238, -237, -236,
    0,    0,    0, -246,    0, -234, -233, -232, -231,    0,
 -230, -229, -225, -224, -222, -220, -219, -218, -217, -216,
 -215,    0, -213, -212, -211, -210,    0,    8,   11,    0,
    0,    0,    0,    0,   13,   15,    0,    0,   23, -193,
    0,    0, -191, -207,    0,    0,    0,    0,    0, -206,
 -205, -204, -203, -202, -201, -200,    0,    0,    0,   41,
    0,    0,    0,    0, -197,    0,
};
const short yyrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  132,  183,    0,  115,    0,  149,    0,    0,    0,
    0,   61,    0,   91,    0,    0,    0,    0,    0,  166,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
    0,    0,   31,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,
};
const short yygindex[] = {                                      0,
    0,   -7,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,
};
#define YYTABLESIZE 468
const short yytable[] = {                                      49,
   38,   42,   43,    2,    3,    4,   33,   50,   51,   52,
   53,   54,   55,   56,   57,   34,    5,   45,   46,    6,
   47,   35,   36,    7,    8,    9,   10,   37,   44,   58,
   40,   59,   11,   38,   39,   40,   70,   60,   61,   62,
   63,   64,   65,   66,   38,   67,   68,   69,   71,   72,
   73,   74,   90,   75,   76,   91,   92,   77,   93,   78,
   26,   79,   80,   81,   82,   83,   94,   84,   85,   86,
   87,   88,   89,   95,   40,   96,   97,   98,   99,  100,
  101,  102,  103,  104,  105,  106,    0,    0,    0,    0,
   28,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   19,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    2,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    5,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    4,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    3,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   48,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   38,   38,   38,   38,
   38,   38,   38,   38,   38,   38,   38,    0,    0,   38,
    0,    0,   38,    0,    0,    0,   38,   38,   38,   38,
    0,    0,    0,    0,    0,   38,   40,   40,   40,   40,
   40,   40,   40,   40,   40,   40,   40,    0,    0,   40,
    0,    0,   40,    0,    0,    0,   40,   40,   40,   40,
    0,    0,    0,    0,    0,   40,   26,   26,   26,   26,
   26,   26,   26,   26,   26,   26,   26,    0,    0,   26,
    0,    0,   26,    0,    0,    0,   26,   26,   26,   26,
    0,    0,    0,    0,    0,   26,   28,   28,   28,   28,
   28,   28,   28,   28,   28,   28,   28,    0,    0,   28,
    0,    0,   28,    0,    0,    0,   28,   28,   28,   28,
   19,   19,   19,    0,    0,   28,    0,    0,    0,    0,
    0,    0,   19,   19,    0,   19,   19,    2,    2,    2,
   19,   19,   19,   19,    0,    0,    0,    0,    0,   19,
    2,    0,    0,    2,    5,    5,    5,    2,    2,    2,
    2,    0,    0,    0,    0,    0,    2,    5,    0,    0,
    5,    4,    4,    4,    5,    5,    5,    5,    0,    0,
    0,    0,    0,    5,    4,    0,    0,    4,    3,    3,
    3,    4,    4,    4,    4,    0,    0,    0,    0,    0,
    4,    0,    0,    0,    3,    0,    0,    0,    3,    3,
    3,    3,    0,    0,    0,    0,    0,    3,
};
const short yycheck[] = {                                      44,
    0,    9,   10,  256,  257,  258,  283,  259,  260,  261,
  262,  263,  264,  265,  266,  283,  269,  268,  269,  272,
  271,  283,  283,  276,  277,  278,  279,  283,   44,   44,
    0,   44,  285,  280,  281,  282,   44,  273,  274,  275,
  276,  270,  270,  270,   44,  284,  284,  284,  283,  283,
  283,  283,   45,  284,  284,   45,   44,  283,   44,  284,
    0,  284,  283,  283,  283,  283,   44,  284,  284,  283,
  283,  283,  283,  267,   44,  267,  284,  284,  284,  284,
  284,  284,  284,  284,   44,  283,   -1,   -1,   -1,   -1,
    0,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,    0,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,    0,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,    0,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,    0,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,    0,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  270,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  256,  257,  258,  259,
  260,  261,  262,  263,  264,  265,  266,   -1,   -1,  269,
   -1,   -1,  272,   -1,   -1,   -1,  276,  277,  278,  279,
   -1,   -1,   -1,   -1,   -1,  285,  256,  257,  258,  259,
  260,  261,  262,  263,  264,  265,  266,   -1,   -1,  269,
   -1,   -1,  272,   -1,   -1,   -1,  276,  277,  278,  279,
   -1,   -1,   -1,   -1,   -1,  285,  256,  257,  258,  259,
  260,  261,  262,  263,  264,  265,  266,   -1,   -1,  269,
   -1,   -1,  272,   -1,   -1,   -1,  276,  277,  278,  279,
   -1,   -1,   -1,   -1,   -1,  285,  256,  257,  258,  259,
  260,  261,  262,  263,  264,  265,  266,   -1,   -1,  269,
   -1,   -1,  272,   -1,   -1,   -1,  276,  277,  278,  279,
  256,  257,  258,   -1,   -1,  285,   -1,   -1,   -1,   -1,
   -1,   -1,  268,  269,   -1,  271,  272,  256,  257,  258,
  276,  277,  278,  279,   -1,   -1,   -1,   -1,   -1,  285,
  269,   -1,   -1,  272,  256,  257,  258,  276,  277,  278,
  279,   -1,   -1,   -1,   -1,   -1,  285,  269,   -1,   -1,
  272,  256,  257,  258,  276,  277,  278,  279,   -1,   -1,
   -1,   -1,   -1,  285,  269,   -1,   -1,  272,  256,  257,
  258,  276,  277,  278,  279,   -1,   -1,   -1,   -1,   -1,
  285,   -1,   -1,   -1,  272,   -1,   -1,   -1,  276,  277,
  278,  279,   -1,   -1,   -1,   -1,   -1,  285,
};
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 285
#if YYDEBUG
const char * const yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,"','","'-'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"DEVICE","CARD",
"ANONYMOUS","TUPLE","MANFID","VERSION","FUNCTION","PCI","BIND","CIS","TO",
"NEEDS_MTD","MODULE","OPTS","CLASS","REGION","JEDEC","DTYPE","DEFAULT","MTD",
"INCLUDE","EXCLUDE","RESERVE","IRQ_NO","PORT","MEMORY","STRING","NUMBER",
"SOURCE",
};
const char * const yyrule[] = {
"$accept : list",
"list :",
"list : list adjust",
"list : list device",
"list : list mtd",
"list : list card",
"list : list opts",
"list : list mtd_opts",
"list : list SOURCE",
"list : list error",
"adjust : INCLUDE resource",
"adjust : EXCLUDE resource",
"adjust : RESERVE resource",
"adjust : adjust ',' resource",
"resource : IRQ_NO NUMBER",
"resource : PORT NUMBER '-' NUMBER",
"resource : MEMORY NUMBER '-' NUMBER",
"device : DEVICE STRING",
"device : needs_mtd",
"device : module",
"device : class",
"card : CARD STRING",
"card : anonymous",
"card : tuple",
"card : manfid",
"card : pci",
"card : version",
"card : function",
"card : bind",
"card : cis",
"anonymous : card ANONYMOUS",
"tuple : card TUPLE NUMBER ',' NUMBER ',' STRING",
"manfid : card MANFID NUMBER ',' NUMBER",
"pci : card PCI NUMBER ',' NUMBER",
"version : card VERSION STRING",
"version : version ',' STRING",
"function : card FUNCTION NUMBER",
"cis : card CIS STRING",
"bind : card BIND STRING",
"bind : card BIND STRING TO NUMBER",
"bind : bind ',' STRING",
"bind : bind ',' STRING TO NUMBER",
"needs_mtd : device NEEDS_MTD",
"opts : MODULE STRING OPTS STRING",
"module : device MODULE STRING",
"module : module OPTS STRING",
"module : module ',' STRING",
"class : device CLASS STRING",
"region : REGION STRING",
"region : dtype",
"region : jedec",
"region : default",
"dtype : region DTYPE NUMBER",
"jedec : region JEDEC NUMBER NUMBER",
"default : region DEFAULT",
"mtd : region MTD STRING",
"mtd : mtd OPTS STRING",
"mtd_opts : MTD STRING OPTS STRING",
};
#endif
#if YYDEBUG
#include <stdio.h>
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH 10000
#endif
#endif
#define YYINITSTACKSIZE 200
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short *yyss;
short *yysslim;
YYSTYPE *yyvs;
int yystacksize;
#line 470 "yacc_config.y"
void yyerror(char *msg, ...)
{
     va_list ap;
     char str[256];

     va_start(ap, msg);
     sprintf(str, "error in file '%s' line %d: ",
	     current_file, current_lineno);
     vsprintf(str+strlen(str), msg, ap);
#if YYDEBUG
     fprintf(stderr, "%s\n", str);
#else
     syslog(LOG_ERR, "%s", str);
#endif
     va_end(ap);
}

static int add_binding(card_info_t *card, char *name, int fn)
{
    device_info_t *dev = root_device;
    if (card->bindings == MAX_BINDINGS) {
	yyerror("too many bindings\n");
	return -1;
    }
    for (; dev; dev = dev->next)
	if (strcmp((char *)dev->dev_info, name) == 0) break;
    if (dev == NULL) {
	yyerror("unknown device '%s'", name);
	return -1;
    }
    card->device[card->bindings] = dev;
    card->dev_fn[card->bindings] = fn;
    card->bindings++;
    free(name);
    return 0;
}

static int add_module(device_info_t *dev, char *name)
{
    if (dev->modules == MAX_MODULES) {
	yyerror("too many modules for '%s'", dev->dev_info);
	return -1;
    }
    dev->module[dev->modules] = name;
    dev->opts[dev->modules] = NULL;
    dev->modules++;
    return 0;
}

#if YYDEBUG
adjust_list_t *root_adjust = NULL;
device_info_t *root_device = NULL;
card_info_t *root_card = NULL, *blank_card = NULL, *root_func = NULL;
mtd_ident_t *root_mtd = NULL, *default_mtd = NULL;

void main(int argc, char *argv[])
{
    yydebug = 1;
    if (argc > 1)
	parse_configfile(argv[1]);
}
#endif
#line 447 "y.tab.c"
/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack()
{
    int newsize, i;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = yystacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;
    i = yyssp - yyss;
    newss = yyss ? (short *)realloc(yyss, newsize * sizeof *newss) :
      (short *)malloc(newsize * sizeof *newss);
    if (newss == NULL)
        return -1;
    yyss = newss;
    yyssp = newss + i;
    newvs = yyvs ? (YYSTYPE *)realloc(yyvs, newsize * sizeof *newvs) :
      (YYSTYPE *)malloc(newsize * sizeof *newvs);
    if (newvs == NULL)
        return -1;
    yyvs = newvs;
    yyvsp = newvs + i;
    yystacksize = newsize;
    yysslim = yyss + newsize - 1;
    return 0;
}

#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab

#ifndef YYPARSE_PARAM
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG void
#define YYPARSE_PARAM_DECL
#else	/* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif	/* ANSI-C/C++ */
#else	/* YYPARSE_PARAM */
#ifndef YYPARSE_PARAM_TYPE
#define YYPARSE_PARAM_TYPE void *
#endif
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG YYPARSE_PARAM_TYPE YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else	/* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL YYPARSE_PARAM_TYPE YYPARSE_PARAM;
#endif	/* ANSI-C/C++ */
#endif	/* ! YYPARSE_PARAM */

int
yyparse (YYPARSE_PARAM_ARG)
    YYPARSE_PARAM_DECL
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register const char *yys;

    if ((yys = getenv("YYDEBUG")))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    if (yyss == NULL && yygrowstack()) goto yyoverflow;
    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if ((yyn = yydefred[yystate])) goto yyreduce;
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
        if (yyssp >= yysslim && yygrowstack())
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
#if defined(lint) || defined(__GNUC__)
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#if defined(lint) || defined(__GNUC__)
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
                if (yyssp >= yysslim && yygrowstack())
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
case 2:
#line 84 "yacc_config.y"
{
		    adjust_list_t **tail = &root_adjust;
		    while (*tail != NULL) tail = &(*tail)->next;
		    *tail = yyvsp[0].adjust;
		}
break;
case 3:
#line 90 "yacc_config.y"
{
		    yyvsp[0].device->next = root_device;
		    root_device = yyvsp[0].device;
		}
break;
case 4:
#line 95 "yacc_config.y"
{
		    if (yyvsp[0].mtd->mtd_type == 0) {
			yyerror("no ID method for '%s'", yyvsp[0].mtd->name);
			YYERROR;
		    }
		    if (yyvsp[0].mtd->module == NULL) {
			yyerror("no MTD module for '%s'", yyvsp[0].mtd->name);
			YYERROR;
		    }
		    yyvsp[0].mtd->next = root_mtd;
		    root_mtd = yyvsp[0].mtd;
		}
break;
case 5:
#line 108 "yacc_config.y"
{
		    if (yyvsp[0].card->ident_type == 0) {
			yyerror("no ID method for '%s'", yyvsp[0].card->name);
			YYERROR;
		    }
		    if (yyvsp[0].card->bindings == 0) {
			yyerror("no driver bindings for '%s'", yyvsp[0].card->name);
			YYERROR;
		    }
		    if (yyvsp[0].card->ident_type == FUNC_IDENT) {
			yyvsp[0].card->next = root_func;
			root_func = yyvsp[0].card;
		    } else {
			yyvsp[0].card->next = root_card;
			root_card = yyvsp[0].card;
		    }
		}
break;
case 10:
#line 132 "yacc_config.y"
{
		    yyvsp[0].adjust->adj.Action = ADD_MANAGED_RESOURCE;
		    yyval.adjust = yyvsp[0].adjust;
		}
break;
case 11:
#line 137 "yacc_config.y"
{
		    yyvsp[0].adjust->adj.Action = REMOVE_MANAGED_RESOURCE;
		    yyval.adjust = yyvsp[0].adjust;
		}
break;
case 12:
#line 142 "yacc_config.y"
{
		    yyvsp[0].adjust->adj.Action = ADD_MANAGED_RESOURCE;
		    yyvsp[0].adjust->adj.Attributes |= RES_RESERVED;
		    yyval.adjust = yyvsp[0].adjust;
		}
break;
case 13:
#line 148 "yacc_config.y"
{
		    yyvsp[0].adjust->adj.Action = yyvsp[-2].adjust->adj.Action;
		    yyvsp[0].adjust->adj.Attributes = yyvsp[-2].adjust->adj.Attributes;
		    yyvsp[0].adjust->next = yyvsp[-2].adjust;
		    yyval.adjust = yyvsp[0].adjust;
		}
break;
case 14:
#line 157 "yacc_config.y"
{
		    yyval.adjust = calloc(sizeof(adjust_list_t), 1);
		    yyval.adjust->adj.Resource = RES_IRQ;
		    yyval.adjust->adj.resource.irq.IRQ = yyvsp[0].num;
		}
break;
case 15:
#line 163 "yacc_config.y"
{
		    if ((yyvsp[0].num < yyvsp[-2].num) || (yyvsp[0].num > 0xffff)) {
			yyerror("invalid port range 0x%x-0x%x", yyvsp[-2].num, yyvsp[0].num);
			YYERROR;
		    }
		    yyval.adjust = calloc(sizeof(adjust_list_t), 1);
		    yyval.adjust->adj.Resource = RES_IO_RANGE;
		    yyval.adjust->adj.resource.io.BasePort = yyvsp[-2].num;
		    yyval.adjust->adj.resource.io.NumPorts = yyvsp[0].num - yyvsp[-2].num + 1;
		}
break;
case 16:
#line 174 "yacc_config.y"
{
		    if (yyvsp[0].num < yyvsp[-2].num) {
			yyerror("invalid address range 0x%x-0x%x", yyvsp[-2].num, yyvsp[0].num);
			YYERROR;
		    }
		    yyval.adjust = calloc(sizeof(adjust_list_t), 1);
		    yyval.adjust->adj.Resource = RES_MEMORY_RANGE;
		    yyval.adjust->adj.resource.memory.Base = yyvsp[-2].num;
		    yyval.adjust->adj.resource.memory.Size = yyvsp[0].num - yyvsp[-2].num + 1;
		}
break;
case 17:
#line 187 "yacc_config.y"
{
		    yyval.device = calloc(sizeof(device_info_t), 1);
		    yyval.device->refs = 1;
		    strcpy(yyval.device->dev_info, yyvsp[0].str);
		    free(yyvsp[0].str);
		}
break;
case 21:
#line 199 "yacc_config.y"
{
		    yyval.card = calloc(sizeof(card_info_t), 1);
		    yyval.card->refs = 1;
		    yyval.card->name = yyvsp[0].str;
		}
break;
case 30:
#line 215 "yacc_config.y"
{
		    if (yyvsp[-1].card->ident_type) {
			yyerror("ID method already defined for '%s'", yyvsp[-1].card->name);
			YYERROR;
		    }
		    if (blank_card) {
			yyerror("Anonymous card already defined");
			YYERROR;
		    }
		    yyvsp[-1].card->ident_type = BLANK_IDENT;
		    blank_card = yyvsp[-1].card;
		}
break;
case 31:
#line 230 "yacc_config.y"
{
		    if (yyvsp[-6].card->ident_type) {
			yyerror("ID method already defined for '%s'", yyvsp[-6].card->name);
			YYERROR;
		    }
		    yyvsp[-6].card->ident_type = TUPLE_IDENT;
		    yyvsp[-6].card->id.tuple.code = yyvsp[-4].num;
		    yyvsp[-6].card->id.tuple.ofs = yyvsp[-2].num;
		    yyvsp[-6].card->id.tuple.info = yyvsp[0].str;
		}
break;
case 32:
#line 243 "yacc_config.y"
{
		    if (yyvsp[-4].card->ident_type & (EXCL_IDENT|MANFID_IDENT)) {
			yyerror("ID method already defined for '%s'", yyvsp[-4].card->name);
			YYERROR;
		    }
		    yyvsp[-4].card->ident_type |= MANFID_IDENT;
		    yyvsp[-4].card->manfid.manf = yyvsp[-2].num;
		    yyvsp[-4].card->manfid.card = yyvsp[0].num;
		}
break;
case 33:
#line 254 "yacc_config.y"
{
		    if (yyvsp[-4].card->ident_type) {
			yyerror("ID method already defined for '%s'", yyvsp[-4].card->name);
			YYERROR;
		    }
		    yyvsp[-4].card->ident_type = PCI_IDENT;
		    yyvsp[-4].card->manfid.manf = yyvsp[-2].num;
		    yyvsp[-4].card->manfid.card = yyvsp[0].num;
		}
break;
case 34:
#line 265 "yacc_config.y"
{
		    if (yyvsp[-2].card->ident_type & (EXCL_IDENT|VERS_1_IDENT)) {
			yyerror("ID method already defined for '%s'", yyvsp[-2].card->name);
			YYERROR;
		    }
		    yyvsp[-2].card->ident_type |= VERS_1_IDENT;
		    yyvsp[-2].card->id.vers.ns = 1;
		    yyvsp[-2].card->id.vers.pi[0] = yyvsp[0].str;
		}
break;
case 35:
#line 275 "yacc_config.y"
{
		    if (yyvsp[-2].card->id.vers.ns == 4) {
			yyerror("too many version strings for '%s'", yyvsp[-2].card->name);
			YYERROR;
		    }
		    yyvsp[-2].card->id.vers.pi[yyvsp[-2].card->id.vers.ns] = yyvsp[0].str;
		    yyvsp[-2].card->id.vers.ns++;
		}
break;
case 36:
#line 286 "yacc_config.y"
{
		    if (yyvsp[-2].card->ident_type) {
			yyerror("ID method already defined for '%s'", yyvsp[-2].card->name);
			YYERROR;
		    }
		    yyvsp[-2].card->ident_type = FUNC_IDENT;
		    yyvsp[-2].card->id.func.funcid = yyvsp[0].num;
		}
break;
case 37:
#line 297 "yacc_config.y"
{ yyvsp[-2].card->cis_file = strdup(yyvsp[0].str); }
break;
case 38:
#line 301 "yacc_config.y"
{
		    if (add_binding(yyvsp[-2].card, yyvsp[0].str, 0) != 0)
			YYERROR;
		}
break;
case 39:
#line 306 "yacc_config.y"
{
		    if (add_binding(yyvsp[-4].card, yyvsp[-2].str, yyvsp[0].num) != 0)
			YYERROR;
		}
break;
case 40:
#line 311 "yacc_config.y"
{
		    if (add_binding(yyvsp[-2].card, yyvsp[0].str, 0) != 0)
			YYERROR;
		}
break;
case 41:
#line 316 "yacc_config.y"
{
		    if (add_binding(yyvsp[-4].card, yyvsp[-2].str, yyvsp[0].num) != 0)
			YYERROR;
		}
break;
case 42:
#line 323 "yacc_config.y"
{
		    yyvsp[-1].device->needs_mtd = 1;
		}
break;
case 43:
#line 329 "yacc_config.y"
{
		    device_info_t *d;
		    int i, found = 0;
		    for (d = root_device; d; d = d->next) {
			for (i = 0; i < d->modules; i++)
			    if (strcmp(yyvsp[-2].str, d->module[i]) == 0) break;
			if (i < d->modules) {
			    if (d->opts[i])
				free(d->opts[i]);
			    d->opts[i] = strdup(yyvsp[0].str);
			    found = 1;
			}
		    }
		    free(yyvsp[-2].str); free(yyvsp[0].str);
		    if (!found) {
			yyerror("module name '%s' not found", yyvsp[-2].str);
			YYERROR;
		    }
		}
break;
case 44:
#line 351 "yacc_config.y"
{
		    if (add_module(yyvsp[-2].device, yyvsp[0].str) != 0)
			YYERROR;
		}
break;
case 45:
#line 356 "yacc_config.y"
{
		    if (yyvsp[-2].device->opts[yyvsp[-2].device->modules-1] == NULL) {
			yyvsp[-2].device->opts[yyvsp[-2].device->modules-1] = yyvsp[0].str;
		    } else {
			yyerror("too many module options for '%s'",
				yyvsp[-2].device->module[yyvsp[-2].device->modules-1]);
			YYERROR;
		    }
		}
break;
case 46:
#line 366 "yacc_config.y"
{
		    if (add_module(yyvsp[-2].device, yyvsp[0].str) != 0)
			YYERROR;
		}
break;
case 47:
#line 373 "yacc_config.y"
{
		    if (yyvsp[-2].device->class != NULL) {
			yyerror("extra class string '%s'", yyvsp[0].str);
			YYERROR;
		    }
		    yyvsp[-2].device->class = yyvsp[0].str;
		}
break;
case 48:
#line 383 "yacc_config.y"
{
		    yyval.mtd = calloc(sizeof(mtd_ident_t), 1);
		    yyval.mtd->refs = 1;
		    yyval.mtd->name = yyvsp[0].str;
		}
break;
case 52:
#line 394 "yacc_config.y"
{
		    if (yyvsp[-2].mtd->mtd_type) {
			yyerror("ID method already defined for '%s'", yyvsp[-2].mtd->name);
			YYERROR;
		    }
		    yyvsp[-2].mtd->mtd_type = DTYPE_MTD;
		    yyvsp[-2].mtd->dtype = yyvsp[0].num;
		}
break;
case 53:
#line 405 "yacc_config.y"
{
		    if (yyvsp[-3].mtd->mtd_type) {
			yyerror("ID method already defined for '%s'", yyvsp[-3].mtd->name);
			YYERROR;
		    }
		    yyvsp[-3].mtd->mtd_type = JEDEC_MTD;
		    yyvsp[-3].mtd->jedec_mfr = yyvsp[-1].num;
		    yyvsp[-3].mtd->jedec_info = yyvsp[0].num;
		}
break;
case 54:
#line 417 "yacc_config.y"
{
		    if (yyvsp[-1].mtd->mtd_type) {
			yyerror("ID method already defined for '%s'", yyvsp[-1].mtd->name);
			YYERROR;
		    }
		    if (default_mtd) {
			yyerror("Default MTD already defined");
			YYERROR;
		    }
		    yyvsp[-1].mtd->mtd_type = DEFAULT_MTD;
		    default_mtd = yyvsp[-1].mtd;
		}
break;
case 55:
#line 432 "yacc_config.y"
{
		    if (yyvsp[-2].mtd->module != NULL) {
			yyerror("extra MTD entry for '%s'", yyvsp[-2].mtd->name);
			YYERROR;
		    }
		    yyvsp[-2].mtd->module = yyvsp[0].str;
		}
break;
case 56:
#line 440 "yacc_config.y"
{
		    if (yyvsp[-2].mtd->opts == NULL) {
			yyvsp[-2].mtd->opts = yyvsp[0].str;
		    } else {
			yyerror("too many module options for '%s'", yyvsp[-2].mtd->module);
			YYERROR;
		    }
		}
break;
case 57:
#line 451 "yacc_config.y"
{
		    mtd_ident_t *m;
		    int found = 0;
		    for (m = root_mtd; m; m = m->next)
			if (strcmp(yyvsp[-2].str, m->module) == 0) break;
		    if (m) {
			if (m->opts) free(m->opts);
			m->opts = strdup(yyvsp[0].str);
			found = 1;
		    }
		    free(yyvsp[-2].str); free(yyvsp[0].str);
		    if (!found) {
			yyerror("MTD name '%s' not found", yyvsp[-2].str);
			YYERROR;
		    }
		}
break;
#line 1042 "y.tab.c"
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
    if (yyssp >= yysslim && yygrowstack())
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
