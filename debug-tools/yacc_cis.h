typedef union {
    char *str;
    u_long num;
    float flt;
    cistpl_power_t pwr;
    cisparse_t *parse;
    tuple_info_t *tuple;
} YYSTYPE;
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


extern YYSTYPE yylval;
