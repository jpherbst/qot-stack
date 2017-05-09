/**
 * @file qot_virt.h
 * @brief Header with virtualization related datatypes (based on QEMU-KVM Virtioserial) 
 * @author Sandeep D'souza
 * 
 * Copyright (c) Carnegie Mellon University, 2017. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *      1. Redistributions of source code must retain the above copyright notice, 
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
 *
 */

#ifndef QOT_STACK_VIRT_C_QOT_H
#define QOT_STACK_VIRT_C_QOT_H

// QoT Datatypes
#include "../qot_types.h"

/**
 * @brief QoT Virt Message Types
 */
typedef enum {   
    TIMELINE_CREATE    = (0),            /* Create a timeline                        */
    TIMELINE_DESTROY   = (1),            /* Destroy a timeline                       */
    TIMELINE_UPDATE    = (2),            /* Update timeline parameters               */
} virtmsg_type_t;

/**
 * @brief QoT Virt Messaging format
 */
typedef struct qot_virtmsg {
    qot_timeline_t info;                 /* Timeline Info                            */
    timequality_t demand;                /* Requested QoT                            */
    virtmsg_type_t msgtype;              /* Message type                             */
    qot_return_t retval;                 /* Return Code (Populated by host daemon)   */
} qot_virtmsg_t;

/**
 * @brief Virt Timeline Data Structure
 */
typedef struct timeline_virt {
	int index;
	int bind_count;
	char filename[15];
} timeline_virt_t;

/**
 * @brief Virt Timeline Clockparams Data Structure
 */
typedef struct timeline_clockparams {
	tl_translation_t translation;       /* Translation parameters (core to timeline)  */
	timequality_t quality;               /* Contains resolution and chieved accuracy   */
} tl_clockparams_t;

#endif