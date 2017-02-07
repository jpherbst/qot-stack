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

using namespace qot;

std::ostream& operator <<(std::ostream& os, const qot_msgs::NameService& ns)
{
	os << "(name = "        << ns.name() 
	   << ", userID = "       << ns.userID()
	   << ")";
	return os;
}


ClusterManager::ClusterManager(const std::string &name, const std::string &uuid)
	: sub_entity(), name(name), uuid(uuid), ClusterNodes(), AliveNodes(), readyFlag(false)
{
	// Initialize the Cluster Manager	
	/**
     * A ReadCondition is created and assigned a handler which is triggered
     * when a new user joins
     */
    dds::sub::status::DataState newDataState;
    newDataState << dds::sub::status::SampleState::not_read()
            << dds::sub::status::ViewState::new_view()
            << dds::sub::status::InstanceState::alive();
    qot::NewUserHandler newUserHandler(ClusterNodes, AliveNodes, newDataState);
    dds::sub::cond::ReadCondition newUser(sub_entity.nameServiceReader, newDataState, newUserHandler);

    /**
     * A StatusCondition is created and assigned a handler which is triggered
     * when a DataWriter changes it's liveliness
     */
    qot::UserLeftHandler userLeftHandler(ClusterNodes, AliveNodes, sub_entity.nameServiceReader);
    dds::core::cond::StatusCondition userLeft(sub_entity.MessageReader, userLeftHandler);
    dds::core::status::StatusMask statusMask;
    statusMask << dds::core::status::StatusMask::liveliness_changed();
    userLeft.enabled_statuses(statusMask);

    /** A WaitSet is created and the four conditions created above are attached to it */
    waitSet += newUser;
    waitSet += userLeft;

    thread = boost::thread(boost::bind(&ClusterManager::watch, this));

	BOOST_LOG_TRIVIAL(info) << "ClusterManager Initialized for node " << name;
}

ClusterManager::~ClusterManager() 
{
	// Can be extended later ...
	// Join the main thread
	thread.join();
}

// Subscribe to Messages
qot_return_t ClusterManager::DefineCluster(const std::vector<std::string> Nodes)
{
	// Copy Cluster Vector from User and sort it
	ClusterNodes.assign(Nodes.begin(), Nodes.end());
	std::sort(ClusterNodes.begin(), ClusterNodes.end());
	return QOT_RETURN_TYPE_OK;
}

qot_return_t ClusterManager::WaitForReady()
{
	std::unique_lock<std::mutex> lck(mtx);
  	while (!readyFlag) 
  		cv.wait(lck);
  	return QOT_RETURN_TYPE_OK;
}

void ClusterManager::watch()
{
	while(1)
	{
		waitSet.dispatch();
		if(ClusterNodes == AliveNodes)
		{
			std::unique_lock<std::mutex> lck(mtx);
			readyFlag = true;
			cv.notify_all();
		}
		else
		{
			readyFlag = false;
		}
	}
}


