#ifndef _PCMCIA_PM_H
#define _PCMCIA_PM_H

#if (LINUX_VERSION_CODE < 0x02032b)

#include <linux/apm_bios.h>

/* This is an ugly hack: it only works in case statements */
#define PM_SUSPEND		APM_SYS_SUSPEND: case APM_USER_SUSPEND
#define PM_RESUME		APM_NORMAL_RESUME: case APM_CRITICAL_RESUME

#define pm_register(a, b, fn)	apm_register_callback(fn)
#define pm_unregister_all(fn)	apm_unregister_callback(fn)

#else

#include_next <linux/pm.h>

#endif

#endif /* _PCMCIA_PM_H */
