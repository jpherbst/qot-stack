/*
 * @file Messenger.cpp
 * @brief Library to manage Inter-Process Notifications for Distributed QoT based Coordinated Applications
 * @author Sandeep D'souza
 * 
 * Copyright (c) Carnegie Mellon University, 2016. All rights reserved.
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
#include "Messenger.hpp"

#include <vector>
#include <string>

// Delays (milliseconds)
#define DELAY_HEARTBEAT 		1000
#define DELAY_INITIALIZING		5000

// # heartbeats timeout for master
static const int MASTER_TIMEOUT = 3; // (in units of DELAY_HEARTBEAT)

static const int NEW_MASTER_WAIT = 3;

using namespace qot;

std::ostream& operator <<(std::ostream& os, const qot_msgs::TimelineMsgingType& tms)
{
	os << "(name = "        << tms.name() 
	   << ", uuid = "       << tms.uuid()
	   << ")";
	return os;
}

// Need to populate these functions
void Messenger::add_to_subscribed_msg_type_list(qot_msg_type_t type)
{
	return;
}

// Need to populate these functions
void Messenger::remove_from_subscribed_msg_type_list(qot_msg_type_t type)
{
	return;
}

void Messenger::on_data_available(dds::sub::DataReader<qot_msgs::TimelineMsgingType>& dr) 
{
	// get only new/unread data
	dds::sub::status::DataState aliveDataState;
            aliveDataState << dds::sub::status::SampleState::any()
                << dds::sub::status::ViewState::any()
                << dds::sub::status::InstanceState::alive();

    /**
     * Take messages. Using take instead of read removes the messages from
     * the system, preventing resources from being saturated due to a build
     * up of messages
     */
    dds::sub::LoanedSamples<qot_msgs::TimelineMsgingType> messages
        = dr.select().state(aliveDataState).take();

    /** Output the username and content for each message */
    for (dds::sub::LoanedSamples<qot_msgs::TimelineMsgingType>::const_iterator message
            = messages.begin(); message < messages.end(); ++message)
    {
        if(message->info().valid())
        {
            BOOST_LOG_TRIVIAL(info) << "Message Received from " << message->data().name() << " says " << message->data().type();
        }
    }
	// for (auto &s : dr.select().state(dds::sub::status::SampleState::not_read()).take()) {
	// 	BOOST_LOG_TRIVIAL(info) << "Message Received from " << s->data().name() << " says " << s->data().type();
	// }
	//BOOST_LOG_TRIVIAL(info) << "on_data_available() called";
}

void Messenger::on_liveliness_changed(dds::sub::DataReader<qot_msgs::TimelineMsgingType>& dr, 
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

Messenger::Messenger(const std::string &name, const std::string &uuid)
	: pub_entity(), sub_entity(),//dp(0), topic(dp, "timeline_messaging"), pub(dp), dw(pub, topic), sub(dp), dr(sub, topic),
	name(name), uuid(uuid), cluster_manager(name, uuid)
{
	// Create the message listener
	//dr.listener(this, dds::core::status::StatusMask::data_available());
	sub_entity.MessageReader.listener(this, dds::core::status::StatusMask::data_available());

	// Add this node to the NameService
	name_msg.userID() = rand();          // Give a user ID -> this has to be changed later should be automated
	name_msg.name() = name;
	pub_entity.nameServiceWriter << name_msg;

	BOOST_LOG_TRIVIAL(info) << "Messenger Initialized for node " << timeline_msg.name();
}

Messenger::~Messenger() 
{
	// Cancel the listener
	//dr.listener(nullptr, dds::core::status::StatusMask::none());
	sub_entity.MessageReader.listener(nullptr, dds::core::status::StatusMask::none());
	cluster_manager.~ClusterManager();
}

qot_return_t Messenger::Publish(const qot_message_t msg)
{
	qot_msgs::UncertainTimestamp utimestamp;

	// Unroll Message Timestamp (resolution is in ns -> convert to asec later)
	utimestamp.timestamp()     = msg.timestamp.estimate.sec*nSEC_PER_SEC + msg.timestamp.estimate.asec/ASEC_PER_NSEC;
	utimestamp.uncertainty_u() = msg.timestamp.interval.above.sec*nSEC_PER_SEC + msg.timestamp.interval.above.asec/ASEC_PER_NSEC;
	utimestamp.uncertainty_l() = msg.timestamp.interval.below.sec*nSEC_PER_SEC + msg.timestamp.interval.below.asec/ASEC_PER_NSEC;
	
	// Unroll message parameters
	timeline_msg.name() = (std::string) name;			      // Our name
	timeline_msg.uuid() = (std::string) uuid;   			  // Timeline UUID
	timeline_msg.type() = (qot_msgs::MsgType) msg.type;  	  // Msg Type
	timeline_msg.data() = std::string(msg.data);              // Msg Data
	timeline_msg.utimestamp() = utimestamp;                   // Timestamp Associated with Msg
	
	// Publish the message
	pub_entity.MessageWriter.write(timeline_msg);
	return QOT_RETURN_TYPE_OK;  
}

// Subscribe to Messages
qot_return_t Messenger::Subscribe(const qot_msg_type_t type, const std::string &name)
{
	if (type >= 0 && type < QOT_MSG_INVALID)
		add_to_subscribed_msg_type_list(type);
	// if (!name.empty())
	// 	add_to_subscribed_msg_name_list(name);

	return QOT_RETURN_TYPE_OK;
}

// Unsubscribe from Messages
qot_return_t Messenger::Unsubscribe(const qot_msg_type_t type, const std::string &name)
{
	if (type >= 0 && type < QOT_MSG_INVALID)
		remove_from_subscribed_msg_type_list(type);
	// if (!name.empty())
	// 	remove_from_subscribed_msg_name_list(name);

	return QOT_RETURN_TYPE_OK;
}

qot_return_t Messenger::DefineCluster(const std::vector<std::string> Nodes)
{
	// Call the Cluster Manager Cluster Define Function
	cluster_manager.DefineCluster(Nodes);
	return QOT_RETURN_TYPE_OK;
}

qot_return_t Messenger::WaitForPeers()
{
	qot_return_t retval;
	retval = cluster_manager.WaitForReady();
	return retval;
}

