/**
 * @file PubSub.hpp
 * @brief Library Header to manage Topic-based Publish Subscribe for Distributed QoT based Coordinated Applications
 * @author Sandeep D'souza
 *
 * Copyright (c) Carnegie Mellon University, 2018. All rights reserved.
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

#ifndef PUBSUB_HPP
#define PUBSUB_HPP

// Timeline Message DDS IDL
#include <msg/QoT_DCPS.hpp>

// Include the QoT api
extern "C"
{
	#include "../../../qot_types.h"
}

std::ostream& operator <<(std::ostream& os, const qot_msgs::TimelineMsgingType& tms);

namespace qot
{
	// Distributed Inter-Process Messenger functionality
	class Publisher
	{
		// Constructor and destructor
		// The Constructor initializes private member variables and starts the DDS datawriter
		// The Destructor destroys the datawriter
		public: Publisher(const std::string &topicName, const std::string &nodeName, const std::string &timelineUUID);
		public: ~Publisher();
	
		// Publish to a topic -> Needs to be called for each data point published
		public: qot_return_t Publish(const qot_message_t msg);

		// Information about the application, topic and the timeline
		private: std::string timeline_uuid;    // timeline uuid
		private: std::string node_name;    	   // application name
		private: std::string topic_name;	   // topic name 

		// DDS Data Writer
		private: dds::pub::DataWriter<qot_msgs::TimelineMsgingType> dataWriter;		

		// DDS Message Data Type
		private: qot_msgs::TimelineMsgingType timeline_msg;


	};

	// Distributed Inter-Process Messenger functionality
	class Subscriber : public dds::sub::NoOpDataReaderListener<qot_msgs::TimelineMsgingType>
	{
		// Constructor and destructor
		// The Constructor initializes private member variables and starts the DDS datareader
		// The Destructor destroys the datareader
		public: Subscriber(const std::string &topicName, const std::string &timelineUUID, qot_msg_callback_t callback);
		public: ~Subscriber();

		// Required by dds::sub::NoOpDataReaderListener dds callback. Gets called when new data is available to read. 
		public: virtual void on_data_available(dds::sub::DataReader<qot_msgs::TimelineMsgingType>& dr);

		// another dds callback. not sure what it does
		public: virtual void on_liveliness_changed(dds::sub::DataReader<qot_msgs::TimelineMsgingType>& dr, 
			const dds::core::status::LivelinessChangedStatus& status);

		// UserFunction Callback for message
		private: qot_msg_callback_t msg_callback;

		// Information about the topic and the timeline
		private: std::string timeline_uuid;    // timeline uuid
		private: std::string topic_name;       // topic name

		// DDS Data Writer
		private: dds::sub::DataReader<qot_msgs::TimelineMsgingType> dataReader;
	};
}

#endif


