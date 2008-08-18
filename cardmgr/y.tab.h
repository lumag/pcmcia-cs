#define DEVICE 257
#define CARD 258
#define ANONYMOUS 259
#define TUPLE 260
#define MANFID 261
#define VERSION 262
#define FUNCTION 263
#define BIND 264
#define TO 265
#define NEEDS_MTD 266
#define MODULE 267
#define OPTS 268
#define CLASS 269
#define REGION 270
#define JEDEC 271
#define DTYPE 272
#define DEFAULT 273
#define MTD 274
#define INCLUDE 275
#define EXCLUDE 276
#define RESERVE 277
#define IRQ_NO 278
#define PORT 279
#define MEMORY 280
#define STRING 281
#define NUMBER 282
typedef union {
    char *str;
    u_long num;
    struct device_info_t *device;
    struct card_info_t *card;
    struct mtd_ident_t *mtd;
    struct adjust_list_t *adjust;
} YYSTYPE;
extern YYSTYPE yylval;
