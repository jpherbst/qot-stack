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
		if (s->data().uuid().compare(timeline.uuid()) == 0)
		{
			// This message is from somebody else
			if (s->data().name().compare(timeline.name()) != 0)
			{
				//BOOST_LOG_TRIVIAL(info) << "Message received from peer " << s->data().name();

				// If I currently think that I am the master
				if (timeline.master().compare(timeline.name())==0)
				{
					// If the same accuracy is desired at both ends, then there is a collision
					// that we need to resolve. We use a string comparison on the name.
					bool handover = (s->data().accuracy() < timeline.accuracy());
					if (s->data().accuracy() == timeline.accuracy())
					{
						//BOOST_LOG_TRIVIAL(info) << "Same desired accuracy. Resolving conflict";  
					 	if (s->data().name().compare(timeline.name()) < 0)
					 	{
					 		//BOOST_LOG_TRIVIAL(info) << "Peer chosen"; 
					 		handover = true;
					 	}
					 	//else BOOST_LOG_TRIVIAL(info) << "Self chosen"; 
					}

					// But I shouldn't be, because this peer needs better accuracy...
					if (handover)
					{
						BOOST_LOG_TRIVIAL(info) << "The master role should be handed to slave "  
							<< s->data().name() << ":" << s->data().domain();

						// Handover the master ownership to the peer
						timeline.master() = s->data().name();

						// (Re)start the synchronization service as master
						//sync.Start(phc, qotfd, timeline.domain(), false, timeline.accuracy());
						if(timeline.accuracy() > 0){
							int sync_interval = (int) floor(log2(timeline.accuracy()/(20.0*nSEC_PER_SEC)));
							sync->Start(false, sync_interval, timeline.domain(), &timelinefd, 1);
						}
					}
				}

				// If I am a slave, but this node thinks I should be the master
				else if (s->data().master().compare(timeline.name()) == 0)
				{
					BOOST_LOG_TRIVIAL(info) << "Some slave " << s->data().name() <<
						 " thinks that I should be the master on domain " << s->data().domain();

					// Make myself the master and copy over the domain
					timeline.domain() = s->data().domain();
					timeline.master() = timeline.name();

					// (Re)start the synchronization service as master
					//sync.Start(phc, qotfd, timeline.domain(), true, timeline.accuracy());
					if(timeline.accuracy() > 0){
						int sync_interval = (int) floor(log2(timeline.accuracy()/(20.0*nSEC_PER_SEC)));
						sync->Start(true, sync_interval, timeline.domain(), &timelinefd, 1);
					}
				}

				// If I am a slave and this node thinks that it is the master
				else if (s->data().name().compare(s->data().master()) == 0)
				{
					//BOOST_LOG_TRIVIAL(info) << "I am a slave and listening to master "  
					//	<< s->data().name() << ":" << s->data().domain();

					// If the master's domain is different to what I'd expect
					if (s->data().domain() != timeline.domain())
					{
						BOOST_LOG_TRIVIAL(info) << "The domain does not match what I think it is "  
							<< " and so I am restarting the sync service";

						// Set the new domain
						timeline.domain() = s->data().domain();

						// (Re)start the synchronization service
						//sync.Start(phc, qotfd, timeline.domain(), false, timeline.accuracy());
						if(timeline.accuracy() > 0){
							int sync_interval = (int) floor(log2(timeline.accuracy()/(20.0*nSEC_PER_SEC)));
							sync->Start(false, sync_interval, timeline.domain(), &timelinefd, 1);
						}
					}

					// Make sure that we are on the right domain
					timeline.master() = s->data().name();
				}
			}
		}

		// This is a master from some other timeline
		else if (s->data().name().compare(s->data().master()) == 0)
		{
			// If I currently think that I am the master
			if (timeline.master().compare(timeline.name())==0)
			{
				// And our domains collide (this is a bad thing)
				if (s->data().domain() == timeline.domain())
				{
					BOOST_LOG_TRIVIAL(info) << "I am the master and ther is a domain clash on " << s->data().domain();

					// Pick a new random domain in the interval [0, 127]
					timeline.domain() = rand() % 128;

					BOOST_LOG_TRIVIAL(info) << "Switching to " << timeline.domain();

					// (Re)start the synchronization service as master
					//sync.Start(phc, qotfd, timeline.domain(), true, timeline.accuracy());
					// Convert the file descriptor to a clock handle
					if(timeline.accuracy() > 0){
						int sync_interval = (int) floor(log2(timeline.accuracy()/(20.0*nSEC_PER_SEC)));
						sync->Start(true, sync_interval, timeline.domain(), &timelinefd, 1);
					}
				}
			}
		}
	}
}

void  Coordinator::on_liveliness_changed(dds::sub::DataReader<qot_msgs::TimelineType>& dr, 
	const dds::core::status::LivelinessChangedStatus& status) 
{
	// Not sure what to do with this...
}

