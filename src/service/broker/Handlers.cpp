/*
 * @file Handlers.cpp
 * @brief Broker Management Callback Handlers
 * @author Sandeep D'souza
 * 
 * Copyright (c) Carnegie Mellon University, 2018. All rights reserved.
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

#include "Handlers.hpp"

using namespace qot_broker;

ReadTimelineHandler::ReadTimelineHandler(timeline_reader_callback_t callback_function, dds::sub::status::DataState& dataState)
: dataState(dataState) 
{
    callback = callback_function;
}

void ReadTimelineHandler::operator() (dds::sub::DataReader<qot_msgs::TimelineType>& dr)
{
    // Call the registered callback function
    callback(dr, dataState);
}

ReadTopicHandler::ReadTopicHandler(topic_reader_callback_t callback_function, dds::sub::status::DataState& dataState)
: dataState(dataState) 
{
    callback = callback_function;
}

void ReadTopicHandler::operator() (dds::sub::DataReader<DDS::TopicBuiltinTopicData>& dr)
{
    // Call the registered callback function
    callback(dr, dataState);
}

ReadSubscriberHandler::ReadSubscriberHandler(sub_reader_callback_t callback_function, dds::sub::status::DataState& dataState)
: dataState(dataState) 
{
    callback = callback_function;
}

void ReadSubscriberHandler::operator() (dds::sub::DataReader<DDS::SubscriptionBuiltinTopicData>& dr)
{
    // Call the registered callback function
    callback(dr, dataState);
}

ReadPublisherHandler::ReadPublisherHandler(pub_reader_callback_t callback_function, dds::sub::status::DataState& dataState)
: dataState(dataState) 
{
    callback = callback_function;
}

void ReadPublisherHandler::operator() (dds::sub::DataReader<DDS::PublicationBuiltinTopicData>& dr)
{
    // Call the registered callback function
    callback(dr, dataState);
}

DeletionHandler::DeletionHandler(deletion_callback_t callback_function, dds::sub::status::DataState& dataState)
: dataState(dataState) 
{
    callback = callback_function;
}

void DeletionHandler::operator() (dds::core::Entity& e)
{
    // Call the registered callback function
    callback(e, dataState);
}


