/*======================================================================

    Utility to look up information about IDE devices

    Written by David Hinds, dhinds@allegro.stanford.edu

    ide_info.c 1.3 1997/12/18 18:24:03
    
======================================================================*/

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>

#include <linux/major.h>
#include <linux/hdreg.h>

/*====================================================================*/

int main(int argc, char *argv[])
{
    int fd;
    struct hd_driveid id;

    if (argc != 2) {
	fprintf(stderr, "usage: %s [device]\n", argv[0]);
	exit(EXIT_FAILURE);
    }
    
    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
	perror("open() failed");
	exit(1);
    }
    if (ioctl(fd, HDIO_GET_IDENTITY, &id) != 0) {
	perror("could not get IDE device info");
	exit(1);
    }

    printf("MODEL=\"%s\"\n", id.model);
    printf("FW_REV=\"%s\"\n", id.fw_rev);
    printf("SERIAL_NO=\"%s\"\n", id.serial_no);
    exit(0);
    return 0;
}
