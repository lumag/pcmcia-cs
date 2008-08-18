/* version.h 1.115 2003/03/04 17:03:08 (David Hinds) */

#define CS_PKG_RELEASE		"3.2.5"
#define CS_PKG_RELEASE_CODE	0x3205

#define VERSION(v,p,s)		(((v)<<16)+(p<<8)+s)

#ifdef CONFIG_PCMCIA
#include_next <pcmcia/version.h>
#else
#define CS_RELEASE		CS_PKG_RELEASE
#define CS_RELEASE_CODE		CS_PKG_RELEASE_CODE
#endif
