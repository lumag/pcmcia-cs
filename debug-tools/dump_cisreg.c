/*======================================================================

    PCMCIA card CIS dump

    Written by David Hinds, dhinds@allegro.stanford.edu
    
======================================================================*/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/cisreg.h>
#include <pcmcia/ds.h>

static const char *version =
"dump_cisreg.c 1.11 1997/12/29 17:48:45 (David Hinds)\n";

/*====================================================================*/

static int lookup_dev(char *name)
{
    FILE *f;
    int n;
    char s[32], t[32];
    
    f = fopen("/proc/devices", "r");
    if (f == NULL)
	return -1;
    while (fgets(s, 32, f) != NULL) {
	if (sscanf(s, "%d %s", &n, t) == 2)
	    if (strcmp(name, t) == 0)
		break;
    }
    fclose(f);
    if (strcmp(name, t) == 0)
	return n;
    else
	return -1;
} /* lookup_dev */

/*====================================================================*/

static int open_dev(dev_t dev)
{
    char *fn;
    int fd;
    
    if ((fn = tmpnam(NULL)) == NULL)
	return -1;
    if (mknod(fn, (S_IFCHR|S_IREAD|S_IWRITE), dev) != 0)
	return -1;
    fd = open(fn, O_RDONLY);
    unlink(fn);
    return fd;
} /* open_dev */

/*====================================================================*/

static int get_reg(int fd, int fn, off_t off)
{
    ds_ioctl_arg_t arg;
    int ret;

    arg.conf_reg.Function = fn;
    arg.conf_reg.Action = CS_READ;
    arg.conf_reg.Offset = off;
    ret = ioctl(fd, DS_ACCESS_CONFIGURATION_REGISTER, &arg);
    if (ret != 0) {
	printf("  read config register: %s\n\n", strerror(errno));
	return -1;
    }
    return arg.conf_reg.Value;
}

static int dump_option(int fd, int fn, int mfc)
{
    int v = get_reg(fd, fn, CISREG_COR);

    if (v == -1) return -1;
    printf("  Configuration option register = %#2.2x\n", v);
    printf("   ");
    if (v & COR_LEVEL_REQ) printf(" [level_req]");
    if (v & COR_SOFT_RESET) printf(" [soft_reset]");
    if (mfc) {
	if (v & COR_FUNC_ENA) printf(" [func_ena]");
	if (v & COR_ADDR_DECODE) printf(" [addr_decode]");
	if (v & COR_IREQ_ENA) printf(" [ireq_ena]");
	printf(" [index = %#2.2x]\n", v & COR_MFC_CONFIG_MASK);
    } else
	printf(" [index = %#2.2x]\n", v & COR_CONFIG_MASK);
    return 0;
}

static void dump_status(int fd, int fn)
{
    int v = get_reg(fd, fn, CISREG_CCSR);
    
    printf("  Card configuration and status register = %#2.2x\n", v);
    printf("   ");
    if (v & CCSR_INTR_ACK) printf(" [intr_ack]");
    if (v & CCSR_INTR_PENDING) printf(" [intr_pending]");
    if (v & CCSR_POWER_DOWN) printf(" [power_down]");
    if (v & CCSR_AUDIO_ENA) printf(" [audio]");
    if (v & CCSR_IOIS8) printf(" [IOis8]");
    if (v & CCSR_SIGCHG_ENA) printf(" [sigchg]");
    if (v & CCSR_CHANGED) printf(" [changed]");
    printf("\n");
}

static void dump_pin(int fd, int fn)
{
    int v = get_reg(fd, fn, CISREG_PRR);
    
    printf("  Pin replacement register = %#2.2x\n", v);
    printf("   ");
    if (v & PRR_WP_STATUS) printf(" [wp]");
    if (v & PRR_READY_STATUS) printf(" [ready]");
    if (v & PRR_BVD2_STATUS) printf(" [bvd2]");
    if (v & PRR_BVD1_STATUS) printf(" [bvd1]");
    if (v & PRR_WP_EVENT) printf(" [wp_event]");
    if (v & PRR_READY_EVENT) printf(" [ready_event]");
    if (v & PRR_BVD2_EVENT) printf(" [bvd2_event]");
    if (v & PRR_BVD1_EVENT) printf(" [bvd1_event]");
    printf("\n");
}

