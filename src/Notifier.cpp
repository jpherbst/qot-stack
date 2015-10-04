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
	: asio(io)
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

	        // Check to see if this is a character device, and add it
	        if (entry->d_type & DT_DIR)
	        {
	        	// DO NOTHING
	        }
	        else
	        {
				// Append the directory to the name 
				std::ostringstream oss("");
				oss << dir;
				oss << "/";
				oss << entry->d_name;

				// Add the directory
				this->add(oss.str());
	        }
	     }
    }
    
    /* After going through all the entries, close the directory. */
    if (closedir(d))
    	BOOST_LOG_TRIVIAL(error) << "Could not close the directory " << dir;

	/* Create INOTIFY instance and check for error */
	fd = inotify_init();
	if (fd < 0)
		BOOST_LOG_TRIVIAL(error) << "Could not open the directory " << dir;
	else
		thread = boost::thread(boost::bind(&Notifier::watch, this, dir.c_str()));
}

Notifier::~Notifier()
{
	// Close the INOTIFY instance
	close(fd);

	// Join the listen thread
	thread.join();
}

void Notifier::add(const std::string &path)
{
	BOOST_LOG_TRIVIAL(info) << "New timeline detected at " << path;
	std::map<std::string,Timeline>::iterator it = timelines.find(path);
	if (it == timelines.end())
		timelines.insert(std::map<std::string,Timeline>::value_type(path,Timeline(asio, path)));
	else
		BOOST_LOG_TRIVIAL(warning) << "The timeline has already been added";
}

void Notifier::del(const std::string &path)
{
	BOOST_LOG_TRIVIAL(info) << "Timeline deletion detected at " << path;
	std::map<std::string,Timeline>::iterator it = timelines.find(path);
	if (it != timelines.end())
		timelines.erase(it);
	else
		BOOST_LOG_TRIVIAL(warning) << "No such timeline exists locally";
}

void Notifier::watch(const char* dir)
{
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
			break;
		else if (pfds.revents & POLLIN)
		{
			// Read to determine the event change happens on watched directory
			int length = read(fd, buffer, EVENT_BUF_LEN); 
			if (length < 0)
			{
				close(fd);
				return;
			}  

			// Read return the list of change events happens. 
			int i = 0;
			while (i < length)
			{     
				struct inotify_event *event = (struct inotify_event *) &buffer[i];    

				// Append the directory to the name 
				std::ostringstream oss("");
				oss << dir;
				oss << "/";
				oss << event->name;
				    
				if (event->len)
				{
					if (event->mask & IN_CREATE)
					{
						if (event->mask & IN_ISDIR)
							BOOST_LOG_TRIVIAL(warning) << "Warning: unexpected creation of directory " << oss.str();
						else
							this->add(oss.str());
					}
					else if (event->mask & IN_DELETE)
					{
						if (event->mask & IN_ISDIR) 
							BOOST_LOG_TRIVIAL(warning) << "Warning: unexpected deletion of directory " << oss.str();
						else
							this->del(oss.str());
					}
				}

				// Append size to determine when one should end
				i += EVENT_SIZE + event->len;
			}
		}
	}

	// Remove the watch directory from the watch list.
	inotify_rm_watch(fd, wd);
}
