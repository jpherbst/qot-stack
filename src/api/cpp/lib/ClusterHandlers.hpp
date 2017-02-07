/*
 * @file ClusterHandler.cpp
 * @brief Cluster Management Callback Handlers Header
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

#ifndef CLUSTER_HANDLERS_HPP
#define CLUSTER_HANDLERS_HPP

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

// Include the QoT Data Types
extern "C"
{
    #include "../../../qot_types.h"
}

/**
 * The logic utilises WaitSets and two DataReaders in order to detect
 * when a user has joined or left the message board. It will display the username
 * of a user who joins and will use a Query to display the number of messages sent
 * by a user who leaves.
 *
 */


namespace qot  {

    /**
     * The NewUserHandler will be called when the ReadCondition triggers.
     * It will output a message showing the user name of the user that has joined.
     */
    class NewUserHandler
    {
    public:
        /**
         * @param dataState The dataState on which to filter the messages
         */
        NewUserHandler(std::vector<std::string>& ClusterNodes, std::vector<std::string>& AliveNodes, dds::sub::status::DataState& dataState);
        void operator() (dds::sub::DataReader<qot_msgs::NameService>& dr);

    private:
        dds::sub::status::DataState& dataState;
        std::vector<std::string>& ClusterNodes;
        std::vector<std::string>& AliveNodes;
    };

    /**
     * The UserLeftHandler will be called when the StatusCondition triggers.
     * It will output a message showing users that have left and the number of messages
     * they have sent.
     */
    class UserLeftHandler
    {
    public:
        /**
         * @param nameServiceReader the dds::sub::DataReader<qot_msgs::NameService>
         */
        UserLeftHandler(std::vector<std::string>& ClusterNodes, std::vector<std::string>& AliveNodes, dds::sub::DataReader<qot_msgs::NameService>& nameServiceReader);

        void operator() (dds::core::Entity& e);

    private:
        dds::sub::DataReader<qot_msgs::NameService>& nameServiceReader;
        int prevAliveCount;
        std::vector<std::string>& ClusterNodes;
        std::vector<std::string>& AliveNodes;
    };

    /* Escape Handler handles termination of the waitset */
    class EscapeHandler
    {
    public:
        EscapeHandler(bool& terminated) : terminated(terminated) {}

        void operator() (void)
        {
            std::cout << "Cluster Manager has terminated." << std::endl;
            terminated = true;
        }

    private:
        bool& terminated;
    };

}

#endif


