/**
 * @file Broker.hpp
 * @brief DDS Broker header for interfacing with WAN-scale Pub-Sub
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
#ifndef BROKER_HPP
#define BROKER_HPP

// std includes
#include <thread>
#include <map>
#include <mutex>      

// Boost includes
#include <boost/thread.hpp>
#include <boost/log/trivial.hpp>

// Timeline Message DDS IDL
#include "dds/dds.hpp"
#include <msg/QoT_DCPS.hpp>

namespace qot_broker {

/*  Class to maintain node information 
 *  A 'node' is a 'spliced' instance.
 *  In shared memory mode, this will be a splice daemon process managing the domain.
 *  In single process mode each application will be a 'node'.
 */
class NodeInfo
{
	public:
	    NodeInfo(const DDS::ParticipantBuiltinTopicData& participantData);

	    bool operator==(const NodeInfo& rhs)
	    {
	        return nodeID_ == rhs.nodeID_;
	    }

	    int32_t nodeID_;       // Node Unique ID
	    std::string hostName_; // Node Name
	    /**
	    * Vector to hold the IDs of the currently alive participants in the node
	    */
	    std::vector<uint32_t> participants;
};

/*  Class to maintain timeline information */
class TimelineInfo
{
	public:
	    TimelineInfo(const std::string& timelineUUID);
	    std::string getTimelineUUID();
	    bool operator==(const TimelineInfo& rhs)
	    {
	        return timeline_uuid == rhs.timeline_uuid;
	    }

	private:
		std::string timeline_uuid; // Timeline Name
};

/* Class to maintain information of topics published by other nodes on the LAN*/
class TopicInfo
{
	public:
	    TopicInfo(const DDS::TopicBuiltinTopicData& topicData);
	    std::string getTopicName();
	    int32_t getTopicID();
	    std::string getTimelineUUID();
	    int addSubscriber(int32_t participant_ID, int32_t dr_ID, std::string timelineUUID);
	    int addPublisher(int32_t participant_ID, int32_t dw_ID, std::string timelineUUID);
	    int removeSubscriber(int32_t participant_ID, int32_t dr_ID);
	    int removePublisher(int32_t participant_ID, int32_t dw_ID);
	    void deleteTopicPubSubNodes()
	    bool operator==(const TopicInfo& rhs)
	    {
	        return topicID_ == rhs.topicID_;
	    }
	private:
	    void setTimelineUUID(std::string& uuid);
	    int32_t topicID_;						 // Topic Unique ID
	    std::string topicName_; 			     // Topic Name
	    std::string timeline_uuid;				 // Timeline Name under which the topic falls
	    std::map<int32_t, int32_t> subscribers;	 // List of Subscribers of the topic
	    std::map<int32_t, int32_t> publishers;   // List of Publishers of the topic
	    std::mutex topic_mtx;           	     // mutex for critical section

};

/* Class to Start the Broker Threads which listen for new topics, publishers and subscribers */
class Broker
{
	public:
		Broker();  // Constructor
		~Broker(); // Destructor

		// Thread handlers
		void ParticipantThread();
		void TimelineThread();
		void TopicThread();
		void PublisherThread();
		void SubscriberThread();
	private: 
		// Worker threads
		boost::thread participant_thread; // Worker thread for finding new participants
		boost::thread timeline_thread;    // Worker thread for finding new timelines
		boost::thread topic_thread; 	  // Worker thread for finding new topics
		boost::thread publisher_thread;   // Worker thread for finding new publishers
		boost::thread subscriber_thread;  // Worker thread for finding new subscribers
		bool kill; 						  // Kill flag for threads
		
		// DDS Information
		dds::domain::DomainParticipant dp; 		// Domain Participant
        dds::sub::Subscriber builtinSubscriber;	// BuiltInTopics Subscribers
};

}

#endif


