/*======================================================================

    Utility to look up information about SCSI devices

    Written by David Hinds, dhinds@allegro.stanford.edu

    scsi_info.c 1.7 1997/12/18 18:24:03
    
======================================================================*/

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>

#include <linux/version.h>
#include <linux/config.h>

#include <linux/major.h>
#define VERSION(v,p,s) (((v)<<16)+(p<<8)+s)
#if (LINUX_VERSION_CODE > VERSION(1,3,97))
#include <scsi/scsi.h>
#else
#if (LINUX_VERSION_CODE > VERSION(1,3,0))
#include <linux/scsi.h>
#else
#include <linux/fs.h>
#endif
#endif

/*====================================================================*/

static int get_host_no(int host)
{
    DIR *dir;
    struct dirent *ent;
    char fn[128];
    
    dir = opendir("/proc/scsi");
    if (dir == NULL)
	return -1;
    while ((ent = readdir(dir)) != NULL)
	if ((ent->d_ino & 0xff) == host)
	    break;
    closedir(dir);
    if (!ent) {
	perror("could not find SCSI host in /proc/scsi");
	return -1;
    }
    
    sprintf(fn, "/proc/scsi/%s", ent->d_name);
    dir = opendir(fn);
    if (dir == NULL) {
	return -1;
    }
    do {
	ent = readdir(dir);
    } while ((ent) && (ent->d_name[0] == '.'));
    closedir(dir);
    if (ent)
	return atoi(ent->d_name);
    else
	return -1;
}

/*====================================================================*/

int main(int argc, char *argv[])
{
    int i, fd, host, channel, id, lun;
    u_long arg[2];
    char match[128], s[128], vendor[9], model[17], rev[5];
    FILE *f;

    if (argc != 2) {
	fprintf(stderr, "usage: %s [device]\n", argv[0]);
	exit(EXIT_FAILURE);
    }
    
    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
	perror("open() failed");
	exit(1);
    }
    if (ioctl(fd, SCSI_IOCTL_GET_IDLUN, arg) != 0) {
	perror("could not get SCSI device info");
	exit(1);
    }

    id = arg[0] & 0xff;
    lun = (arg[0] >> 8) & 0xff;
    channel = (arg[0] >> 16) & 0xff;
    host = ((arg[0] >> 24) & 0xff);

    sprintf(match, "Host: scsi%d Channel: %02x Id: %02x Lun: %02x\n",
	    get_host_no(host), channel, id, lun);

    printf("SCSI_ID=\"%d,%d,%d\"\n", channel, id, lun);
    
    f = fopen("/proc/scsi/scsi", "r");
    if (f == NULL) {
	printf("MODEL=\"Unknown\"\nFW_REV=\"Unknown\"\n");
	exit(0);
    }

    while (fgets(s, 128, f) != NULL)
	if (strcmp(s, match) == 0) break;
    fgets(s, 128, f);
    strncpy(vendor, s+10, 8); vendor[8] = '\0';
    for (i = 7; (i >= 0) && (vendor[i] == ' '); i--)
	vendor[i] = '\0';
    strncpy(model, s+26, 16); model[16] = '\0';
    for (i = 15; (i >= 0) && (model[i] == ' '); i--)
	model[i] = '\0';
    strncpy(rev, s+48, 4); rev[4] = '\0';
    for (i = 3; (i >= 0) && (rev[i] == ' '); i--)
	rev[i] = '\0';
    printf("MODEL=\"%s %s\"\nFW_REV=\"%s\"\n", vendor, model, rev);
    exit(0);
    return 0;
}
