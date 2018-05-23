/**
 * @file Coordinator.hpp
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
#ifndef COORDINATOR_HPP
#define COORDINATOR_HPP

// Boost includes
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/log/trivial.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

// Timeline Message
#include <msg/QoT_DCPS.hpp>

// Synchronization engine
//#include "PTP.hpp"
#include "sync/Sync.hpp"

// Include the QOT api
extern "C"
{
	#include "../qot_types.h"
}

std::ostream& operator <<(std::ostream& os, const qot_msgs::TimelineType& ts);

namespace qot
{
	// Coordinator functionality
	class Coordinator : public dds::sub::NoOpDataReaderListener<qot_msgs::TimelineType>
	{
		// Constructor and destructor
		public: Coordinator(boost::asio::io_service *io, const std::string &name, const std::string &iface, const std::string &addr);
		public: ~Coordinator();

		// Required by dds::sub::NoOpDataReaderListener
		// dds callback. Gets called when new data is available to
		// read. All it does is update the lastcount_ member if the
		// new data is from the master.
		public: virtual void on_data_available(dds::sub::DataReader<qot_msgs::TimelineType>& dr);

		// another dds callback. not sure what it does
		public: virtual void on_liveliness_changed(dds::sub::DataReader<qot_msgs::TimelineType>& dr, 
			const dds::core::status::LivelinessChangedStatus& status);
	
		// Initialize this coordinator with a name
		public: void Start(int id, int fd, const char* uuid, timeinterval_t acc, timelength_t res);

		// Update the target metrics
		public: void Update(timeinterval_t acc, timelength_t res);

		// Stop this coordinator
		public: void Stop();

		// Heartbeat signal for this coordinator
		// This function is periodically called with an asynchronous
		// timer. It checks for timed out master and has logic for
		// electing the master.
		private: void Heartbeat(const boost::system::error_code& err);

		// Asynchronous service
		private: boost::asio::io_service *asio;
		private: boost::asio::deadline_timer timer;

		// Coordinator state
		private: int counter_;   					// count # heartbeats since start
		private: int lastcount_; 					// counter_ value at last time I saw msg from current master
		private: int tml_id_;    					// timeline id, as in /dev/timeline[tml_id_]
		private: int timelinefd; 					// fd for timeline character device
		private: qot_timeline_type_t tl_type;		// timeline type

		// Join the DDS domain to exchange information about timelines
		private: dds::domain::DomainParticipant dp;
		private: dds::topic::Topic<qot_msgs::TimelineType> topic;
		private: dds::pub::Publisher pub;
		private: dds::pub::DataWriter<qot_msgs::TimelineType> dw;
		private: dds::sub::Subscriber sub;
		private: dds::sub::DataReader<qot_msgs::TimelineType> dr;
		private: qot_msgs::TimelineType timeline;

		// Networking Variables
		private: boost::asio::io_service *io;		// IO service
		private: std::string address;				// IP address of the node
	    private: std::string iface;					// Interface type

	    // Clock Sync Class
		private: boost::shared_ptr<Sync> sync;
	};
}

#endif
