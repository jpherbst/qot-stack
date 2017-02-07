/**
 * @file ClusterManager.hpp
 * @brief Library header for Cluster Management
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
#ifndef CLUSTER_HPP
#define CLUSTER_HPP

// Boost includes
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/log/trivial.hpp>
#include <boost/thread.hpp> 
#include <boost/date_time/posix_time/posix_time.hpp>

// Timeline Message
#include <msg/QoT_DCPS.hpp>

// std librasy includes
#include <vector>
#include <string>
#include <algorithm>
#include <mutex>
#include <condition_variable>

// Include the QoT Data Types
extern "C"
{
	#include "../../../qot_types.h"
}

// Get the Subscriber Entities
#include "MsgingEntities.cpp"

namespace qot
{
	// Distributed Inter-Process Cluster Management
	class ClusterManager 
	{
		// Node Management Vectors 
		// -> ClusterNodes: Specified Nodes in Coordination
		// -> ActiveNodes : Nodes that have joined the DDS Domain
		private: std::vector<std::string> ClusterNodes; 
		private: std::vector<std::string> AliveNodes;
		
		// Constructor and destructor
		// The Constructor initializes private member variables and starts the DDS listener
		// The Destructor stops the DDS listener
		public: ClusterManager(const std::string &name, const std::string &uuid);
		public: ~ClusterManager();
	
		// Define the nodes which will be a part of the coordination
		public: qot_return_t DefineCluster(const std::vector<std::string> Nodes);

		// Wait for al the nodes to join the cluster
		public: qot_return_t WaitForReady();

		// Private Function -> Watch for changes to the cluster
		private: void watch();


		// Information about the application and the timeline
		private: std::string uuid;    // timeline uuid
		private: std::string name;    // application name

		// DDS Mesage Subscribing Entities
		private: qot::SubEntities sub_entity;

		// DDS NameService Message type
		private: qot_msgs::NameService name_msg;
		private: std::string filter_expression;

		// Handlers for Node Management
		//private: qot::NewUserHandler newUserHandler;
		//private: qot::UserLeftHandler userLeftHandler;

		// DDS Waitset
		private: dds::core::cond::WaitSet waitSet;

		// ClusterManagement Thread
		private: boost::thread thread;

		// Flag Signifying whether all  nodes have joined the DDS domain
		public: bool readyFlag;

		// Guard condition to terminate the watch thread 
    	private: dds::core::cond::GuardCondition escape;

    	// Flag to terminate the cluster manager
		private: bool terminated;

		// ClusterManagement Ready Synchronization 
		private: std::mutex mtx;
		private: std::condition_variable cv;

	};
}

#endif
