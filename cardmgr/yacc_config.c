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
#line 2 "yacc_config.y"
/*
 * yacc_config.y 1.42 1998/07/08 11:13:39
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

#line 54 "yacc_config.y"
typedef union {
    char *str;
    u_long num;
    struct device_info_t *device;
    struct card_info_t *card;
    struct mtd_ident_t *mtd;
    struct adjust_list_t *adjust;
} YYSTYPE;
#line 66 "y.tab.c"
#define DEVICE 257
#define CARD 258
#define ANONYMOUS 259
#define TUPLE 260
#define MANFID 261
#define VERSION 262
#define FUNCTION 263
#define BIND 264
#define CIS 265
#define TO 266
#define NEEDS_MTD 267
#define MODULE 268
#define OPTS 269
#define CLASS 270
#define REGION 271
#define JEDEC 272
#define DTYPE 273
#define DEFAULT 274
#define MTD 275
#define INCLUDE 276
#define EXCLUDE 277
#define RESERVE 278
#define IRQ_NO 279
#define PORT 280
#define MEMORY 281
#define STRING 282
#define NUMBER 283
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,    0,    0,    0,    0,    0,    1,    1,    1,
    1,    2,    2,    2,    3,    3,    3,    3,    7,    7,
    7,    7,    7,    7,    7,    7,    8,    9,   10,   11,
   11,   12,   14,   13,   13,   13,   13,    4,   20,    5,
    5,    5,    6,   15,   15,   15,   15,   15,   17,   16,
   18,   19,
};
short yylen[] = {                                         2,
    0,    2,    2,    2,    2,    2,    2,    2,    2,    2,
    3,    2,    4,    4,    2,    1,    1,    1,    2,    1,
    1,    1,    1,    1,    1,    1,    2,    7,    5,    3,
    3,    3,    3,    3,    5,    3,    5,    2,    4,    3,
    3,    3,    3,    2,    1,    1,    1,    1,    3,    4,
    2,    3,
};
short yydefred[] = {                                      1,
    0,    7,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   16,    0,   18,    0,   20,   21,   22,    0,   24,
    0,   26,    0,   46,   45,   47,    0,    6,   15,   19,
    0,   44,    0,    0,    0,    8,    9,   10,    0,   38,
    0,    0,    0,    0,   27,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   51,    0,    0,   12,    0,
    0,   11,   40,   43,   41,   42,    0,    0,   30,   32,
    0,   33,   31,    0,    0,   49,   52,   39,    0,    0,
    0,    0,    0,    0,   50,   13,   14,    0,   29,   35,
   37,    0,   28,
};
short yydgoto[] = {                                       1,
   10,   36,   11,   12,   13,   14,   15,   16,   17,   18,
   19,   20,   21,   22,   23,   24,   25,   26,   27,   28,
};
short yysindex[] = {                                      0,
 -251,    0, -278, -274, -266, -260, -245, -245, -245,  -21,
 -249,    0,  -44,    0, -250,    0,    0,    0,  -12,    0,
   -7,    0, -244,    0,    0,    0,    0,    0,    0,    0,
 -231,    0, -243, -242, -241,    0,    0,    0, -245,    0,
 -239, -238, -236, -234,    0, -233, -232, -230, -229, -227,
 -226, -225, -224, -223, -222,    0, -220, -219,    0,    4,
    8,    0,    0,    0,    0,    0,   -5,   15,    0,    0,
 -202,    0,    0, -201, -217,    0,    0,    0, -216, -214,
 -212, -211, -210, -209,    0,    0,    0,   31,    0,    0,
    0, -206,    0,
};
short yyrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  113,
  139,    0,  109,    0,  136,    0,    0,    0,   47,    0,
   70,    0,    0,    0,    0,    0,   86,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    1,    0,    0,   24,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,
};
short yygindex[] = {                                      0,
    0,   -6,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};
#define YYTABLESIZE 417
short yytable[] = {                                      44,
   34,   37,   38,   29,    2,    3,    4,   30,   45,   46,
   47,   48,   49,   50,   51,   31,    5,   40,   41,    6,
   42,   32,   39,   36,    7,    8,    9,   54,   55,   56,
   57,   52,   62,   33,   34,   35,   53,   58,   81,   59,
   60,   61,   63,   64,   34,   65,   23,   66,   79,   67,
   68,   69,   80,   70,   71,   72,   73,   74,   82,   75,
   76,   77,   78,   83,   84,   85,   86,   36,   87,   25,
   88,   89,   90,   91,   92,   93,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    4,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   17,    0,
    0,    0,    2,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    5,    0,    0,    3,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   43,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   34,   34,   34,   34,
   34,   34,   34,   34,   34,   34,    0,    0,   34,    0,
    0,   34,    0,    0,    0,    0,   34,   34,   34,   36,
   36,   36,   36,   36,   36,   36,   36,   36,   36,    0,
    0,   36,    0,    0,   36,    0,    0,    0,    0,   36,
   36,   36,   23,   23,   23,   23,   23,   23,   23,   23,
   23,   23,    0,    0,   23,    0,    0,   23,    0,    0,
    0,    0,   23,   23,   23,   25,   25,   25,   25,   25,
   25,   25,   25,   25,   25,    0,    0,   25,    0,    0,
   25,    4,    4,    4,    0,   25,   25,   25,    0,    0,
    0,    0,    0,    4,    0,    0,    4,   48,   48,   48,
   48,    4,    4,    4,   17,   17,   17,    0,    2,    2,
    2,    0,    0,    0,    0,   17,   17,    0,   17,   17,
    2,    0,    0,    2,   17,   17,   17,    0,    2,    2,
    2,    5,    5,    5,    3,    3,    3,    0,    0,    0,
    0,    0,    0,    5,    0,    0,    5,    0,    0,    3,
    0,    5,    5,    5,    3,    3,    3,
};
short yycheck[] = {                                      44,
    0,    8,    9,  282,  256,  257,  258,  282,  259,  260,
  261,  262,  263,  264,  265,  282,  268,  267,  268,  271,
  270,  282,   44,    0,  276,  277,  278,  272,  273,  274,
  275,   44,   39,  279,  280,  281,   44,  269,   44,  283,
  283,  283,  282,  282,   44,  282,    0,  282,   45,  283,
  283,  282,   45,  283,  282,  282,  282,  282,   44,  283,
  283,  282,  282,  266,  266,  283,  283,   44,  283,    0,
  283,  283,  283,  283,   44,  282,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,    0,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,    0,   -1,
   -1,   -1,    0,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,    0,   -1,   -1,    0,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  269,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  256,  257,  258,  259,
  260,  261,  262,  263,  264,  265,   -1,   -1,  268,   -1,
   -1,  271,   -1,   -1,   -1,   -1,  276,  277,  278,  256,
  257,  258,  259,  260,  261,  262,  263,  264,  265,   -1,
   -1,  268,   -1,   -1,  271,   -1,   -1,   -1,   -1,  276,
  277,  278,  256,  257,  258,  259,  260,  261,  262,  263,
  264,  265,   -1,   -1,  268,   -1,   -1,  271,   -1,   -1,
   -1,   -1,  276,  277,  278,  256,  257,  258,  259,  260,
  261,  262,  263,  264,  265,   -1,   -1,  268,   -1,   -1,
  271,  256,  257,  258,   -1,  276,  277,  278,   -1,   -1,
   -1,   -1,   -1,  268,   -1,   -1,  271,  272,  273,  274,
  275,  276,  277,  278,  256,  257,  258,   -1,  256,  257,
  258,   -1,   -1,   -1,   -1,  267,  268,   -1,  270,  271,
  268,   -1,   -1,  271,  276,  277,  278,   -1,  276,  277,
  278,  256,  257,  258,  256,  257,  258,   -1,   -1,   -1,
   -1,   -1,   -1,  268,   -1,   -1,  271,   -1,   -1,  271,
   -1,  276,  277,  278,  276,  277,  278,
};
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 283
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,"','","'-'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"DEVICE","CARD",
"ANONYMOUS","TUPLE","MANFID","VERSION","FUNCTION","BIND","CIS","TO","NEEDS_MTD",
"MODULE","OPTS","CLASS","REGION","JEDEC","DTYPE","DEFAULT","MTD","INCLUDE",
"EXCLUDE","RESERVE","IRQ_NO","PORT","MEMORY","STRING","NUMBER",
};
char *yyrule[] = {
"$accept : list",
"list :",
"list : list adjust",
"list : list device",
"list : list mtd",
"list : list card",
"list : list opts",
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
"card : version",
"card : function",
"card : bind",
"card : cis",
"anonymous : card ANONYMOUS",
"tuple : card TUPLE NUMBER ',' NUMBER ',' STRING",
"manfid : card MANFID NUMBER ',' NUMBER",
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
"region : mtd",
"dtype : region DTYPE NUMBER",
"jedec : region JEDEC NUMBER NUMBER",
"default : region DEFAULT",
"mtd : region MTD STRING",
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
#line 414 "yacc_config.y"
void yyerror(char *msg, ...)
{
     va_list ap;
     char str[256];

     va_start(ap, msg);
     sprintf(str, "config error, file '%s' line %d: ",
	     current_file, current_lineno);
     vsprintf(str+strlen(str), msg, ap);
#if YYDEBUG
     fprintf(stderr, "%s\n", str);
#else
     syslog(LOG_INFO, "%s", str);
#endif
     va_end(ap);
}

static int add_binding(card_info_t *card, char *name, int fn)
{
    device_info_t *dev = root_device;
    if (card->functions == MAX_FUNCTIONS) {
	yyerror("too many bindings\n");
	return -1;
    }
    for (; dev; dev = dev->next)
	if (strcmp((char *)dev->dev_info, name) == 0) break;
    if (dev == NULL) {
	yyerror("unknown device: %s", name);
	return -1;
    }
    card->device[card->functions] = dev;
    card->dev_fn[card->functions] = fn;
    card->functions++;
    free(name);
    return 0;
}

static int add_module(device_info_t *dev, char *name)
{
    if (dev->modules == MAX_MODULES) {
	yyerror("too many modules");
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
#line 403 "y.tab.c"
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
case 2:
#line 73 "yacc_config.y"
{
		    adjust_list_t **tail = &root_adjust;
		    while (*tail != NULL) tail = &(*tail)->next;
		    *tail = yyvsp[0].adjust;
		}
break;
case 3:
#line 79 "yacc_config.y"
{
		    yyvsp[0].device->next = root_device;
		    root_device = yyvsp[0].device;
		}
break;
case 4:
#line 84 "yacc_config.y"
{
		    if (yyvsp[0].mtd->mtd_type == 0) {
			yyerror("no ID method for this card");
			YYERROR;
		    }
		    if (yyvsp[0].mtd->module == NULL) {
			yyerror("no MTD module specified");
			YYERROR;
		    }
		    yyvsp[0].mtd->next = root_mtd;
		    root_mtd = yyvsp[0].mtd;
		}
break;
case 5:
#line 97 "yacc_config.y"
{
		    if (yyvsp[0].card->ident_type == 0) {
			yyerror("no ID method for this card");
			YYERROR;
		    }
		    if (yyvsp[0].card->functions == 0) {
			yyerror("no function bindings");
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
case 8:
#line 119 "yacc_config.y"
{
		    yyvsp[0].adjust->adj.Action = ADD_MANAGED_RESOURCE;
		    yyval.adjust = yyvsp[0].adjust;
		}
break;
case 9:
#line 124 "yacc_config.y"
{
		    yyvsp[0].adjust->adj.Action = REMOVE_MANAGED_RESOURCE;
		    yyval.adjust = yyvsp[0].adjust;
		}
break;
case 10:
#line 129 "yacc_config.y"
{
		    yyvsp[0].adjust->adj.Action = ADD_MANAGED_RESOURCE;
		    yyvsp[0].adjust->adj.Attributes |= RES_RESERVED;
		    yyval.adjust = yyvsp[0].adjust;
		}
break;
case 11:
#line 135 "yacc_config.y"
{
		    yyvsp[0].adjust->adj.Action = yyvsp[-2].adjust->adj.Action;
		    yyvsp[0].adjust->adj.Attributes = yyvsp[-2].adjust->adj.Attributes;
		    yyvsp[0].adjust->next = yyvsp[-2].adjust;
		    yyval.adjust = yyvsp[0].adjust;
		}
break;
case 12:
#line 144 "yacc_config.y"
{
		    yyval.adjust = calloc(sizeof(adjust_list_t), 1);
		    yyval.adjust->adj.Resource = RES_IRQ;
		    yyval.adjust->adj.resource.irq.IRQ = yyvsp[0].num;
		}
break;
case 13:
#line 150 "yacc_config.y"
{
		    if ((yyvsp[0].num < yyvsp[-2].num) || (yyvsp[0].num > 0xffff)) {
			yyerror("invalid port range");
			YYERROR;
		    }
		    yyval.adjust = calloc(sizeof(adjust_list_t), 1);
		    yyval.adjust->adj.Resource = RES_IO_RANGE;
		    yyval.adjust->adj.resource.io.BasePort = yyvsp[-2].num;
		    yyval.adjust->adj.resource.io.NumPorts = yyvsp[0].num - yyvsp[-2].num + 1;
		}
break;
case 14:
#line 161 "yacc_config.y"
{
		    if (yyvsp[0].num < yyvsp[-2].num) {
			yyerror("invalid address range");
			YYERROR;
		    }
		    yyval.adjust = calloc(sizeof(adjust_list_t), 1);
		    yyval.adjust->adj.Resource = RES_MEMORY_RANGE;
		    yyval.adjust->adj.resource.memory.Base = yyvsp[-2].num;
		    yyval.adjust->adj.resource.memory.Size = yyvsp[0].num - yyvsp[-2].num + 1;
		}
break;
case 15:
#line 174 "yacc_config.y"
{
		    yyval.device = calloc(sizeof(device_info_t), 1);
		    strcpy(yyval.device->dev_info, yyvsp[0].str);
		    free(yyvsp[0].str);
		}
break;
case 19:
#line 185 "yacc_config.y"
{
		    yyval.card = calloc(sizeof(card_info_t), 1);
		    yyval.card->name = yyvsp[0].str;
		}
break;
case 27:
#line 199 "yacc_config.y"
{
		    if (yyvsp[-1].card->ident_type != 0) {
			yyerror("ID method already defined");
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
case 28:
#line 214 "yacc_config.y"
{
		    if (yyvsp[-6].card->ident_type != 0) {
			yyerror("ID method already defined");
			YYERROR;
		    }
		    yyvsp[-6].card->ident_type = TUPLE_IDENT;
		    yyvsp[-6].card->id.tuple.code = yyvsp[-4].num;
		    yyvsp[-6].card->id.tuple.ofs = yyvsp[-2].num;
		    yyvsp[-6].card->id.tuple.info = yyvsp[0].str;
		}
break;
case 29:
#line 227 "yacc_config.y"
{
		    if (yyvsp[-4].card->ident_type != 0) {
			yyerror("ID method already defined");
			YYERROR;
		    }
		    yyvsp[-4].card->ident_type = MANFID_IDENT;
		    yyvsp[-4].card->id.manfid.manf = yyvsp[-2].num;
		    yyvsp[-4].card->id.manfid.card = yyvsp[0].num;
		}
break;
case 30:
#line 238 "yacc_config.y"
{
		    if (yyvsp[-2].card->ident_type != 0) {
			yyerror("ID method already defined\n");
			YYERROR;
		    }
		    yyvsp[-2].card->ident_type = VERS_1_IDENT;
		    yyvsp[-2].card->id.vers.ns = 1;
		    yyvsp[-2].card->id.vers.pi[0] = yyvsp[0].str;
		}
break;
case 31:
#line 248 "yacc_config.y"
{
		    if (yyvsp[-2].card->id.vers.ns == 4) {
			yyerror("too many version strings");
			YYERROR;
		    }
		    yyvsp[-2].card->id.vers.pi[yyvsp[-2].card->id.vers.ns] = yyvsp[0].str;
		    yyvsp[-2].card->id.vers.ns++;
		}
break;
case 32:
#line 259 "yacc_config.y"
{
		    if (yyvsp[-2].card->ident_type != 0) {
			yyerror("ID method already defined\n");
			YYERROR;
		    }
		    yyvsp[-2].card->ident_type = FUNC_IDENT;
		    yyvsp[-2].card->id.func.funcid = yyvsp[0].num;
		}
break;
case 33:
#line 270 "yacc_config.y"
{ yyvsp[-2].card->cis_file = strdup(yyvsp[0].str); }
break;
case 34:
#line 274 "yacc_config.y"
{
		    if (add_binding(yyvsp[-2].card, yyvsp[0].str, 0) != 0)
			YYERROR;
		}
break;
case 35:
#line 279 "yacc_config.y"
{
		    if (add_binding(yyvsp[-4].card, yyvsp[-2].str, yyvsp[0].num) != 0)
			YYERROR;
		}
break;
case 36:
#line 284 "yacc_config.y"
{
		    if (add_binding(yyvsp[-2].card, yyvsp[0].str, 0) != 0)
			YYERROR;
		}
break;
case 37:
#line 289 "yacc_config.y"
{
		    if (add_binding(yyvsp[-4].card, yyvsp[-2].str, yyvsp[0].num) != 0)
			YYERROR;
		}
break;
case 38:
#line 296 "yacc_config.y"
{
		    yyvsp[-1].device->needs_mtd = 1;
		}
break;
case 39:
#line 302 "yacc_config.y"
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
			yyerror("module name not found!");
			YYERROR;
		    }
		}
break;
case 40:
#line 324 "yacc_config.y"
{
		    if (add_module(yyvsp[-2].device, yyvsp[0].str) != 0)
			YYERROR;
		}
break;
case 41:
#line 329 "yacc_config.y"
{
		    if (yyvsp[-2].device->opts[yyvsp[-2].device->modules-1] == NULL)
			yyvsp[-2].device->opts[yyvsp[-2].device->modules-1] = yyvsp[0].str;
		    else {
			yyerror("too many options");
			YYERROR;
		    }
		}
break;
case 42:
#line 338 "yacc_config.y"
{
		    if (add_module(yyvsp[-2].device, yyvsp[0].str) != 0)
			YYERROR;
		}
break;
case 43:
#line 345 "yacc_config.y"
{
		    if (yyvsp[-2].device->class != NULL) {
			yyerror("extra class string");
			YYERROR;
		    }
		    yyvsp[-2].device->class = yyvsp[0].str;
		}
break;
case 44:
#line 355 "yacc_config.y"
{
		    yyval.mtd = calloc(sizeof(mtd_ident_t), 1);
		    yyval.mtd->name = yyvsp[0].str;
		}
break;
case 49:
#line 366 "yacc_config.y"
{
		    if (yyvsp[-2].mtd->mtd_type != 0) {
			yyerror("ID method already defined");
			YYERROR;
		    }
		    yyvsp[-2].mtd->mtd_type = DTYPE_MTD;
		    yyvsp[-2].mtd->dtype = yyvsp[0].num;
		}
break;
case 50:
#line 377 "yacc_config.y"
{
		    if (yyvsp[-3].mtd->mtd_type != 0) {
			yyerror("ID method already defined");
			YYERROR;
		    }
		    yyvsp[-3].mtd->mtd_type = JEDEC_MTD;
		    yyvsp[-3].mtd->jedec_mfr = yyvsp[-1].num;
		    yyvsp[-3].mtd->jedec_info = yyvsp[0].num;
		}
break;
case 51:
#line 389 "yacc_config.y"
{
		    if (yyvsp[-1].mtd->mtd_type != 0) {
			yyerror("ID method already defined");
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
case 52:
#line 404 "yacc_config.y"
{
		    if (yyvsp[-2].mtd->module != NULL) {
			yyerror("extra MTD entry");
			YYERROR;
		    }
		    yyvsp[-2].mtd->module = yyvsp[0].str;
		}
break;
#line 898 "y.tab.c"
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
