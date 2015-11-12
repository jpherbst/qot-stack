/*
 * @file qotdaemon.c
 * @brief System daemon to manage timelies for the Linux QoT stackl
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

extern "C"
{
	#include <time.h>
}

// Basic printing
#include <iostream>
#include <fstream>
#include <string>

// Boost includes
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

// Notification
#include "Notifier.hpp"

// Convenience
using namespace qot;

// Generate a random tring
std::string RandomString(uint32_t length)
{
    auto randchar = []() -> char
    {
        const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    std::string str(length,0);
    std::generate_n( str.begin(), length, randchar );
    return str;
}

// Main entry point of application
int main(int argc, char **argv)
{
	// Parse command line options
	boost::program_options::options_description desc("Allowed options");
	desc.add_options()
		("help,h",  	"produce help message")
		("verbose,v",  	"print verbose debug messages")
		("iface,i",  	boost::program_options::value<std::string>()
			->default_value("eth0"), "PTP-compliant interface") 
		("name,n", 		boost::program_options::value<std::string>()
			->default_value(RandomString(32)), "name of this node") 
	;
	boost::program_options::variables_map vm;
	boost::program_options::store(
		boost::program_options::parse_command_line(argc, argv, desc), vm);
	boost::program_options::notify(vm);    

	// Set logging level
	if (vm.count("verbose") > 0)
    {
    	boost::log::core::get()->set_filter
	    (
	        boost::log::trivial::severity >= boost::log::trivial::info
	    );
	}
	else
	{
    	boost::log::core::get()->set_filter
	    (
	        boost::log::trivial::severity >= boost::log::trivial::warning
	    );
	}

	// Print some help with arguments
	if (vm.count("help") > 0)
	{
		std::cout << desc << "\n";
		return 0;
	}

	// Create an IO service
	boost::asio::io_service io;
	boost::asio::io_service::work work(io);

	// Some friendly debug
	BOOST_LOG_TRIVIAL(info) << "My UNIQUE name is " << vm["name"].as<std::string>() 
		<< " and I will perform synchronization over interface " << vm["iface"].as<std::string>();

	// Create the inotify monitoring dservice for /dev/timelineX and incoming DDS messages
	qot::Notifier notifier(&io, vm["name"].as<std::string>(), vm["iface"].as<std::string>());

	// Run the io service
	io.run();

	// Everything OK
	return 0;
}
