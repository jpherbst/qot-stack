/*
 * @file ClusterManager.cpp
 * @brief Library Implementation for Cluster Management
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
#include "ClusterManager.hpp"

// Get the Cluster Handlers
#include "ClusterHandlers.hpp"

#define DEBUG 0

using namespace qot;

std::ostream& operator <<(std::ostream& os, const qot_msgs::NameService& ns)
{
	os << "(name = "        << ns.name() 
	   << ", userID = "       << ns.userID()
	   << ")";
	return os;
}


ClusterManager::ClusterManager(const std::string &name, const std::string &uuid)
	: sub_entity(name, uuid), name(name), uuid(uuid), ClusterNodes(), AliveNodes()
{
	// Initialize the Cluster Manager	
	/* Set the callback to NULL*/
	node_callback = NULL;
	
	/**
     * A ReadCondition is created and assigned a handler which is triggered
     * when a new user joins
     */
    dds::sub::status::DataState newDataState;
    newDataState << dds::sub::status::SampleState::not_read()
            << dds::sub::status::ViewState::new_view()
            << dds::sub::status::InstanceState::alive();
    qot::NewUserHandler newUserHandler(ClusterNodes, AliveNodes, node_callback, newDataState);
    dds::sub::cond::ReadCondition newUser(sub_entity.nameServiceReader, newDataState, newUserHandler);

    /**
     * A StatusCondition is created and assigned a handler which is triggered
     * when a DataWriter changes it's liveliness
     */
    qot::UserLeftHandler userLeftHandler(ClusterNodes, AliveNodes, node_callback, sub_entity.nameServiceReader);
    dds::core::cond::StatusCondition userLeft(sub_entity.MessageReader, userLeftHandler);
    dds::core::status::StatusMask statusMask;
    statusMask << dds::core::status::StatusMask::liveliness_changed();
    userLeft.enabled_statuses(statusMask);

    /**
     * A GuardCondition is assigned a handler which will be used to close
     * the message board
     */
    EscapeHandler escapeHandler(terminated);
    escape.handler(escapeHandler);


    /** A WaitSet is created and the four conditions created above are attached to it */
    waitSet += newUser;
    waitSet += userLeft;
    waitSet += escape;

    /* Start Polling Thread to wait for all users to join */
    readyFlag = false; // Set ready flag to false -> indicates all nodes have joined
    terminated = false; // Set terminated flag to false
    thread = boost::thread(boost::bind(&ClusterManager::watch, this));
    
    if(DEBUG)
		BOOST_LOG_TRIVIAL(info) << "ClusterManager Initialized for node " << name;
}

ClusterManager::~ClusterManager() 
{
	// Can be extended later ...
	// Join the main thread
	escape.trigger_value(true);
	thread.join();
}

// Define the initial cluster of nodes participating in the coordination
qot_return_t ClusterManager::DefineCluster(const std::vector<std::string> Nodes, qot_node_callback_t callback)
{
	// Copy Cluster Vector from User and sort it
	ClusterNodes.assign(Nodes.begin(), Nodes.end());
	std::sort(ClusterNodes.begin(), ClusterNodes.end());
	node_callback = callback;
	return QOT_RETURN_TYPE_OK;
}

qot_return_t ClusterManager::WaitForReady()
{
	std::unique_lock<std::mutex> lck(mtx);
	if(DEBUG)
		std::cout << "[ClusterManager::WaitForReady] Waiting for nodes to join: Ready Flag is " << readyFlag << "\n";
  	while (!readyFlag) 
  		cv.wait(lck);
  	if(DEBUG)
  		std::cout << "[ClusterManager::WaitForReady] All Nodes Joined: Ready Flag is " << readyFlag << "\n";
  	return QOT_RETURN_TYPE_OK;
}

void ClusterManager::watch()
{
	while(!terminated)
	{
		waitSet.dispatch(); 
		if(std::includes(AliveNodes.begin(), AliveNodes.end(), ClusterNodes.begin(), ClusterNodes.end()) && !ClusterNodes.empty() && !AliveNodes.empty())
		{
			std::unique_lock<std::mutex> lck(mtx);
			if(DEBUG)
				std::cout << "Ready flag has become true" << std::endl;
			readyFlag = true;
			cv.notify_all();
		}
		else
		{
			readyFlag = false;
		}
	}
}


