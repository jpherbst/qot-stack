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
#include <set>

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

// Initialize Hashmap to convert from qot_msg_type_t to the DDS idl 
// ->> Should be changed when a new message is added
void Messenger::initialize_hashmap()
{
	type_hashmap[QOT_MSG_COORD_READY] = "COORD_READY"; 
	type_hashmap[QOT_MSG_COORD_START] = "COORD_START";
	type_hashmap[QOT_MSG_COORD_STOP]  = "COORD_STOP";
	type_hashmap[QOT_MSG_SENSOR_VAL]  = "SENSOR_VAL";
	type_hashmap[QOT_MSG_INVALID]     = "INVALID_MSG";
}

// Need to populate these functions
void Messenger::add_to_subscribed_msg_type_list(qot_msg_type_t type)
{
	unsigned long long temp_mask = 1;
	temp_mask = temp_mask << type;
	sub_type_mask = sub_type_mask | temp_mask;
	return;
}

// Need to populate these functions
void Messenger::remove_from_subscribed_msg_type_list(qot_msg_type_t type)
{
	unsigned long long temp_mask = 1;
	temp_mask = temp_mask << type;
	temp_mask = ~temp_mask;
	sub_type_mask = sub_type_mask & temp_mask;
	return;
}

// Need to populate these functions
void Messenger::add_to_subscribed_msg_name_list(const std::string &name)
{
	// Add the unique name of the node to the unordered set
	sub_nodes.insert(name);
	return;
}

// Need to populate these functions
void Messenger::remove_from_subscribed_msg_name_list(const std::string &name)
{
	// Remove the node's name from the unordered set of subscribed nodes
	sub_nodes.erase(name);
	return;
}

void Messenger::on_data_available(dds::sub::DataReader<qot_msgs::TimelineMsgingType>& dr) 
{
	// QoT Message type
	qot_message_t qot_msg;
	// get only new/unread data
	dds::sub::status::DataState aliveDataState;
            aliveDataState << dds::sub::status::SampleState::any()
                << dds::sub::status::ViewState::any()
                << dds::sub::status::InstanceState::alive();

    // Setup query based on application subscription requests
    query_lock.lock();
    dds::sub::Query query(dr, sub_expression, sub_params);
    query_lock.unlock();
    /**
     * Take messages. Using take instead of read removes the messages from
     * the system, preventing resources from being saturated due to a build
     * up of messages
     */
    dds::sub::LoanedSamples<qot_msgs::TimelineMsgingType> messages
        = dr.select().content(query).state(aliveDataState).take();

    /** Output the username and content for each message */
    for (dds::sub::LoanedSamples<qot_msgs::TimelineMsgingType>::const_iterator message
            = messages.begin(); message < messages.end(); ++message)
    {
        if(message->info().valid())
        {
            strcpy(qot_msg.name, (message->data().name()).c_str());
            qot_msg.type = (qot_msg_type_t) message->data().type();
            strcpy(qot_msg.data, (message->data().data()).c_str());
            TP_FROM_nSEC(qot_msg.timestamp.estimate, message->data().utimestamp().timestamp());
            TL_FROM_nSEC(qot_msg.timestamp.interval.above, message->data().utimestamp().uncertainty_u());
            TL_FROM_nSEC(qot_msg.timestamp.interval.below, message->data().utimestamp().uncertainty_l());
            if(msg_callback != NULL)
			{
				msg_callback(&qot_msg);
			}
        }
    }
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
	: pub_entity(name, uuid), sub_entity(name, uuid), name(name), uuid(uuid), cluster_manager(name, uuid), sub_type_mask(0xffffffffffffffff)
{
	std::ostringstream convert;

	// Initialize the hashmap for the queries
	initialize_hashmap();

	// Query expression initialization (accept all packets except the invalid ones)
	sub_expression = "(type < %0)";
	convert << "INVALID_MSG";
	sub_params.clear();
	sub_params.push_back(convert.str());

	// Initialize Subscription Function pointer to NULL
	msg_callback = NULL;

	// Create the message listener
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
	sub_entity.MessageReader.listener(nullptr, dds::core::status::StatusMask::none());
	std::cout << "Messenger has terminated" << std::endl;
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

// Subscribe to Messages -> The list is reset each time
qot_return_t Messenger::Subscribe(const std::set<qot_msg_type_t> &MsgTypes, qot_msg_callback_t callback) 
{
	int param_count = 1;
	std::ostringstream sub_exp_temp;

	query_lock.lock();

	if(callback != NULL)
	{
		msg_callback = callback;
	}
	else
	{
		query_lock.unlock();
		return QOT_RETURN_TYPE_ERR;
	}

	// Clear the parameter vector and the expression string
	sub_expression = "(type < %0)";
	sub_params.clear();
	sub_params.push_back(type_hashmap[QOT_MSG_INVALID]);


	// Populate the Subscription Message Type List
	for(std::set<qot_msg_type_t>::const_iterator it = MsgTypes.begin() ; it != MsgTypes.end(); ++it)
	{
		if (*it >= 0 && *it < QOT_MSG_INVALID)
		{
			sub_params.push_back(type_hashmap[*it]);
			if (it != MsgTypes.begin())
			{
				sub_exp_temp << " OR ";
			}
			else
			{
				sub_exp_temp << " AND (";
			}
			sub_exp_temp << "(type = %" << param_count << ")";
			param_count++;
		}
	}
	sub_exp_temp << ")";
	sub_expression = sub_expression + sub_exp_temp.str();
	query_lock.unlock();
	std::cout << sub_expression << std::endl;
	return QOT_RETURN_TYPE_OK;
}

qot_return_t Messenger::DefineCluster(const std::vector<std::string> Nodes, qot_node_callback_t callback)
{
	// Call the Cluster Manager Cluster Define Function
	cluster_manager.DefineCluster(Nodes, callback);
	return QOT_RETURN_TYPE_OK;
}

qot_return_t Messenger::WaitForPeers()
{
	qot_return_t retval;
	retval = cluster_manager.WaitForReady();
	return retval;
}

