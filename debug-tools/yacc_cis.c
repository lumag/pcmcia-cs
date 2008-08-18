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
 * yacc_cis.y 1.4 1998/07/17 17:11:47
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

#line 52 "yacc_cis.y"
typedef union {
    char *str;
    u_long num;
    float flt;
    cistpl_power_t pwr;
    cisparse_t *parse;
    tuple_info_t *tuple;
} YYSTYPE;
#line 61 "y.tab.c"
#define STRING 257
#define NUMBER 258
#define FLOAT 259
#define VERS_1 260
#define MANFID 261
#define FUNCID 262
#define CONFIG 263
#define CFTABLE 264
#define MFC 265
#define POST 266
#define ROM 267
#define VNOM 268
#define VMIN 269
#define VMAX 270
#define ISTATIC 271
#define IAVG 272
#define IMAX 273
#define IDOWN 274
#define VCC 275
#define VPP1 276
#define VPP2 277
#define IO 278
#define MEM 279
#define DEFAULT 280
#define BVD 281
#define WP 282
#define RDYBSY 283
#define MWAIT 284
#define AUDIO 285
#define READONLY 286
#define PWRDOWN 287
#define BIT8 288
#define BIT16 289
#define IRQ_NO 290
#define MASK 291
#define LEVEL 292
#define PULSE 293
#define SHARED 294
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,   11,   11,   12,   12,   10,   10,   10,   10,
   10,    3,    3,    4,    5,    5,    5,    6,    1,    1,
    1,    1,    1,    1,    1,    2,    2,    8,    8,    8,
    8,    9,    9,    9,    9,    9,    7,    7,    7,    7,
    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,
};
short yylen[] = {                                         2,
    1,    2,    0,    2,    4,    5,    1,    1,    1,    1,
    1,    2,    3,    4,    2,    2,    2,    6,    2,    2,
    2,    2,    2,    2,    2,    0,    2,    5,    5,    2,
    2,    3,    4,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    2,    2,    3,    3,    3,    1,    1,
};
short yydefred[] = {                                      3,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    8,
    0,   10,    0,    0,    0,    4,    0,   12,    0,   15,
    0,   37,    3,    0,   16,   17,   26,   26,   26,    0,
   38,   39,   40,   41,   42,   43,   44,   45,    0,   30,
   31,    0,   35,   34,   36,    0,    0,    0,    0,   13,
    0,    0,    0,    0,   32,    0,    0,    3,   14,    0,
    5,    0,    0,    0,    0,    0,    0,    0,   27,    0,
   33,    0,    0,    0,   19,   20,   21,   22,   23,   24,
   25,   28,   29,    6,   18,
};
short yydgoto[] = {                                       1,
   69,   51,    9,   10,   11,   12,   13,   14,   15,   16,
    2,   17,
};
short yysindex[] = {                                      0,
    0, -220, -246, -221, -212, -211, -210,  -74,    6,    0,
 -261,    0, -266,  -44, -259,    0,    8,    0,    9,    0,
   10,    0,    0, -206,    0,    0,    0,    0,    0, -203,
    0,    0,    0,    0,    0,    0,    0,    0, -255,    0,
    0, -202,    0,    0,    0,  -66, -200, -199, -123,    0,
 -243, -243, -243,   15,    0, -197,   17,    0,    0,   20,
    0, -194, -193, -192, -191, -189, -188, -187,    0, -185,
    0, -184, -117, -183,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,
};
short yyrindex[] = {                                      0,
    0,   76,    0,    0,    0,    0,    0,    0,    7,    0,
   38,    0,   69,    1,   32,    0,   77,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   63,   94,  125,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,
};
short yygindex[] = {                                      0,
    0,   -6,    0,    0,    0,    0,    0,    0,    0,    0,
  -19,    0,
};
#define YYTABLESIZE 415
short yytable[] = {                                      42,
   49,   61,   55,   49,   25,   26,    7,   84,   27,   28,
   29,   30,   18,   31,   32,   33,   34,   35,   36,   37,
   38,   52,   53,   39,   62,   63,   64,   65,   66,   67,
   68,   50,   43,   44,   45,   56,   19,    9,   73,    3,
    4,    5,    6,    7,    8,   20,   21,   22,   23,   24,
   50,   46,   47,   48,   54,   57,   58,   59,   60,   70,
   71,   72,   46,   74,   75,   76,   77,   78,   11,   79,
   80,   81,   82,   83,   85,    1,    2,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   47,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   48,   49,    0,    0,    0,    0,
    0,    7,    0,    0,    0,    0,    3,    4,    5,    6,
    7,    0,    3,    4,    5,    6,    7,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   50,    0,    0,    0,
    0,    0,    9,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   46,    0,    0,
    0,    0,    0,   11,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   47,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   40,   41,    0,    0,    0,    0,   48,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   49,   49,   49,   49,   49,   49,    7,    7,    7,    7,
    7,    7,    0,    0,    0,   49,   49,   49,   49,    0,
   49,   49,   49,   49,   49,   49,   49,   49,    0,    0,
   49,   50,   50,   50,   50,   50,   50,    9,    9,    9,
    9,    9,    9,    0,    0,    0,   50,   50,   50,   50,
    0,   50,   50,   50,   50,   50,   50,   50,   50,    0,
    0,   50,   46,   46,   46,   46,   46,   46,   11,   11,
   11,   11,   11,   11,    0,    0,    0,   46,   46,   46,
   46,    0,   46,   46,   46,   46,   46,   46,   46,   46,
    0,    0,   46,   47,   47,   47,   47,   47,   47,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   47,   47,
   47,   47,    0,   47,   47,   47,   47,   47,   47,   47,
   47,    0,    0,   47,   48,   48,   48,   48,   48,   48,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   48,
   48,   48,   48,    0,   48,   48,   48,   48,   48,   48,
   48,   48,    0,    0,   48,
};
short yycheck[] = {                                      44,
    0,  125,  258,   23,  266,  267,    0,  125,  275,  276,
  277,  278,  259,  280,  281,  282,  283,  284,  285,  286,
  287,   28,   29,  290,  268,  269,  270,  271,  272,  273,
  274,    0,  292,  293,  294,  291,  258,    0,   58,  260,
  261,  262,  263,  264,  265,  258,  258,  258,  123,   44,
  257,   44,   44,   44,  258,  258,  123,  258,  258,   45,
  258,   45,    0,   44,  259,  259,  259,  259,    0,  259,
  259,  259,  258,  258,  258,    0,    0,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,    0,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,    0,  125,   -1,   -1,   -1,   -1,
   -1,  125,   -1,   -1,   -1,   -1,  260,  261,  262,  263,
  264,   -1,  260,  261,  262,  263,  264,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  125,   -1,   -1,   -1,
   -1,   -1,  125,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  125,   -1,   -1,
   -1,   -1,   -1,  125,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  125,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  288,  289,   -1,   -1,   -1,   -1,  125,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  260,  261,  262,  263,  264,  265,  260,  261,  262,  263,
  264,  265,   -1,   -1,   -1,  275,  276,  277,  278,   -1,
  280,  281,  282,  283,  284,  285,  286,  287,   -1,   -1,
  290,  260,  261,  262,  263,  264,  265,  260,  261,  262,
  263,  264,  265,   -1,   -1,   -1,  275,  276,  277,  278,
   -1,  280,  281,  282,  283,  284,  285,  286,  287,   -1,
   -1,  290,  260,  261,  262,  263,  264,  265,  260,  261,
  262,  263,  264,  265,   -1,   -1,   -1,  275,  276,  277,
  278,   -1,  280,  281,  282,  283,  284,  285,  286,  287,
   -1,   -1,  290,  260,  261,  262,  263,  264,  265,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  275,  276,
  277,  278,   -1,  280,  281,  282,  283,  284,  285,  286,
  287,   -1,   -1,  290,  260,  261,  262,  263,  264,  265,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  275,
  276,  277,  278,   -1,  280,  281,  282,  283,  284,  285,
  286,  287,   -1,   -1,  290,
};
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 294
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,"','","'-'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,"'{'",0,"'}'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"STRING","NUMBER",
"FLOAT","VERS_1","MANFID","FUNCID","CONFIG","CFTABLE","MFC","POST","ROM","VNOM",
"VMIN","VMAX","ISTATIC","IAVG","IMAX","IDOWN","VCC","VPP1","VPP2","IO","MEM",
"DEFAULT","BVD","WP","RDYBSY","MWAIT","AUDIO","READONLY","PWRDOWN","BIT8",
"BIT16","IRQ_NO","MASK","LEVEL","PULSE","SHARED",
};
char *yyrule[] = {
"$accept : cis",
"cis : chain",
"cis : chain mfc",
"chain :",
"chain : chain tuple",
"mfc : MFC '{' chain '}'",
"mfc : mfc ',' '{' chain '}'",
"tuple : vers_1",
"tuple : manfid",
"tuple : funcid",
"tuple : config",
"tuple : cftab",
"vers_1 : VERS_1 FLOAT",
"vers_1 : vers_1 ',' STRING",
"manfid : MANFID NUMBER ',' NUMBER",
"funcid : FUNCID NUMBER",
"funcid : funcid POST",
"funcid : funcid ROM",
"config : CONFIG NUMBER ',' NUMBER ',' NUMBER",
"pwr : VNOM FLOAT",
"pwr : VMIN FLOAT",
"pwr : VMAX FLOAT",
"pwr : ISTATIC FLOAT",
"pwr : IAVG FLOAT",
"pwr : IMAX FLOAT",
"pwr : IDOWN FLOAT",
"pwrlist :",
"pwrlist : pwrlist pwr",
"io : cftab IO NUMBER '-' NUMBER",
"io : io ',' NUMBER '-' NUMBER",
"io : io BIT8",
"io : io BIT16",
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
"cftab : irq",
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
#line 271 "yacc_cis.y"

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
#line 369 "y.tab.c"
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
#line 70 "yacc_cis.y"
{ cis_root = yyvsp[0].tuple; }
break;
case 2:
#line 72 "yacc_cis.y"
{ cis_root = yyvsp[-1].tuple; }
break;
case 3:
#line 76 "yacc_cis.y"
{ yyval.tuple = NULL; }
break;
case 4:
#line 78 "yacc_cis.y"
{
		    if (yyvsp[-1].tuple == NULL)
			yyval.tuple = yyvsp[0].tuple;
		    else {
			tuple_info_t *tail = yyvsp[-1].tuple;
			while (tail->next != NULL) tail = tail->next;
			tail->next = yyvsp[0].tuple;
			yyval.tuple = yyvsp[-1].tuple;
		    }
		}
break;
case 5:
#line 91 "yacc_cis.y"
{ mfc[nf++] = yyvsp[-1].tuple; }
break;
case 6:
#line 93 "yacc_cis.y"
{ mfc[nf++] = yyvsp[-1].tuple; }
break;
case 7:
#line 97 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_VERS_1, yyvsp[0].parse); }
break;
case 8:
#line 99 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_MANFID, yyvsp[0].parse); }
break;
case 9:
#line 101 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_FUNCID, yyvsp[0].parse); }
break;
case 10:
#line 103 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_CONFIG, yyvsp[0].parse); }
break;
case 11:
#line 105 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_CFTABLE_ENTRY, yyvsp[0].parse); }
break;
case 12:
#line 109 "yacc_cis.y"
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
case 13:
#line 119 "yacc_cis.y"
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
case 14:
#line 133 "yacc_cis.y"
{
		    yyval.parse = calloc(1, sizeof(cisparse_t));
		    yyval.parse->manfid.manf = yyvsp[-2].num;
		    yyval.parse->manfid.card = yyvsp[0].num;
		}
break;
case 15:
#line 141 "yacc_cis.y"
{
		    yyval.parse = calloc(1, sizeof(cisparse_t));
		    yyval.parse->funcid.func = yyvsp[0].num;
		}
break;
case 16:
#line 146 "yacc_cis.y"
{ yyval.parse->funcid.sysinit |= CISTPL_SYSINIT_POST; }
break;
case 17:
#line 148 "yacc_cis.y"
{ yyval.parse->funcid.sysinit |= CISTPL_SYSINIT_ROM; }
break;
case 18:
#line 152 "yacc_cis.y"
{
		    yyval.parse = calloc(1, sizeof(cisparse_t));
		    yyval.parse->config.last_idx = yyvsp[-4].num;
		    yyval.parse->config.base = yyvsp[-2].num;
		    yyval.parse->config.rmask[0] = yyvsp[0].num;
		}
break;
case 19:
#line 161 "yacc_cis.y"
{
		    yyval.pwr.present |= 1<<CISTPL_POWER_VNOM;
		    yyval.pwr.param[CISTPL_POWER_VNOM] = yyvsp[0].flt * 100000;
		}
break;
case 20:
#line 166 "yacc_cis.y"
{
		    yyval.pwr.present |= 1<<CISTPL_POWER_VMIN;
		    yyval.pwr.param[CISTPL_POWER_VMIN] = yyvsp[0].flt * 100000;
		}
break;
case 21:
#line 171 "yacc_cis.y"
{
		    yyval.pwr.present |= 1<<CISTPL_POWER_VMAX;
		    yyval.pwr.param[CISTPL_POWER_VMAX] = yyvsp[0].flt * 100000;
		}
break;
case 22:
#line 176 "yacc_cis.y"
{
		    yyval.pwr.present |= 1<<CISTPL_POWER_ISTATIC;
		    yyval.pwr.param[CISTPL_POWER_ISTATIC] = yyvsp[0].flt * 1000;
		}
break;
case 23:
#line 181 "yacc_cis.y"
{
		    yyval.pwr.present |= 1<<CISTPL_POWER_IAVG;
		    yyval.pwr.param[CISTPL_POWER_IAVG] = yyvsp[0].flt * 1000;
		}
break;
case 24:
#line 186 "yacc_cis.y"
{
		    yyval.pwr.present |= 1<<CISTPL_POWER_IPEAK;
		    yyval.pwr.param[CISTPL_POWER_IPEAK] = yyvsp[0].flt * 1000;
		}
break;
case 25:
#line 191 "yacc_cis.y"
{
		    yyval.pwr.present |= 1<<CISTPL_POWER_IDOWN;
		    yyval.pwr.param[CISTPL_POWER_IDOWN] = yyvsp[0].flt * 1000;
		}
break;
case 26:
#line 198 "yacc_cis.y"
{
		    yyval.pwr.present = 0;
		}
break;
case 28:
#line 205 "yacc_cis.y"
{
		    int n = yyval.parse->cftable_entry.io.nwin;
		    yyval.parse->cftable_entry.io.win[n].base = yyvsp[-2].num;
		    yyval.parse->cftable_entry.io.win[n].len = yyvsp[0].num-yyvsp[-2].num+1;
		    yyval.parse->cftable_entry.io.nwin++;
		}
break;
case 29:
#line 212 "yacc_cis.y"
{
		    int n = yyval.parse->cftable_entry.io.nwin;
		    yyval.parse->cftable_entry.io.win[n].base = yyvsp[-2].num;
		    yyval.parse->cftable_entry.io.win[n].len = yyvsp[0].num-yyvsp[-2].num+1;
		    yyval.parse->cftable_entry.io.nwin++;
		}
break;
case 30:
#line 219 "yacc_cis.y"
{ yyval.parse->cftable_entry.io.flags |= CISTPL_IO_8BIT; }
break;
case 31:
#line 221 "yacc_cis.y"
{ yyval.parse->cftable_entry.io.flags |= CISTPL_IO_16BIT; }
break;
case 32:
#line 225 "yacc_cis.y"
{ yyval.parse->cftable_entry.irq.IRQInfo1 = (yyvsp[0].num & 0x0f); }
break;
case 33:
#line 227 "yacc_cis.y"
{
		    yyval.parse->cftable_entry.irq.IRQInfo1 = IRQ_INFO2_VALID;
		    yyval.parse->cftable_entry.irq.IRQInfo2 = yyvsp[0].num;
		}
break;
case 34:
#line 232 "yacc_cis.y"
{ yyval.parse->cftable_entry.irq.IRQInfo1 |= IRQ_PULSE_ID; }
break;
case 35:
#line 234 "yacc_cis.y"
{ yyval.parse->cftable_entry.irq.IRQInfo1 |= IRQ_LEVEL_ID; }
break;
case 36:
#line 236 "yacc_cis.y"
{ yyval.parse->cftable_entry.irq.IRQInfo1 |= IRQ_SHARE_ID; }
break;
case 37:
#line 240 "yacc_cis.y"
{
		    yyval.parse = calloc(1, sizeof(cisparse_t));
		    yyval.parse->cftable_entry.index = yyvsp[0].num;
		}
break;
case 38:
#line 245 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_DEFAULT; }
break;
case 39:
#line 247 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_BVDS; }
break;
case 40:
#line 249 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_WP; }
break;
case 41:
#line 251 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_RDYBSY; }
break;
case 42:
#line 253 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_MWAIT; }
break;
case 43:
#line 255 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_AUDIO; }
break;
case 44:
#line 257 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_READONLY; }
break;
case 45:
#line 259 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_PWRDOWN; }
break;
case 46:
#line 261 "yacc_cis.y"
{ yyval.parse->cftable_entry.vcc = yyvsp[0].pwr; }
break;
case 47:
#line 263 "yacc_cis.y"
{ yyval.parse->cftable_entry.vpp1 = yyvsp[0].pwr; }
break;
case 48:
#line 265 "yacc_cis.y"
{ yyval.parse->cftable_entry.vpp2 = yyvsp[0].pwr; }
break;
#line 776 "y.tab.c"
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
