/**
 * @file qot_virtguest.c
 * @brief Library interface which enables guests to convey timeline metadata to the host (based on QEMU-KVM Virtioserial)
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
 * This program is a skeletal version of "auto-virtserial-guest.c" by:
 *
 *      Amit Shah <amit.shah@redhat.com>
 *      git://fedorapeople.org/home/fedora/amitshah/public_git/test-virtserial.git
 *
 *      Copyright (C) 2009, Red Hat, Inc.
 *
 *      Licensed under the GNU General Public License v2. See the file COPYING
 *      for more details.
 *
 * Some information on the virtio serial ports is available at
 *  http://www.linux-kvm.org/page/VMchannel_Requirements
 *  https://fedoraproject.org/wiki/Features/VirtioSerial
 *
 */

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

// Include QoT-compliant virtualization datatypes
#include "qot_virtguest.h"

// Filename of Virtioserial control port
#define VIRTIO_SERIAL_PORT "/dev/virtio-serial/qot_virthost"

/********************* VirtIO Serial File Operations  **************************************/
static ssize_t safewrite(int fd, const void *buf, size_t count, bool eagain_ret)
{
    size_t ret, len;
    int flags;
    bool nonblock;

    nonblock = false;
    flags = fcntl(fd, F_GETFL);
    if (flags > 0 && flags & O_NONBLOCK)
        nonblock = true;

    len = count;
    while (len > 0) {
        ret = write(fd, buf, len);
        if (ret == -1) {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN) {
                if (nonblock && eagain_ret) {
                    return -EAGAIN;
                } else {
                    continue;
                }
            }
            return -errno;
        } else if (ret == 0) {
            break;
        } else {
            buf += ret;
            len -= ret;
        }
    }
    return count - len;
}

static ssize_t saferead(int fd, void *buf, size_t count, bool eagain_ret)
{
    size_t ret, len;
    int flags;
    bool nonblock;

    nonblock = false;
    flags = fcntl(fd, F_GETFL);
    if (flags > 0 && flags & O_NONBLOCK)
        nonblock = true;

    len = count;
    while (len > 0) {
        ret = read(fd, buf, len);
        if (ret == -1) {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN) {
                if (nonblock && eagain_ret) {
                    return -EAGAIN;
                } else {
                    continue;
                }
            }
            return -errno;
        } else if (ret == 0) {
            break;
        } else {
            buf += ret;
            len -= ret;
        }
    }
    return count - len;
}

static int safepoll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    int ret;

    do {
        ret = poll(fds, nfds, timeout);
    } while (ret == -1 && errno == EINTR);

    if (ret == -1)
        ret = -errno;

    return ret;
}

/*********** Send Timeline Metadata to host ***********************************************/
qot_return_t send_message(qot_virtmsg_t *message)
{
    struct pollfd pollfd[1];
    unsigned int i;
    int ret, cfd;

    if(message == NULL)
        return QOT_RETURN_TYPE_ERR;

    ret = access(VIRTIO_SERIAL_PORT, R_OK|W_OK);

back_to_open:
    cfd = open(VIRTIO_SERIAL_PORT, O_RDWR);
    if (cfd == -1) {
        error(errno, errno, "open control port %s", VIRTIO_SERIAL_PORT);
    }

    ret = safewrite(cfd, message, sizeof(qot_virtmsg_t), false);
    if (ret < 0)
    {
        error(-ret, -ret, "write control port");
        printf("Could not send message\n");
    }

    printf("Waiting for reply from host ...\n");

    // Setup polling file descriptor
    pollfd[0].fd = cfd;
    pollfd[0].events = POLLIN;

    while (1) {
        ret = safepoll(pollfd, 1, -1);
        if (ret < 0)
            error(errno, errno, "poll");

        if (!(pollfd[0].revents & POLLIN))
            continue;

        ret = saferead(cfd, message, sizeof(qot_virtmsg_t), false);
        if (ret < sizeof(qot_virtmsg_t)) {
            /*
             * Out of sync with host. Close port and start over.
             * For us to get back in sync with host, this port
             * has to have buffer cachin disabled
             */
            close(cfd);
            goto back_to_open;
        }
        else
        {
            break;
        }
    }
    return QOT_RETURN_TYPE_OK;
}