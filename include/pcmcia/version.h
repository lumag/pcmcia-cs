/* version.h 1.112 2002/08/22 15:26:50 (David Hinds) */

#define CS_PKG_RELEASE		"3.2.2"
#define CS_PKG_RELEASE_CODE	0x3202

#define VERSION(v,p,s)		(((v)<<16)+(p<<8)+s)

#ifdef CONFIG_PCMCIA
#include_next <pcmcia/version.h>
#else
#define CS_RELEASE		CS_PKG_RELEASE
#define CS_RELEASE_CODE		CS_PKG_RELEASE_CODE
#endif
