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
typedef union {
    char *str;
    u_long num;
    float flt;
    cistpl_power_t pwr;
    cisparse_t *parse;
    tuple_info_t *tuple;
} YYSTYPE;
extern YYSTYPE yylval;
