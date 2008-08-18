/*
  Exit code = 0 if IPv6 is supported, 1 if not.
*/
#include <stdio.h>
#include <sys/socket.h>

void main(void)
{
#ifndef AF_INET6
    exit(1);
#else
    int s = socket(AF_INET6, SOCK_STREAM, 0);
    if (s == -1)
	exit(1);
    close(s);
    exit(0);
#endif
}
