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

#include <vector>
#include <string>

// Delays (milliseconds)
#define DELAY_HEARTBEAT 		1000
#define DELAY_INITIALIZING		5000

// # heartbeats timeout for master
static const int MASTER_TIMEOUT = 3; // (in units of DELAY_HEARTBEAT)

static const int NEW_MASTER_WAIT = 3;

using namespace qot;

std::ostream& operator <<(std::ostream& os, const qot_msgs::TimelineType& ts)
{
	os << "(name = "        << ts.name() 
	   << ", uuid = "       << ts.uuid()
	   << ", accuracy = "   << ts.accuracy()
	   << ", resolution = " << ts.resolution()
	   << ")";
	return os;
}

void Coordinator::on_data_available(dds::sub::DataReader<qot_msgs::TimelineType>& dr) 
{
	//BOOST_LOG_TRIVIAL(info) << "on_data_available() called";

	// get only new/unread data
	for (auto &s : dr.select().state(dds::sub::status::SampleState::not_read()).read()) {
		// if it's from master and I'm not the master update his last count
		if (s->data().name() == timeline.master()
		    && timeline.master() != timeline.name()) {
			lastcount_ = counter_;
		}
	}
}

void Coordinator::on_liveliness_changed(dds::sub::DataReader<qot_msgs::TimelineType>& dr, 
	const dds::core::status::LivelinessChangedStatus& status) 
{
	// Not sure what to do with this...

	// according to C++ API reference, this function gets called when the
	// 'liveliness' of a DataWriter writing to this DataReader has changed
	// "changed" meaning has become 'alive' or 'not alive'.
	// 
	// I think this can detect when new writers have joined the board/topic
	// and when current writers have died. (maybe)
	//   - Sean Kim

	BOOST_LOG_TRIVIAL(info) << "on_liveliness_changed() called.";
}

Coordinator::Coordinator(boost::asio::io_service *io, const std::string &name, const std::string &iface, const std::string &addr)
	: dp(0), topic(dp, "timeline"), pub(dp), dw(pub, topic), sub(dp), dr(sub, topic), 
	  timer(*io),
	  counter_(0), lastcount_(0)
{
	timeline.name() = (std::string) name;	// Our name
	sync = Sync::Factory(io, addr, iface);	// handle to sync algorithm
}

Coordinator::~Coordinator() {}

// Initialize this coordinator with a name
void Coordinator::Start(int id, int fd, const char* uuid, timeinterval_t acc, timelength_t res)
{
	// Set the tml id and file decriptor to qot ioctl
	tml_id_ = id;
	timelinefd = fd;

	// Set the timeline information
	timeline.uuid() = (std::string) uuid;	
	BOOST_LOG_TRIVIAL(info) << "[T" << tml_id_ << "] UUID of timeline is " << timeline.uuid();

	
	//choose the max of the upper and lower bound of accuracy
	BOOST_LOG_TRIVIAL(info) << "[T" << tml_id_ << "] Lower Accuracy of timeline is " << acc.below.sec << ", " << acc.below.asec;
	BOOST_LOG_TRIVIAL(info) << "[T" << tml_id_ << "] Higher Accuracy of timeline is " << acc.above.sec << ", " << acc.above.asec;
	timelength_t max_acc_bound;
	timelength_min(&max_acc_bound, &acc.above, &acc.below);
	double scalar_acc = (double) max_acc_bound.sec * aSEC_PER_SEC + (double) max_acc_bound.asec;
	timeline.accuracy() = scalar_acc;				// Our accuracy
	BOOST_LOG_TRIVIAL(info) << "[T" << tml_id_ << "] timeline accuracy is " << timeline.accuracy();

	BOOST_LOG_TRIVIAL(info) << "[T" << tml_id_ << "] Resolution of timeline is " << res.sec << ", " << res.asec;
	double scalar_res = res.sec * aSEC_PER_SEC + res.asec;
	timeline.resolution() = scalar_res;			// Our resolution
	BOOST_LOG_TRIVIAL(info) << "[T" << tml_id_ << "] timeline resolution is " << timeline.resolution();
	timeline.master() = "";					// Indicates master unknown!

	// Create the listener
	dr.listener(this, dds::core::status::StatusMask::data_available());

	// Start the state timer to wait for peers
	//timer.expires_from_now(boost::posix_time::milliseconds(DELAY_INITIALIZING));
	//timer.async_wait(boost::bind(&Coordinator::Timeout, this,  boost::asio::placeholders::error));
	timer.expires_from_now(boost::posix_time::milliseconds(DELAY_INITIALIZING));
	timer.async_wait(boost::bind(&Coordinator::Heartbeat, this,  boost::asio::placeholders::error));
}

// Initialize this coordinator with a name
void Coordinator::Stop()
{
	// Stop the timeout and heartbeat timers
	timer.cancel();

	// Stop synchronizing
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
	if (timeline.master().compare(timeline.name())) {
		//sync.Start(tml_id_, qotfd, timeline.domain(), true, timeline.accuracy());
		if(timeline.accuracy() > 0) {
			int sync_interval = (int) floor(log2(timeline.accuracy()/(2.0*ASEC_PER_USEC)));
			sync->Start(true, sync_interval, timeline.domain(), tml_id_, &timelinefd, 1);
		}
	}
}