Coordinator::Coordinator(boost::asio::io_service *io, const std::string &name, const std::string &iface, const std::string &addr)
	: dp(0), topic(dp, "timeline"), pub(dp), dw(pub, topic), sub(dp), dr(sub, topic), 
		timer(*io)/*, sync(io, iface) */
{
	timeline.name() = (std::string) name;	// Our name
	sync = Sync::Factory(io, addr, iface);	// handle to sync algorithm
}

Coordinator::~Coordinator() {}

// Initialize this coordinator with a name
void Coordinator::Start(int id, int fd, const char* uuid, timeinterval_t acc, timelength_t res)
{
	// Set the phc index and file decriptor to qot ioctl
	phc = id;
	//qotfd = fd;
	timelinefd = fd;

	// Set the timeline information
	timeline.uuid() = (std::string) uuid;	
	BOOST_LOG_TRIVIAL(info) << "UUID of timeline is " << timeline.uuid();

	
	//choose the max of the upper and lower bound of accuracy
	BOOST_LOG_TRIVIAL(info) << "Lower Accuracy of timeline is " << acc.below.sec << ", " << acc.below.asec;
	BOOST_LOG_TRIVIAL(info) << "Higher Accuracy of timeline is " << acc.above.sec << ", " << acc.above.asec;
	timelength_t max_acc_bound;
	timelength_max(&max_acc_bound, &acc.above, &acc.below);
	double scalar_acc = (double) max_acc_bound.sec * aSEC_PER_SEC + (double) max_acc_bound.asec;
	timeline.accuracy() = scalar_acc;				// Our accuracy
	BOOST_LOG_TRIVIAL(info) << "timeline accuracy is " << timeline.accuracy();

	BOOST_LOG_TRIVIAL(info) << "Resolution of timeline is " << res.sec << ", " << res.asec;
	double scalar_res = res.sec * aSEC_PER_SEC + res.asec;
	timeline.resolution() = scalar_res;			// Our resolution
	BOOST_LOG_TRIVIAL(info) << "timeline resolution is " << timeline.resolution();
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

	// Stop syncrhonizing
	sync->Stop();

	// Create the listener
	dr.listener(nullptr, dds::core::status::StatusMask::none());
}

// Update the target metrics
void Coordinator::Update(timeinterval_t acc, timelength_t res)
{
	// Update the timeline information
	//choose the max of the upper and lower bound of accuracy
	timelength_t max_acc_bound;
	timelength_max(&max_acc_bound, &acc.above, &acc.below);
	double scalar_acc = max_acc_bound.sec * aSEC_PER_SEC + max_acc_bound.asec;
	timeline.accuracy() = scalar_acc;				// Our accuracy

	double scalar_res = res.sec * aSEC_PER_SEC + res.asec;
	timeline.resolution() = scalar_res;			// Our resolution

	timeline.master() = "";					// Indicates master unknown!

	// If I am a slave then my accuracy may change the sync rate
	if (timeline.master().compare(timeline.name())){
		//sync.Start(phc, qotfd, timeline.domain(), true, timeline.accuracy());
		if(timeline.accuracy() > 0){
			int sync_interval = (int) floor(log2(timeline.accuracy()/(20.0*nSEC_PER_SEC)));
			sync->Start(true, sync_interval, timeline.domain(), &timelinefd, 1);
		}
	}
}

void Coordinator::Heartbeat(const boost::system::error_code& err)
{
	// Fail graciously
	if (err) return;

	// BOOST_LOG_TRIVIAL(info) << "Heartbeat";

	// Send out the timeline information
	dw.write(timeline);

	// Reset the heartbeat timer to be 1s from last firing
	timer.expires_at(timer.expires_at() + boost::posix_time::milliseconds(DELAY_HEARTBEAT));
	timer.async_wait(boost::bind(&Coordinator::Heartbeat, this, boost::asio::placeholders::error));
}

void Coordinator::Timeout(const boost::system::error_code& err)
{
	BOOST_LOG_TRIVIAL(info) << "Initiliaization timeout";

	// Fail graciously
	if (err) return;

	// No master advertised themselve before the timeout period
	if (timeline.master().compare("") == 0)
	{
		BOOST_LOG_TRIVIAL(info) << "I hear no peers, so I am starting as master";

		// Pick a new random domain in the interval [0, 127]
		//timeline.domain() = rand() % 128;
		timeline.domain() = 0;
		timeline.master() = timeline.name();
	}
	else
	{
		BOOST_LOG_TRIVIAL(info) << "I heard another master, so I am starting as a slave.";
	}
	
	// (Re)start the synchronization service as master
	//sync.Start(phc, qotfd, timeline.domain(), true, timeline.accuracy());
	if(timeline.accuracy() > 0){
		int sync_interval = (int) floor(log2(timeline.accuracy()/(20.0*nSEC_PER_SEC)));
		sync->Start(true, sync_interval, timeline.domain(), &timelinefd, 1);
	}

	// Reset the heartbeat timer to be 1s from last firing
	timer.expires_from_now(boost::posix_time::milliseconds(DELAY_HEARTBEAT));
	timer.async_wait(boost::bind(&Coordinator::Heartbeat, this, boost::asio::placeholders::error));
}
