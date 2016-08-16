/*
 * @file Notifier.cpp
 * @brief Library to manage Quality of Time POSIX clocks
 * @author Andrew Symington
 * 
 * Copyright (c) Regents of the University of California, 2015. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 	1. Redistributions of source code must retain the above copyright notice, 
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
 */

/* This file header */
#include "Notifier.hpp"

/* System dependencies */
extern "C"
{
	// Standard C includes
	#include <stdio.h>
	#include <stdlib.h>
	#include <errno.h>
	#include <poll.h>
	#include <dirent.h>
	#include <sys/types.h>
	#include <sys/inotify.h>

	// So that we know the character device for the timelin
	#include "../qot_types.h"
}

/* Convenience declarations */
#define EVENT_SIZE  	(sizeof(struct inotify_event))
#define EVENT_BUF_LEN   (1024*(EVENT_SIZE + 16))
#define EVENT_MAXLEN 	(512)
#define	EVENT_TIMEOUT	(1000)

/* So that we might expose a meaningful name through PTP interface */
#define QOT_IOCTL_BASE          "/dev"
#define QOT_IOCTL_TIMELINE      "timeline"
#define QOT_IOCTL_FORMAT        "%8s%d"
#define QOT_MAX_NAMELEN         64

using namespace qot;

// Constructor
Notifier::Notifier(boost::asio::io_service *io, const std::string &name, const std::string &iface, const std::string &addr) 
	: asio(io), basename(name), baseiface(iface), baseaddr(addr)
{
	BOOST_LOG_TRIVIAL(info) << "Starting the notifier";

	/* First, add all existing timelines in the system */
	DIR * d = opendir(QOT_IOCTL_BASE);
	if (!d)
	{
		BOOST_LOG_TRIVIAL(error) << "Could not open the directory " << QOT_IOCTL_BASE;
		return;
	}
	else
	{
		struct dirent *entry;
		// "Readdir" gets subsequent entries from "d"
		while (entry = readdir(d))
		{
			// If this event is the creation or deletion of a character device
			char str[QOT_MAX_NAMELEN]; 
			int ret, val;	
			ret = sscanf(entry->d_name, QOT_IOCTL_FORMAT, str, &val);
			if ((ret == 2) && (strncmp(str,QOT_IOCTL_TIMELINE,QOT_MAX_NAMELEN)==0))
				this->add(val);
		}
	}

	/* After going through all the entries, close the directory. */
	if (closedir(d))
		BOOST_LOG_TRIVIAL(error) << "Could not close the directory " << QOT_IOCTL_BASE;

	BOOST_LOG_TRIVIAL(info) << "Watching directory " << QOT_IOCTL_BASE;
	thread = boost::thread(boost::bind(&Notifier::watch, this, QOT_IOCTL_BASE));
}

Notifier::~Notifier()
{
	// Join the listen thread
	thread.join();
}

void Notifier::add(int id)
{
	BOOST_LOG_TRIVIAL(info) << "New timeline detected at " 
	                        << QOT_IOCTL_BASE << "/" << QOT_IOCTL_TIMELINE << id;

	// If we get to this point then this is a valid timeline
	std::map<int,Timeline*>::iterator it = timelines.find(id);
	if (it == timelines.end())
		timelines[id] = new Timeline(asio, basename, baseiface, baseaddr, id);
	else
		BOOST_LOG_TRIVIAL(warning) << "The timeline already exists in our data structure";
}

void Notifier::del(int id)
{
	BOOST_LOG_TRIVIAL(info) << "Timeline deletion detected at " 
	                        << QOT_IOCTL_BASE << "/" << QOT_IOCTL_TIMELINE << id;

	std::map<int,Timeline*>::iterator it = timelines.find(id);
	if (it != timelines.end())
	{
		delete it->second;
		timelines.erase(it);
	}
	else
		BOOST_LOG_TRIVIAL(warning) << "No such timeline exists locally";
}

void Notifier::watch(const char* dir)
{
	char buffer[EVENT_BUF_LEN];

	/* Create INOTIFY instance and check for error */
	int fd = inotify_init();
	if (fd < 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Could not open the directory " << dir;
		return;
	}

	// Monitor the /clk directory for creation and deletion 
	int wd = inotify_add_watch(fd, dir, IN_CREATE | IN_DELETE);

	// Poll
	struct pollfd pfds = { .fd = fd, .events = POLLIN };

	// Keep watching indefinitely
	while (true)
	{
		// Wait for incoming data 
		int ret = poll(&pfds, 1, EVENT_TIMEOUT);
		if (ret < 0)
		{
			BOOST_LOG_TRIVIAL(warning) << "Warning: poll error ";
			break;
		}
		else if (pfds.revents & POLLIN)
		{
			// Read to determine the event change happens on watched directory
			int length = read(fd, buffer, EVENT_BUF_LEN);
			if (length < 0)
			{
				BOOST_LOG_TRIVIAL(warning) << "Warning: read error ";
				close(fd);
				return;
			}

			// Read return the list of change events happens. 
			int i = 0;
			while (i < length)
			{
				struct inotify_event *event = (struct inotify_event *) &buffer[i];    

				// If this event is the creation or delation of a character device
				char str[QOT_MAX_NAMELEN]; 
				int ret, val;	
				ret = sscanf(event->name, QOT_IOCTL_FORMAT, str, &val);
				if (event->len && (ret==2) && (strncmp(str,QOT_IOCTL_TIMELINE,QOT_MAX_NAMELEN)==0))
				{
					// Creation
					if (event->mask & IN_CREATE)
						this->add(val);

					// Deletion
					if (event->mask & IN_DELETE) 
						this->del(val);
				}

				// Append size to determine when one should end
				i += EVENT_SIZE + event->len;
			}
		}
	}

	// Remove the watch directory from the watch list.
	inotify_rm_watch(fd, wd);

	// Close the INOTIFY instance
	close(fd);

}
