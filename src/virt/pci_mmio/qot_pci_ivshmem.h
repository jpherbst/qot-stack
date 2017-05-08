/**
 * @file qot_pci_ivshmem.h
 * @brief Header for Library which enables guests to read timeline clock metadata (mapping, uncertainty)
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
 */
#ifndef QOT_PCI_IVSHMEM_H
#define QOT_PCI_IVSHMEM_H
#include <linux/types.h>

// Virtualization related data types
#include "../qot_virt.h"

/* func : read_timeline_clock_parameters 
 * desc : read timeline clock parameters (mapping and uncertainty) from '/dev/mem' mmap'd pci mmio region */
tl_clockparams_t* read_timeline_clock_parameters(int data_region, __u32 offset);

/* func : setup_pci_mmio 
 * desc : setup pci mmio for read timeline clock parameters (mapping and uncertainty) from '/dev/mem' mmap'd pci mmio region */
int setup_pci_mmio();

#endif