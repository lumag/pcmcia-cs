/* cardmgr.h $Revision: 1.24 $ $Date: 1997/12/24 17:28:02 $ (David Hinds) */

#define MAX_SOCKS	8
#define MAX_FUNCTIONS	4
#define MAX_MODULES	4

typedef struct adjust_list_t {
    adjust_t		adj;
    struct adjust_list_t *next;
} adjust_list_t;

typedef struct func_ident_t {
    u_char		funcid;
} func_ident_t;

typedef struct manfid_ident_t {
    u_short		manf;
    u_short		card;
} manfid_ident_t;

typedef struct vers_ident_t {
    int			ns;
    char		*pi[4];
} vers_ident_t;

typedef struct tuple_ident_t {
    cisdata_t		code;
    long		ofs;
    char		*info;
} tuple_ident_t;

typedef struct device_info_t {
    dev_info_t		dev_info;
    int			needs_mtd;
    int			modules;
    char		*module[MAX_MODULES];
    char		*opts[MAX_MODULES];
    char		*class;
    struct device_info_t *next;
} device_info_t;

typedef struct card_info_t {
    char		*name;
    enum {
	VERS_1_IDENT=1, MANFID_IDENT, TUPLE_IDENT, FUNC_IDENT,
	BLANK_IDENT, CARDBUS_IDENT
    } ident_type;
    union {
	vers_ident_t	vers;
	manfid_ident_t	manfid;
	tuple_ident_t	tuple;
	func_ident_t	func;
    } id;
    int			functions;
    device_info_t	*device[MAX_FUNCTIONS];
    int			dev_fn[MAX_FUNCTIONS];
    struct card_info_t	*next;
} card_info_t;

typedef struct mtd_ident_t {
    char		*name;
    enum {
	JEDEC_MTD=1, DTYPE_MTD, DEFAULT_MTD
    } mtd_type;
    int			dtype, jedec_mfr, jedec_info;
    char		*module;
    struct mtd_ident_t	*next;
} mtd_ident_t;
    
extern adjust_list_t	*root_adjust;
extern device_info_t	*root_device;
extern card_info_t	*blank_card;
extern card_info_t	*root_card, *root_func;
extern mtd_ident_t	*root_mtd, *default_mtd;

int parse_configfile(char *fn);
