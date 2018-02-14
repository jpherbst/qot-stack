/**
 * @file PubSub.hpp
 * @brief API Header to manage Topic-based Publish Subscribe for Distributed QoT based Coordinated Applications
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

// Include the QoT api
extern "C"
{
	#include "../../../qot_types.h"
}

#include <string>

namespace qot
{
	enum TopicType { 
		TOPIC_LOCAL = (0), 	// Will be available within the LAN
		TOPIC_GLOBAL, 		// Will be available across the WAN
		TOPIC_GLOBAL_OPT, 	// Used to share topics only between 2 LANs
		TOPIC_NUM_TYPES		// Number of types
	};

	// Publisher Implementation Class
	class PublisherImpl; 

	// Publisher Wrapper Class
	class Publisher 
	{
		// Constructor and destructor
		public: Publisher(const std::string &topicName, const qot::TopicType topicType, const std::string &nodeName, const std::string &timelineUUID);
		public: ~Publisher();
	  	
	  	// Publish to a topic -> Needs to be called for each data point published
		public: qot_return_t Publish(const qot_message_t msg);
		
		// Publisher Implementation class
		private: PublisherImpl *Impl;
	};

	// Subscriber Implementation Class
	class SubscriberImpl; 

	// Subscriber Wrapper Class
	class Subscriber
	{
		// Constructor and destructor
		public: Subscriber(const std::string &topicName, const qot::TopicType topicType, const std::string &timelineUUID, qot_msg_callback_t callback);
		public: ~Subscriber();
	  	
		// Subscriber Implementation class
		private: SubscriberImpl *Impl;
	};
}

#endif


