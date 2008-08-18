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
typedef union {
    char *str;
    u_long num;
    float flt;
    cistpl_power_t pwr;
    cisparse_t *parse;
    tuple_info_t *tuple;
} YYSTYPE;
extern YYSTYPE yylval;