static void dump_copy(int fd, int fn)
{
    int v = get_reg(fd, fn, CISREG_SCR);
    
    printf("  Socket and copy register = %#2.2x\n", v);
    printf("    [socket = %d] [copy = %d]\n",
	   v & SCR_SOCKET_NUM,
	   (v & SCR_COPY_NUM) >> 4);
}

static void dump_ext_status(int fd, int fn)
{
    int v = get_reg(fd, fn, CISREG_ESR);
    printf("  Extended status register = %#2.2x\n", v);
    printf("   ");
    if (v & ESR_REQ_ATTN_ENA) printf(" [req_attn_ena]");
    if (v & ESR_REQ_ATTN) printf(" [req_attn]");
    printf("\n");
}

/*====================================================================*/

static void dump_all(int fd, int fn, int mfc, u_int mask)
{
    int addr;
    if (mask & PRESENT_OPTION) {
	if (dump_option(fd, fn, mfc) != 0)
	    return;
    }
    if (mask & PRESENT_STATUS)
	dump_status(fd, fn);
    if (mask & PRESENT_PIN_REPLACE)
	dump_pin(fd, fn);
    if (mask & PRESENT_COPY)
	dump_copy(fd, fn);
    if (mask & PRESENT_EXT_STATUS)
	dump_ext_status(fd, fn);
    if (mask & PRESENT_IOBASE_0) {
	addr = get_reg(fd, fn, CISREG_IOBASE_0);
	addr += get_reg(fd, fn, CISREG_IOBASE_1) << 8;
	printf("  IO base = 0x%04x\n", addr);
    }
    if (mask & PRESENT_IOSIZE)
	printf("  IO size = %d\n", get_reg(fd, fn, CISREG_IOSIZE));
    if (mask == 0)
	printf("  no config registers\n\n");
    else
	printf("\n");
}

/*====================================================================*/

#define MAX_SOCKS 8

int main(int argc, char *argv[])
{
    int i, j, nfn, major, fd, ret;
    u_int mask;
    ds_ioctl_arg_t arg;

    if ((argc > 1) && (strcmp(argv[1], "-v") == 0))
	printf("%s", version);
    
    major = lookup_dev("pcmcia");
    if (major < 0) {
	fprintf(stderr, "no pcmcia driver in /proc/devices\n");
	exit(EXIT_FAILURE);
    }
    
    for (i = 0; i < MAX_SOCKS; i++) {
	fd = open_dev((major<<8)+i);
	if (fd < 0) break;
	
	arg.tuple.TupleDataMax = sizeof(arg.tuple_parse.data);
	arg.tuple.Attributes = TUPLE_RETURN_COMMON;
	arg.tuple.DesiredTuple = CISTPL_LONGLINK_MFC;
	arg.tuple.TupleOffset = 0;
	if (ioctl(fd, DS_GET_FIRST_TUPLE, &arg) == 0) {
	    ioctl(fd, DS_GET_TUPLE_DATA, &arg);
	    ioctl(fd, DS_PARSE_TUPLE, &arg);
	    nfn = arg.tuple_parse.parse.longlink_mfc.nfn;
	} else {
	    nfn = 1;
	    arg.tuple.DesiredTuple = CISTPL_DEVICE;
	    ret = ioctl(fd, DS_GET_FIRST_TUPLE, &arg);
	}
	arg.tuple.DesiredTuple = CISTPL_CONFIG;
	
	for (j = 0; j < nfn; j++) {
	    printf("Socket %d function %d:\n", i, j);
	    if (ioctl(fd, DS_GET_NEXT_TUPLE, &arg) != 0) {
		printf("  no config tuple: %s\n\n", strerror(errno));
		continue;
	    }
	    ioctl(fd, DS_GET_TUPLE_DATA, &arg);
	    ioctl(fd, DS_PARSE_TUPLE, &arg);
	    printf("  Config register base = %#4.4x, mask = %#4.4x\n",
		   arg.tuple_parse.parse.config.base,
		   arg.tuple_parse.parse.config.rmask[0]);
	    mask = arg.tuple_parse.parse.config.rmask[0];
	    dump_all(fd, j, (nfn > 1), mask);
	}
    
    }
    return 0;
}
