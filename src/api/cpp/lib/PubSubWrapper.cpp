/**
 * @file PubSub.hpp
 * @brief API Wrapper to manage Topic-based Publish Subscribe for Distributed QoT based Coordinated Applications
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

// Include the QoT data types
extern "C"
{
	#include "../../../qot_types.h"
}

#include <iostream>

#include "PubSub.hpp"
#include "PubSubWrapper.hpp"

using namespace qot;

// Publisher Constructor
Publisher::Publisher(const std::string &topicName, const qot::TopicType topicType, const std::string &nodeName, const std::string &timelineUUID)
{
	// Check if timeline is global before allowing global topics
	if (topicType == TOPIC_GLOBAL || topicType == TOPIC_GLOBAL_OPT)
	{
		std::size_t pos = timelineUUID.find(GLOBAL_TL_STRING);
		if(pos != 0)
		{
			std::cout << "Only Global Timelines can publish to global topics\n";
			throw 20; // Change this to a specific number
		}
	} 
	Impl = new PublisherImpl(topicName, topicType, nodeName, timelineUUID);
}

// Publisher Destructor
Publisher::~Publisher()
{
	delete Impl;
}
	  	
// Publish to a topic -> Needs to be called for each data point published
qot_return_t Publisher::Publish(const qot_message_t msg)
{
	Impl->Publish(msg);
	return QOT_RETURN_TYPE_OK;
}

Subscriber::Subscriber(const std::string &topicName, const qot::TopicType topicType, const std::string &timelineUUID, qot_msg_callback_t callback)
{
	// Check if timeline is global before allowing global topics
	if (topicType == TOPIC_GLOBAL || topicType == TOPIC_GLOBAL_OPT)
	{
		std::size_t pos = timelineUUID.find(GLOBAL_TL_STRING);
		if(pos != 0)
		{
			std::cout << "Only Global Timelines can subscribe to global topics\n";
			throw 20; // Change this to a specific number
		}
	} 
	Impl = new SubscriberImpl(topicName, topicType, timelineUUID, callback);
}

Subscriber::~Subscriber()
{
	delete Impl;
}

