/*
 * @file ClusterHandler.cpp
 * @brief Cluster Management Callback Handlers
 * @author Sandeep D'souza
 * 
 * Copyright (c) Carnegie Mellon University, 2017. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, 
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


/**
 * The logic utilises WaitSets and two DataReaders in order to detect
 * when a user has joined or left the message board. It will display the username
 * of a user who joins and will use a Query to display the number of messages sent
 * by a user who leaves.
 *
 */

#include "ClusterHandlers.hpp"

using namespace qot;


NewUserHandler::NewUserHandler(std::vector<std::string>& ClusterNodes, std::vector<std::string>& AliveNodes, qot_node_callback_t callback, dds::sub::status::DataState& dataState)
: dataState(dataState), ClusterNodes(ClusterNodes), AliveNodes(AliveNodes) 
{
    node_callback = callback;
}

void NewUserHandler::operator() (dds::sub::DataReader<qot_msgs::NameService>& dr)
{
    qot_node_t node_info;
    dds::sub::status::DataState newDataState;
    newDataState << dds::sub::status::SampleState::not_read()
            << dds::sub::status::ViewState::new_view()
            << dds::sub::status::InstanceState::alive();
    /** Read the new users */
    dds::sub::LoanedSamples<qot_msgs::NameService> messages = dr.select().state(newDataState).read();

    /** Output each new user's user name and add it to the vector */
    for (dds::sub::LoanedSamples<qot_msgs::NameService>::const_iterator message = messages.begin();
        message < messages.end(); ++message)
    {
        if(message->info().valid())
        {
            std::cout << "New user: " << message->data().name() << std::endl;
            // Add to List of Active Nodes
            AliveNodes.insert(std::upper_bound(AliveNodes.begin(), AliveNodes.end(), message->data().name()), message->data().name());
            // Populate Callback information
            strcpy(node_info.name, (message->data().name()).c_str()); 
            node_info.userID = message->data().userID();
            node_info.status = QOT_NODE_JOINED;
            // Trigger a Callback to the application
            if(node_callback != NULL)
                node_callback(&node_info);
        }
    }
}

UserLeftHandler::UserLeftHandler(std::vector<std::string>& ClusterNodes, std::vector<std::string>& AliveNodes, qot_node_callback_t callback, dds::sub::DataReader<qot_msgs::NameService>& nameServiceReader)
: nameServiceReader(nameServiceReader), prevAliveCount(0), ClusterNodes(ClusterNodes), AliveNodes(AliveNodes) 
{
    node_callback = callback;
}

void UserLeftHandler::operator() (dds::core::Entity& e)
{
    qot_node_t node_info;
    dds::sub::DataReader<qot_msgs::TimelineMsgingType>& MessageReader
        = (dds::sub::DataReader<qot_msgs::TimelineMsgingType>&)e;

    /** Check that the liveliness has changed */
    const dds::core::status::LivelinessChangedStatus livChangedStatus
        = MessageReader.liveliness_changed_status();

    if(livChangedStatus.alive_count() < prevAliveCount)
    {
        /** Take inactive users so they will not appear in the user list */
        dds::sub::status::DataState notAliveDataState;
        notAliveDataState << dds::sub::status::SampleState::any()
                << dds::sub::status::ViewState::any()
                << dds::sub::status::InstanceState::not_alive_mask();
        dds::sub::LoanedSamples<qot_msgs::NameService> users
            = nameServiceReader.select().state(notAliveDataState).take();

         /** Output the number of messages sent by each user that has left */
        for (dds::sub::LoanedSamples<qot_msgs::NameService>::const_iterator user = users.begin();
            user < users.end(); ++user)
        {
            if(user->info().valid())
            {
                std::cout << "Departed user: " << user->data().name();
                // Remove User from Alive Nodes
                AliveNodes.erase(std::remove(AliveNodes.begin(), AliveNodes.end(), user->data().name()), AliveNodes.end());
                 // Populate Callback information
                strcpy(node_info.name, (user->data().name()).c_str()); 
                node_info.userID = user->data().userID();
                node_info.status = QOT_NODE_LEFT;
                // Trigger a Callback to the application
                if(node_callback != NULL)
                    node_callback(&node_info);
                //terminated = true;
            }
        }
    }
    prevAliveCount = livChangedStatus.alive_count();
}


