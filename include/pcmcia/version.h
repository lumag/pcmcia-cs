/* version.h 1.104 2001/12/01 01:19:10 (David Hinds) */

#define CS_PKG_RELEASE		"3.1.31"
#define CS_PKG_RELEASE_CODE	0x311f

#ifdef CONFIG_PCMCIA
#include_next <pcmcia/version.h>
#else
#define CS_RELEASE		CS_PKG_RELEASE
#define CS_RELEASE_CODE		CS_PKG_RELEASE_CODE
#endif
