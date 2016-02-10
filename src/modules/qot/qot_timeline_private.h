/*
 * @file qot_core.h
 * @brief Interfaces used by clocks to interact with core
 * @author Andrew Symington
 *
 * Copyright (c) Regents of the University of California, 2015.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _PTP_PRIVATE_H_
#define _PTP_PRIVATE_H_

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/posix-clock.h>
#include <linux/ptp_clock.h>
#include <linux/ptp_clock_kernel.h>
#include <linux/time.h>

#define PTP_MAX_TIMESTAMPS 128
#define PTP_BUF_TIMESTAMPS 30

struct timestamp_event_queue {
	struct ptp_extts_event buf[PTP_MAX_TIMESTAMPS];
	int head;
	int tail;
	spinlock_t lock;
};

struct ptp_clock {
	struct posix_clock clock;
	struct device *dev;
	struct ptp_clock_info *info;
	dev_t devid;
	int index; /* index into clocks.map */
	struct pps_device *pps_source;
	long dialed_frequency; /* remembers the frequency adjustment */
	struct timestamp_event_queue tsevq; /* simple fifo for time stamps */
	struct mutex tsevq_mux; /* one process at a time reading the fifo */
	struct mutex pincfg_mux; /* protect concurrent info->pin_config access */
	wait_queue_head_t tsev_wq;
	int defunct; /* tells readers to go away when clock is being removed */
	struct device_attribute *pin_dev_attr;
	struct attribute **pin_attr;
	struct attribute_group pin_attr_group;
};

/*
 * The function queue_cnt() is safe for readers to call without
 * holding q->lock. Readers use this function to verify that the queue
 * is nonempty before proceeding with a dequeue operation. The fact
 * that a writer might concurrently increment the tail does not
 * matter, since the queue remains nonempty nonetheless.
 */
static inline int queue_cnt(struct timestamp_event_queue *q)
{
	int cnt = q->tail - q->head;
	return cnt < 0 ? PTP_MAX_TIMESTAMPS + cnt : cnt;
}

/*
 * see ptp_chardev.c
 */

/* caller must hold pincfg_mux */
int ptp_set_pinfunc(struct ptp_clock *ptp, unsigned int pin,
		    enum ptp_pin_function func, unsigned int chan);

long ptp_ioctl(struct posix_clock *pc,
	       unsigned int cmd, unsigned long arg);

int ptp_open(struct posix_clock *pc, fmode_t fmode);

ssize_t ptp_read(struct posix_clock *pc,
		 uint flags, char __user *buf, size_t cnt);

uint ptp_poll(struct posix_clock *pc,
	      struct file *fp, poll_table *wait);

/*
 * see ptp_sysfs.c
 */

extern const struct attribute_group *ptp_groups[];

int ptp_cleanup_sysfs(struct ptp_clock *ptp);

int ptp_populate_sysfs(struct ptp_clock *ptp);

#endif
