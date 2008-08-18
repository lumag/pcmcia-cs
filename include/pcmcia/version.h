/* version.h 1.107 2002/03/05 01:23:27 (David Hinds) */

#define CS_PKG_RELEASE		"3.1.34"
#define CS_PKG_RELEASE_CODE	0x3122

#ifdef CONFIG_PCMCIA
#include_next <pcmcia/version.h>
#else
#define CS_RELEASE		CS_PKG_RELEASE
#define CS_RELEASE_CODE		CS_PKG_RELEASE_CODE
#endif
