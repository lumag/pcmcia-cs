/* version.h 1.106 2002/03/02 21:53:42 (David Hinds) */

#define CS_PKG_RELEASE		"3.1.33"
#define CS_PKG_RELEASE_CODE	0x3121

#ifdef CONFIG_PCMCIA
#include_next <pcmcia/version.h>
#else
#define CS_RELEASE		CS_PKG_RELEASE
#define CS_RELEASE_CODE		CS_PKG_RELEASE_CODE
#endif
