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
#line 43 "yacc_cis.y"
typedef union {
    char *str;
    u_long num;
    float flt;
    cistpl_power_t pwr;
    cisparse_t *parse;
    tuple_info_t *tuple;
} YYSTYPE;
#line 52 "y.tab.c"
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
#define IRQ 290
#define MASK 291
#define LEVEL 292
#define PULSE 293
#define SHARED 294
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,   11,   11,   12,   12,   10,   10,   10,   10,
   10,    3,    3,    4,    5,    5,    5,    6,    1,    1,
    1,    2,    2,    8,    8,    8,    8,    9,    9,    9,
    9,    9,    7,    7,    7,    7,    7,    7,    7,    7,
    7,    7,    7,    7,    7,    7,
};
short yylen[] = {                                         2,
    1,    2,    0,    2,    4,    5,    1,    1,    1,    1,
    1,    2,    3,    4,    2,    2,    2,    6,    2,    2,
    2,    0,    2,    5,    5,    2,    2,    3,    4,    2,
    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,
    2,    3,    3,    3,    1,    1,
};
short yydefred[] = {                                      3,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    8,
    0,   10,    0,    0,    0,    4,    0,   12,    0,   15,
    0,   33,    3,    0,   16,   17,   22,   22,   22,    0,
   34,   35,   36,   37,   38,   39,   40,   41,    0,   26,
   27,    0,   31,   30,   32,    0,    0,    0,    0,   13,
    0,    0,    0,    0,   28,    0,    0,    3,   14,    0,
    5,    0,    0,    0,   23,    0,   29,    0,    0,    0,
   19,   20,   21,   24,   25,    6,   18,
};
short yydgoto[] = {                                       1,
   65,   51,    9,   10,   11,   12,   13,   14,   15,   16,
    2,   17,
};
short yysindex[] = {                                      0,
    0, -235, -246, -227, -221, -215, -214,  -78,    2,    0,
 -261,    0, -266,  -44, -259,    0,    3,    0,    4,    0,
    5,    0,    0, -207,    0,    0,    0,    0,    0, -206,
    0,    0,    0,    0,    0,    0,    0,    0, -255,    0,
    0, -205,    0,    0,    0,  -72, -204, -203, -123,    0,
 -228, -228, -228,   11,    0, -201,   13,    0,    0,   15,
    0, -199, -198, -197,    0, -194,    0, -193, -117, -192,
    0,    0,    0,    0,    0,    0,    0,
};
short yyrindex[] = {                                      0,
    0,   67,    0,    0,    0,    0,    0,    0,    7,    0,
   38,    0,   69,    1,   32,    0,   68,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   63,   94,  125,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,
};
short yygindex[] = {                                      0,
    0,   -6,    0,    0,    0,    0,    0,    0,    0,    0,
  -19,    0,
};
#define YYTABLESIZE 415
short yytable[] = {                                      42,
   45,   61,   55,   49,   25,   26,    7,   76,   27,   28,
   29,   30,   18,   31,   32,   33,   34,   35,   36,   37,
   38,   52,   53,   39,    3,    4,    5,    6,    7,    8,
   19,   46,   43,   44,   45,   56,   20,    9,   69,   62,
   63,   64,   21,   22,   23,   24,   46,   47,   48,   50,
   58,   54,   57,   59,   60,   66,   67,   68,   70,   71,
   72,   73,   42,   74,   75,   77,    1,    2,   11,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   43,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   44,   45,    0,    0,    0,    0,
    0,    7,    0,    0,    0,    0,    3,    4,    5,    6,
    7,    0,    3,    4,    5,    6,    7,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   46,    0,    0,    0,
    0,    0,    9,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   42,    0,    0,
    0,    0,    0,   11,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   43,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   40,   41,    0,    0,    0,    0,   44,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   45,   45,   45,   45,   45,   45,    7,    7,    7,    7,
    7,    7,    0,    0,    0,   45,   45,   45,   45,    0,
   45,   45,   45,   45,   45,   45,   45,   45,    0,    0,
   45,   46,   46,   46,   46,   46,   46,    9,    9,    9,
    9,    9,    9,    0,    0,    0,   46,   46,   46,   46,
    0,   46,   46,   46,   46,   46,   46,   46,   46,    0,
    0,   46,   42,   42,   42,   42,   42,   42,   11,   11,
   11,   11,   11,   11,    0,    0,    0,   42,   42,   42,
   42,    0,   42,   42,   42,   42,   42,   42,   42,   42,
    0,    0,   42,   43,   43,   43,   43,   43,   43,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   43,   43,
   43,   43,    0,   43,   43,   43,   43,   43,   43,   43,
   43,    0,    0,   43,   44,   44,   44,   44,   44,   44,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   44,
   44,   44,   44,    0,   44,   44,   44,   44,   44,   44,
   44,   44,    0,    0,   44,
};
short yycheck[] = {                                      44,
    0,  125,  258,   23,  266,  267,    0,  125,  275,  276,
  277,  278,  259,  280,  281,  282,  283,  284,  285,  286,
  287,   28,   29,  290,  260,  261,  262,  263,  264,  265,
  258,    0,  292,  293,  294,  291,  258,    0,   58,  268,
  269,  270,  258,  258,  123,   44,   44,   44,   44,  257,
  123,  258,  258,  258,  258,   45,  258,   45,   44,  259,
  259,  259,    0,  258,  258,  258,    0,    0,    0,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
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
"BIT16","IRQ","MASK","LEVEL","PULSE","SHARED",
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
"pwrlist :",
"pwrlist : pwrlist pwr",
"io : cftab IO NUMBER '-' NUMBER",
"io : io ',' NUMBER '-' NUMBER",
"io : io BIT8",
"io : io BIT16",
"irq : cftab IRQ NUMBER",
"irq : cftab IRQ MASK NUMBER",
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
#line 238 "yacc_cis.y"

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
#line 353 "y.tab.c"
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
#line 61 "yacc_cis.y"
{ cis_root = yyvsp[0].tuple; }
break;
case 2:
#line 63 "yacc_cis.y"
{ cis_root = yyvsp[-1].tuple; }
break;
case 3:
#line 67 "yacc_cis.y"
{ yyval.tuple = NULL; }
break;
case 4:
#line 69 "yacc_cis.y"
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
#line 82 "yacc_cis.y"
{ mfc[nf++] = yyvsp[-1].tuple; }
break;
case 6:
#line 84 "yacc_cis.y"
{ mfc[nf++] = yyvsp[-1].tuple; }
break;
case 7:
#line 88 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_VERS_1, yyvsp[0].parse); }
break;
case 8:
#line 90 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_MANFID, yyvsp[0].parse); }
break;
case 9:
#line 92 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_FUNCID, yyvsp[0].parse); }
break;
case 10:
#line 94 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_CONFIG, yyvsp[0].parse); }
break;
case 11:
#line 96 "yacc_cis.y"
{ yyval.tuple = new_tuple(CISTPL_CFTABLE_ENTRY, yyvsp[0].parse); }
break;
case 12:
#line 100 "yacc_cis.y"
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
#line 110 "yacc_cis.y"
{
		    u_int pos = strlen(yyval.parse->version_1.str);
		    if (pos > 0) pos++;
		    yyval.parse->version_1.ofs[yyval.parse->version_1.ns] = pos;
		    strcpy(yyval.parse->version_1.str+pos, yyvsp[0].str);
		    yyval.parse->version_1.ns++;
		}
break;
case 14:
#line 120 "yacc_cis.y"
{
		    yyval.parse = calloc(1, sizeof(cisparse_t));
		    yyval.parse->manfid.manf = yyvsp[-2].num;
		    yyval.parse->manfid.card = yyvsp[0].num;
		}
break;
case 15:
#line 128 "yacc_cis.y"
{
		    yyval.parse = calloc(1, sizeof(cisparse_t));
		    yyval.parse->funcid.func = yyvsp[0].num;
		}
break;
case 16:
#line 133 "yacc_cis.y"
{ yyval.parse->funcid.sysinit |= CISTPL_SYSINIT_POST; }
break;
case 17:
#line 135 "yacc_cis.y"
{ yyval.parse->funcid.sysinit |= CISTPL_SYSINIT_ROM; }
break;
case 18:
#line 139 "yacc_cis.y"
{
		    yyval.parse = calloc(1, sizeof(cisparse_t));
		    yyval.parse->config.last_idx = yyvsp[-4].num;
		    yyval.parse->config.base = yyvsp[-2].num;
		    yyval.parse->config.rmask[0] = yyvsp[0].num;
		}
break;
case 19:
#line 148 "yacc_cis.y"
{
		    yyval.pwr.present |= 1<<CISTPL_POWER_VNOM;
		    yyval.pwr.param[CISTPL_POWER_VNOM] = yyvsp[0].flt * 100000;
		}
break;
case 20:
#line 153 "yacc_cis.y"
{
		    yyval.pwr.present |= 1<<CISTPL_POWER_VMIN;
		    yyval.pwr.param[CISTPL_POWER_VMIN] = yyvsp[0].flt * 100000;
		}
break;
case 21:
#line 158 "yacc_cis.y"
{
		    yyval.pwr.present |= 1<<CISTPL_POWER_VMAX;
		    yyval.pwr.param[CISTPL_POWER_VMAX] = yyvsp[0].flt * 100000;
		}
break;
case 22:
#line 165 "yacc_cis.y"
{
		    yyval.pwr.present = 0;
		}
break;
case 24:
#line 172 "yacc_cis.y"
{
		    int n = yyval.parse->cftable_entry.io.nwin;
		    yyval.parse->cftable_entry.io.win[n].base = yyvsp[-2].num;
		    yyval.parse->cftable_entry.io.win[n].len = yyvsp[0].num-yyvsp[-2].num+1;
		    yyval.parse->cftable_entry.io.nwin++;
		}
break;
case 25:
#line 179 "yacc_cis.y"
{
		    int n = yyval.parse->cftable_entry.io.nwin;
		    yyval.parse->cftable_entry.io.win[n].base = yyvsp[-2].num;
		    yyval.parse->cftable_entry.io.win[n].len = yyvsp[0].num-yyvsp[-2].num+1;
		    yyval.parse->cftable_entry.io.nwin++;
		}
break;
case 26:
#line 186 "yacc_cis.y"
{ yyval.parse->cftable_entry.io.flags |= CISTPL_IO_8BIT; }
break;
case 27:
#line 188 "yacc_cis.y"
{ yyval.parse->cftable_entry.io.flags |= CISTPL_IO_16BIT; }
break;
case 28:
#line 192 "yacc_cis.y"
{ yyval.parse->cftable_entry.irq.IRQInfo1 = (yyvsp[0].num & 0x0f); }
break;
case 29:
#line 194 "yacc_cis.y"
{
		    yyval.parse->cftable_entry.irq.IRQInfo1 = IRQ_INFO2_VALID;
		    yyval.parse->cftable_entry.irq.IRQInfo2 = yyvsp[0].num;
		}
break;
case 30:
#line 199 "yacc_cis.y"
{ yyval.parse->cftable_entry.irq.IRQInfo1 |= IRQ_PULSE_ID; }
break;
case 31:
#line 201 "yacc_cis.y"
{ yyval.parse->cftable_entry.irq.IRQInfo1 |= IRQ_LEVEL_ID; }
break;
case 32:
#line 203 "yacc_cis.y"
{ yyval.parse->cftable_entry.irq.IRQInfo1 |= IRQ_SHARE_ID; }
break;
case 33:
#line 207 "yacc_cis.y"
{
		    yyval.parse = calloc(1, sizeof(cisparse_t));
		    yyval.parse->cftable_entry.index = yyvsp[0].num;
		}
break;
case 34:
#line 212 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_DEFAULT; }
break;
case 35:
#line 214 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_BVDS; }
break;
case 36:
#line 216 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_WP; }
break;
case 37:
#line 218 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_RDYBSY; }
break;
case 38:
#line 220 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_MWAIT; }
break;
case 39:
#line 222 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_AUDIO; }
break;
case 40:
#line 224 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_READONLY; }
break;
case 41:
#line 226 "yacc_cis.y"
{ yyval.parse->cftable_entry.flags |= CISTPL_CFTABLE_PWRDOWN; }
break;
case 42:
#line 228 "yacc_cis.y"
{ yyval.parse->cftable_entry.vcc = yyvsp[0].pwr; }
break;
case 43:
#line 230 "yacc_cis.y"
{ yyval.parse->cftable_entry.vpp1 = yyvsp[0].pwr; }
break;
case 44:
#line 232 "yacc_cis.y"
{ yyval.parse->cftable_entry.vpp2 = yyvsp[0].pwr; }
break;
#line 728 "y.tab.c"
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
