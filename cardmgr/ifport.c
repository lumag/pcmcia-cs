/*======================================================================

    Utility to select the transceiver type for a network device

    Written by David Hinds, dhinds@hyper.stanford.edu

    ifport.c 1.5 1997/12/18 18:24:03
    
======================================================================*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *if_names[] = { "Auto", "10baseT", "10base2", "AUI", "100baseT" };

/*====================================================================*/

static int sockets_open(void)
{
    int sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) != -1)
	return sock;
    else if ((sock = socket(AF_IPX, SOCK_DGRAM, 0)) != -1)
	return sock;
    else if ((sock = socket(AF_AX25, SOCK_DGRAM, 0)) != -1)
	return sock;
    else
	return socket(AF_APPLETALK, SOCK_DGRAM, 0);
}

/*====================================================================*/

void usage(char *s)
{
    fprintf(stderr, "usage: %s interface "
	    "[auto|10baseT|10base2|aui|100baseT|##]\n", s);
    exit(1);
}

int main(int argc, char **argv)
{
    struct ifreq ifr;
    int i, skfd;

    if ((argc < 2) || (argc > 3))
	usage(argv[0]);
    skfd = sockets_open();
    if (skfd == -1) {
	perror("socket");
	exit(1);
    }
    strcpy(ifr.ifr_name, argv[1]);
    if (ioctl(skfd, SIOCGIFMAP, &ifr) < 0)
	fprintf(stderr, "%s: unknown interface.\n", argv[1]);
    else {
	if (argc == 2) {
	    printf("%s\t%d", argv[1], ifr.ifr_map.port);
	    if (ifr.ifr_map.port < 5)
		printf(" (%s)\n", if_names[ifr.ifr_map.port]);
	    else
		printf("\n");
	}
	else {
	    for (i = 0; i < 5; i++)
		if (strcasecmp(argv[2], if_names[i]) == 0)
		    break;
	    if (i < 5)
		ifr.ifr_map.port = i;
	    else {
		char *s;
		ifr.ifr_map.port = strtoul(argv[2], &s, 0);
		if ((s == argv[2]) || (*s != '\0'))
		    usage(argv[0]);
	    }
	    if (ioctl(skfd, SIOCSIFMAP, &ifr) < 0)
		perror("ioctl");
	}
    }
    close(skfd);
    exit(0);
    return 0;
}
