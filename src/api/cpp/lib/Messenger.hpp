/**
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
#ifndef MESSENGER_HPP
#define MESSENGER_HPP

// Boost includes
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/log/trivial.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

// Timeline Message DDS IDL
#include <msg/QoT_DCPS.hpp>

// DDS Publish Subscribe Entities
#include "MsgingEntities.cpp"

// Cluster Manager
#include "ClusterManager.hpp"

// Include the QoT api
extern "C"
{
	#include "../../../qot_types.h"
}

std::ostream& operator <<(std::ostream& os, const qot_msgs::TimelineMsgingType& tms);

namespace qot
{
	// Distributed Inter-Process Messenger functionality
	class Messenger : public dds::sub::NoOpDataReaderListener<qot_msgs::TimelineMsgingType>
	{
		// Constructor and destructor
		// The Constructor initializes private member variables and starts the DDS listener
		// The Destructor stops the DDS listener
		public: Messenger(const std::string &name, const std::string &uuid);
		public: ~Messenger();

		// Required by dds::sub::NoOpDataReaderListener dds callback. Gets called when new data is available to read. 
		public: virtual void on_data_available(dds::sub::DataReader<qot_msgs::TimelineMsgingType>& dr);

		// another dds callback. not sure what it does
		public: virtual void on_liveliness_changed(dds::sub::DataReader<qot_msgs::TimelineMsgingType>& dr, 
			const dds::core::status::LivelinessChangedStatus& status);
	
		// Publish a Message
		public: qot_return_t Publish(const qot_message_t msg);

		// Subscribe to Messages
		public: qot_return_t Subscribe(const qot_msg_type_t type, const std::string &name);

		// Unsubscribe from Messages
		public: qot_return_t Unsubscribe(const qot_msg_type_t type, const std::string &name);

		// Define the cluster -> Wrapper around the cluster manager function
		public: qot_return_t DefineCluster(const std::vector<std::string> Nodes);

		// Wait for all cluster peers to join -> Wrapper around the cluster manager function
		public: qot_return_t WaitForPeers();

		// Private member Functions
		// Add and remove types of messages to subscribe to
		private: void add_to_subscribed_msg_type_list(qot_msg_type_t type);
		private: void remove_from_subscribed_msg_type_list(qot_msg_type_t type);


		// Messenger information about the application and the timeline
		private: std::string uuid;    // timeline uuid
		private: std::string name;    // application name

		// Join the DDS domain to exchange information about timelines
		// private: dds::domain::DomainParticipant dp;
		// private: dds::topic::Topic<qot_msgs::TimelineMsgingType> topic;
		// private: dds::pub::Publisher pub;
		// private: dds::pub::DataWriter<qot_msgs::TimelineMsgingType> dw;
		// private: dds::sub::Subscriber sub;
		// private: dds::sub::DataReader<qot_msgs::TimelineMsgingType> dr;
		private: qot_msgs::TimelineMsgingType timeline_msg;
		private: qot_msgs::NameService name_msg;
		// private: std::string filter_expression;
		// private: std::vector<qot_msgs::MsgType> filter_params; 
		private: qot::PubEntities pub_entity;
		private: qot::SubEntities sub_entity;

		// Cluster Management Class
		private: qot::ClusterManager cluster_manager; 

	};
}

#endif
