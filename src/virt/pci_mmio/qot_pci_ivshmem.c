/**
 * @file qot_pci_ivshmem.c
 * @brief Library interface which enables guests to read timeline clock metadata (mapping, uncertainty)
 *        (Based on QEMU-KVM ivshmem using a PCI-MMIO-based device)
 * @author Sandeep D'souza
 * 
 * Copyright (c) Carnegie Mellon University, 2017. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * based on the following program:
 *  prog : upci_rw.c
 *  desc : reads/writes the I/O or MMIO region of a PCI Base Address Register.
 *          Uses "inb(2)"/"outb(2)" for I/O register access.
 *          Uses "/dev/mem" for MMIO access
 *
 * notes :
 *
 *  o Example usage: 
 *         $ gcc -DDEBUG -I. -Wall -Wextra -Wno-unused -O2 \
 *               -o ne_upci_rw ne_upci_rw.c upci.c upci.h
 *
 *         $ lspci -s 00:04.0 -m -n -v
 *         Device:  00:04.0
 *         Class:   0500
 *         Vendor:  1af4
 *         Device:  1110
 *         SVendor: 1af4
 *         SDevice: 1100
 *         PhySlot: 4
 *
 *         $ ./ne_upci_rw -D 0x1110 -V 0X1af4 -d 0x1100 -v 0x1af4 -r 2 -a 0
 *         PCI device  6 at bus/device/function 00/04/0
 *         Vendor/Device/SSVendor/SSDevice: 1af4/1110/1af4/1100
 *         Region 0: MEM 256 bytes at febd2000
 *         Region 2: MEM 1048576 bytes at fe000000
 *
 *  o See "http:://nairobi-embedded.org/mmap_n_devmem.html" for more.
 * 
 *  o Derived from "pci_read.c/pci_write.c" of LinuxCNC (www.linuxcnc.org)
 *    source:
 * 
 *      (c) 2007, Stephen Wille Padnos, swpadnos @ users.sourceforge.net
 *      loosely based on a work by John Kasunich: bfload.c
 *
 * Siro Mugabi, nairobi-embedded.org
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as published 
 *  by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <linux/types.h>
#include "upci.h"

#include "qot_pci_ivshmem.h"

#define prfmt(fmt) "%s:%d:: " fmt, __func__, __LINE__
#define prinfo(fmt, ...) printf(prfmt(fmt), ##__VA_ARGS__)
#define prerr(fmt, ...) fprintf(stderr, prfmt(fmt), ##__VA_ARGS__)
#ifdef DEBUG
#define prdbg(fmt, ...) printf(prfmt(fmt), ##__VA_ARGS__)
#else
#define prdbg(fmt, ...) do{}while(0)
#endif


/* func : read_timeline_clock_parameters 
 * desc : read timeline clock parameters (mapping and uncertainty) from '/dev/mem' mmap'd pci mmio region */
tl_clockparams_t* read_timeline_clock_parameters(int data_region, __u32 offset)
{
	tl_clockparams_t* data;
	data = (tl_clockparams_t*)upci_read_data(data_region, offset);
	return data;
}

int setup_pci_mmio()
{
	int i, retval, data_region;
	__u16 vendor_id = 0, device_id = 0, ss_vendor_id = 0, ss_device_id = 0, instance = 0;
	struct upci_dev_info info;
	__s32 dev_num;
	int dev_region;

	// PCI Device Information
	vendor_id = 0x1af4; // VendorID
	device_id = 0x1110; // DeviceID
	ss_vendor_id = 0x1af4; // SubVendorID
	ss_device_id = 0x1100; // SubDeviceID
	dev_region = 2; // PCI Region (BAR)

	/* scan pci bus */
	retval = upci_scan_bus();
	if (retval < 0) {
		prerr("PCI bus data missing");
		exit(EXIT_FAILURE);
	}

	/* find the matching device */
	info.vendor_id = vendor_id;
	info.device_id = device_id;
	info.ss_vendor_id = ss_vendor_id;
	info.ss_device_id = ss_device_id;
	info.instance = instance;
	dev_num = upci_find_device(&info);
	if (dev_num < 0) {
		prerr("DeviceID %d, VendorID %d: "
		      "subDeviceID %d, SubVendorID %d: "
		      "Instance %d not found. Quiting...\n",
		      device_id, vendor_id, ss_device_id, ss_vendor_id,
		      instance);
		return -1;
	}

	upci_print_device_info(dev_num);
	
	/* perform pci operation */
	data_region = upci_open_region(dev_num, dev_region);
		
	return data_region;
}