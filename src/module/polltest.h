#ifndef _POLLTEST_H_
#define _POLLTEST_H_

#include <linux/ioctl.h>

// Magic code specific to our ioctl code
#define MAGIC_CODE 0xEF

// IOCTL with /dev/qot
#define POLLTEST_GET_VALUE	_IOR(MAGIC_CODE, 1, int *)

#endif
