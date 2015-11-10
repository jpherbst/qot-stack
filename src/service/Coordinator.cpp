/*
 * @file Coordinator.cpp
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
#include "Coordinator.hpp"

using namespace qot;

std::ostream& operator <<(std::ostream& os, const qot_msgs::TimelineType& ts)
{
  os << "(name = " << ts.name() 
	 << ", uuid = " << ts.uuid()
	 << ", accuracy = " << ts.accuracy()
	 << ", resolution = " << ts.resolution()
	 << ")";
  return os;
}

void TimelineListener::on_data_available(dds::sub::DataReader<qot_msgs::TimelineType>& dr) 
{ 
	std::cout << "----------on_data_available-----------" << std::endl;      
	auto samples =  dr.read();
	std::for_each(samples.begin(), samples.end(), [](
		const dds::sub::Sample<qot_msgs::TimelineType>& s)
		{
			std::cout << s.data() << std::endl;
		}
	);
}

void  TimelineListener::on_liveliness_changed(dds::sub::DataReader<qot_msgs::TimelineType>& dr, 
	const dds::core::status::LivelinessChangedStatus& status) 
{
	std::cout << "!!! Liveliness Changed !!!" << std::endl;
}

Coordinator::Coordinator(boost::asio::io_service *io, const std::string &name)
	: dp(0), topic(dp, "timeline"), pub(dp), dw(pub, topic), sub(dp), dr(sub, topic),
		timer(*io, boost::posix_time::seconds(1))
{
	// Set the name
	timeline.name() = (std::string) name;
}

Coordinator::~Coordinator() {}

// Initialize this coordinator with a name
void Coordinator::Start(const char* uuid, double acc, double res)
{
	// Set the timeline information
	timeline.uuid() = (std::string) uuid;
	timeline.accuracy() = acc;
	timeline.resolution() = res;

	// Create the listener
	dr.listener(&this->listener, dds::core::status::StatusMask::data_available());

	// Start the heartbeat timer
  	timer.async_wait(boost::bind(&Coordinator::Heartbeat, this,  boost::asio::placeholders::error));
}

// Initialize this coordinator with a name
void Coordinator::Stop()
{
	// Stop the heartbeat
	timer.cancel();

	// Create the listener
	dr.listener(nullptr, dds::core::status::StatusMask::none());
}

// Update the target metrics
void Coordinator::Update(double acc, double res)
{
	// Update the timeline information
	timeline.accuracy() = acc;
	timeline.resolution() = res;
}

void Coordinator::Heartbeat(const boost::system::error_code& err)
{
	// Fail graciously
	if (err) return;

	// Send out the timeline information
	dw.write(timeline);

	// Reset the heartbeat timer
	timer.expires_at(timer.expires_at() + boost::posix_time::seconds(1));
	timer.async_wait(boost::bind(&Coordinator::Heartbeat, this, boost::asio::placeholders::error));
}
