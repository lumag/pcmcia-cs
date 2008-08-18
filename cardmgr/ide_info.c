/*======================================================================

    Utility to look up information about IDE devices

    ide_info.c 1.6 1999/07/20 16:02:24

    The contents of this file are subject to the Mozilla Public
    License Version 1.1 (the "License"); you may not use this file
    except in compliance with the License. You may obtain a copy of
    the License at http://www.mozilla.org/MPL/

    Software distributed under the License is distributed on an "AS
    IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
    implied. See the License for the specific language governing
    rights and limitations under the License.

    The initial developer of the original code is David A. Hinds
    <dhinds@hyper.stanford.edu>.  Portions created by David A. Hinds
    are Copyright (C) 1998 David A. Hinds.  All Rights Reserved.
    
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
