/* version.h 1.105 2002/01/02 03:05:03 (David Hinds) */

#define CS_PKG_RELEASE		"3.1.32"
#define CS_PKG_RELEASE_CODE	0x3120

#ifdef CONFIG_PCMCIA
#include_next <pcmcia/version.h>
#else
#define CS_RELEASE		CS_PKG_RELEASE
#define CS_RELEASE_CODE		CS_PKG_RELEASE_CODE
#endif
