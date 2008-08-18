#ifndef _COMPAT_SEQ_FILE_H
#define _COMPAT_SEQ_FILE_H

#include <linux/version.h>

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,4,17))
#include_next <linux/seq_file.h>
#endif

#endif /* _COMPAT_SEQ_FILE_H */
