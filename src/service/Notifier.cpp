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

/* Trivial logging */
#include <boost/log/trivial.hpp>

using namespace qot;

Notifier::Notifier(boost::asio::io_service *io, const std::string &dir)
	: asio(io), basedir(dir)
{
	BOOST_LOG_TRIVIAL(info) << "Starting the notifier at directory " << dir;

	/* First, add all existing timelines in the system */
  	DIR * d = opendir(dir.c_str());
    if (!d)
    {
    	BOOST_LOG_TRIVIAL(error) << "Could not open the directory " << dir;
    }
    else
    {
	    while (1)
	    {
	        // "Readdir" gets subsequent entries from "d"
	        struct dirent *entry = readdir(d);
	        if (!entry)
	            break;

			// If this event is the creation or delation of a character device
			char str[8]; 
			int ret, val;	
			ret = sscanf(entry->d_name, "%8s%d", str, &val);
	        if ((ret==2) && (strcmp(str,"timeline")==0))
    			this->add(entry->d_name);
  	     }
    }
    
    /* After going through all the entries, close the directory. */
    if (closedir(d))
    	BOOST_LOG_TRIVIAL(error) << "Could not close the directory " << dir;

	BOOST_LOG_TRIVIAL(info) << "Watching directory " << dir;
	thread = boost::thread(boost::bind(&Notifier::watch, this, dir.c_str()));
}

Notifier::~Notifier()
{
	// Join the listen thread
	thread.join();
}

void Notifier::add(const char *name)
{
	std::ostringstream oss("");
	oss << basedir << "/" << name;
	BOOST_LOG_TRIVIAL(info) << "New timeline detected at " << oss.str();
	std::map<std::string,Timeline>::iterator it = timelines.find(oss.str());
	if (it == timelines.end())
		timelines.emplace(oss.str(),Timeline(asio, oss.str()));
	else
		BOOST_LOG_TRIVIAL(warning) << "The timeline has already been added";
}

void Notifier::del(const char *name)
{
	std::ostringstream oss("");
	oss << basedir << "/" << name;
	BOOST_LOG_TRIVIAL(info) << "Timeline deletion detected at " << oss.str();
	std::map<std::string,Timeline>::iterator it = timelines.find(oss.str());
	if (it != timelines.end())
		timelines.erase(it);
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
		//else if (pfds.revents & POLLIN)
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
				char str[8]; 
				int ret, val;	
				ret = sscanf(event->name, "%8s%d", str, &val);
				if (event->len && (ret==2) && (strcmp(str,"timeline")==0))
				{
					// Creation
					if (event->mask & IN_CREATE)
						this->add(event->name);

					// Deletion
					if (event->mask & IN_DELETE) 
						this->del(event->name);
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