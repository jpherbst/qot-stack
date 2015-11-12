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

// Delays
#define DELAY_HEARTBEAT 		1000
#define DELAY_INITIALIZING		5000

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

void Coordinator::on_data_available(dds::sub::DataReader<qot_msgs::TimelineType>& dr) 
{ 
	// Iterate over all samples
	for (auto& s : dr.read())
	{
		// If this is the timeline of interest
		if (s->data().uuid().compare(this->timeline.uuid()) == 0)
		{
			// This message is from somebody else
			if (s->data().name().compare(this->timeline.name()) != 0)
			{
				// If I currently think that I am the master
				if (this->timeline.master().compare(this->timeline.name())==0)
				{
					// But I shouldn't be, because this peer needs better accuracy...
					if (s->data().accuracy() < this->timeline.accuracy());
					{
						// Handover the master ownership to the node
						this->timeline.master() = s->data().name();

						// Become a slave
						// this->sync.Slave();
					}
				}

				// If I am a slave, but this node thinks I should be the master
				else if (s->data().master().compare(this->timeline.name()) == 0)
				{
					// Make myself the master and copy over the domain
					this->timeline.domain() = s->data().domain();
					this->timeline.master() = this->timeline.name();

					// Become the master
					// this->sync.Master();
				}

				// If I am a slave and this node thinks that it is the master
				else if (s->data().name().compare(s->data().master()) == 0)
				{
					// Make sure that we are on the right domain
					// this->sync.Start(s->data().domain());
				}
			}
		}

		// This is a master from some other timeline
		else if (s->data().name().compare(s->data().master()) == 0)
		{
			// If I currently think that I am the master
			if (this->timeline.master().compare(this->timeline.name())==0)
			{
				// And our domains collide (this is a bad thing)
				if (s->data().domain() == this->timeline.domain())
				{
					// Pick a new random domain in the interval [0, 128]
					this->timeline.domain() = rand() % 128;

					// Switch PTP domain
					// this->sync.Start(this->timeline.domain());
				}
			}
		}
	}
}

void  Coordinator::on_liveliness_changed(dds::sub::DataReader<qot_msgs::TimelineType>& dr, 
	const dds::core::status::LivelinessChangedStatus& status) 
{
	// Not sure what to fo with this...
}

Coordinator::Coordinator(boost::asio::io_service *io, const std::string &name)
	: dp(0), topic(dp, "timeline"), pub(dp), dw(pub, topic), sub(dp), dr(sub, topic), 
		timer(*io), sync(io)
{
	timeline.name() = (std::string) name;	// Our name
}

Coordinator::~Coordinator() {}

// Initialize this coordinator with a name
void Coordinator::Start(int id, const char* uuid, double acc, double res)
{
	// Set the timeline information
	timeline.uuid() = (std::string) uuid;	
	timeline.accuracy() = acc;				// Our accuracy
	timeline.resolution() = res;			// Our resolution
	timeline.master() = "";					// Indicates master unknown!

	// Create the listener
	dr.listener(this, dds::core::status::StatusMask::data_available());

	// Start the state timer to wait for peers
	timer.expires_from_now(boost::posix_time::milliseconds(DELAY_INITIALIZING));
  	timer.async_wait(boost::bind(&Coordinator::Timeout, this,  boost::asio::placeholders::error));
}

// Initialize this coordinator with a name
void Coordinator::Stop()
{
	// Stop the timeout and heartbeat timers
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

	// Reset the heartbeat timer to be 1s from last firing
	timer.expires_at(timer.expires_at() + boost::posix_time::milliseconds(DELAY_HEARTBEAT));
	timer.async_wait(boost::bind(&Coordinator::Heartbeat, this, boost::asio::placeholders::error));
}

void Coordinator::Timeout(const boost::system::error_code& err)
{
	// Fail graciously
	if (err) return;

	// No master advertised themselve before the timeout period
	if (this->timeline.master().compare("") == 0)
	{
		// Pick a new random domain in the interval [0, 128]
		this->timeline.domain() = rand() % 128;

		// Switch PTP domain
		//this->sync.Start(this->timeline.domain());

		// Switch PTP domain
		//this->sync.Master();
	}

	// Reset the heartbeat timer to be 1s from last firing
	timer.expires_from_now(boost::posix_time::milliseconds(DELAY_HEARTBEAT));
	timer.async_wait(boost::bind(&Coordinator::Heartbeat, this, boost::asio::placeholders::error));
}