void Coordinator::Heartbeat(const boost::system::error_code& err)
{
	// Fail graciously
	if (err) {
		BOOST_LOG_TRIVIAL(info) << "heartbeat got error. returning";
		return;
	}

	counter_ += 1; // increment heartbeat counter

	// Check for master timeout, but only if I am a slave and actually
	// have a master
	if (timeline.name() != timeline.master()         // I am slave
	    && !timeline.master().empty()                // I have a master
	    && counter_ - lastcount_ > MASTER_TIMEOUT) { // Master timeout

		BOOST_LOG_TRIVIAL(info) << "master timed out. resetting";

		// get rid of old samples
		// TODO: ideally just 'take' the timed out master's samples
		dr.take();

/*
		std::vector<std::string> params(1);
		params[0] = timeline.master();

		dds::sub::cond::QueryCondition cond = 
			dr.create_querycondition(
				DDS::READ_SAMPLE_STATE | DDS::NOT_READ_SAMPLE_STATE,
				DDS::NEW_VIEW_STATE | DDS::NOT_NEW_VIEW_STATE,
				DDS::ALIVE_INSTANCE_STATE | DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE | DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE,
				"name LIKE %1", params);
		qot_msgs::TimelineTypeSeq data;
		DDS::SampleInfoSeq info;
		dr.take_w_condition(data, info, cond);

		for (auto &s : data) {
			BOOST_LOG_TRIVIAL(info) << "found take_w_condition data with name "
			                        << s->data().name();
		}
*/

		// update my master as undecided
		timeline.master() = "";
		lastcount_ = counter_;
	}

	// Send out the timeline information
	dw.write(timeline);

	// tmp vars for finding new master if there needs to be
	double      minacc    = timeline.accuracy();
	int         newdomain = timeline.domain();
	std::string newmaster = timeline.master();

	// If we recently lost a master
	if (timeline.master() == "" && counter_ - lastcount_ < NEW_MASTER_WAIT)
		goto wait_for_next_heartbeat;

	// if no master, then start out assuming it'll be me
	if (newmaster.empty())
		newmaster = timeline.name();

	// Iterate over all samples
	for (auto& s : dr.read()) {
		// If this is NOT the timeline of interest
		if (s->data().uuid() != timeline.uuid()) {
			// check for domain collision
			if (s->data().domain() == timeline.domain()      // Domains collide
			    && timeline.name() == timeline.master()      // I am a master
			    && s->data().name() == s->data().master()) { // He is also a master
				// we want to change the domain
				// TODO: maybe just have the node with 'lower' name change his instead of having both change?
				BOOST_LOG_TRIVIAL(info) << "[T" << tml_id_ << "] I am the master and there is a domain clash on " << s->data().domain();

				// Pick a new random domain in the interval [0, 127]
				timeline.domain() = rand() % 128;

				BOOST_LOG_TRIVIAL(info) << "[T" << tml_id_ << "] Switching to " << timeline.domain();

				// (Re)start the synchronization service as master
				//sync.Start(tml_id_, qotfd, timeline.domain(), true, timeline.accuracy());
				// Convert the file descriptor to a clock handle
				if(timeline.accuracy() > 0){
					int sync_interval = (int) floor(log2(timeline.accuracy()/(2.0*ASEC_PER_USEC)));
					sync->Start(true, sync_interval, timeline.domain(), tml_id_, &timelinefd, 1);
				}
			}

			// else nothing to do, since this is not my timeline
			continue;
		}

		// This is my timeline

		// Ignore if this is my own msg
		if (s->data().name() == timeline.name())
			continue;

		// check accuracy req
		if (s->data().accuracy() < minacc) { // found new min accuracy
			// update
			minacc = s->data().accuracy();
			newmaster = s->data().name();
			newdomain = s->data().domain();
		} else if (s->data().accuracy() == minacc     // equal
		           && s->data().name() < newmaster) { // alphabetically
			newmaster = s->data().name();
			newdomain = s->data().domain();
		}
	}

	// done processing data
	// see if we've found a new master
	if (newmaster != timeline.master()) {
		BOOST_LOG_TRIVIAL(info) << "[T" << tml_id_ << "] new master should be " << newmaster;
		timeline.master() = newmaster;
		timeline.domain() = newdomain;

		BOOST_LOG_TRIVIAL(info) << "[T" << tml_id_ << "] restarting sync service";
		// restart sync service as master or slave depending on whether master == my name
		if(timeline.accuracy() > 0){
			int sync_interval = (int) floor(log2(timeline.accuracy()/(2.0*ASEC_PER_USEC)));
			sync->Start(timeline.master() == timeline.name(), sync_interval, timeline.domain(), tml_id_, &timelinefd, 1);
		}
	}

wait_for_next_heartbeat:
	// Reset the heartbeat timer to be 1s from last firing
	timer.expires_at(timer.expires_at() + boost::posix_time::milliseconds(DELAY_HEARTBEAT));
	timer.async_wait(boost::bind(&Coordinator::Heartbeat, this, boost::asio::placeholders::error));
}
