/*
 * @file Handlers.hpp
 * @brief Broker Management Callback Handlers Header
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

#ifndef HANDLERS_HPP
#define HANDLERS_HPP

// Timeline Message
#include <msg/QoT_DCPS.hpp>

// Define Callback functions
typedef void (*timeline_reader_callback_t)(dds::sub::DataReader<qot_msgs::TimelineType>& dr, dds::sub::status::DataState& dataState);
typedef void (*topic_reader_callback_t)(dds::sub::DataReader<DDS::TopicBuiltinTopicData>& dr, dds::sub::status::DataState& dataState);
typedef void (*pub_reader_callback_t)(dds::sub::DataReader<DDS::PublicationBuiltinTopicData>& dr, dds::sub::status::DataState& dataState);
typedef void (*sub_reader_callback_t)(dds::sub::DataReader<DDS::SubscriptionBuiltinTopicData>& dr, dds::sub::status::DataState& dataState);
typedef void (*deletion_callback_t)(dds::core::Entity& e, dds::sub::status::DataState& dataState);

namespace qot_broker  {

    /**
     * The ReadTimelineHandler will be called when the ReadCondition triggers.
     * The handler is initialized based on the required data state
     * A callback function is used to handle the case specific processing
     */
    class ReadTimelineHandler
    {
    public:
        /**
         * @param callback The callback function which handles data processing 
         * @param dataState The dataState on which to filter the messages
         */
        ReadTimelineHandler(timeline_reader_callback_t callback_function, dds::sub::status::DataState& dataState);
        void operator() (dds::sub::DataReader<qot_msgs::TimelineType>& dr);

    private:
        dds::sub::status::DataState dataState;
        timeline_reader_callback_t callback;
    };

    /**
     * The ReadTopicHandler will be called when the ReadCondition triggers.
     * The handler is initialized based on the required data state
     * A callback function is used to handle the case specific processing
     */
    class ReadTopicHandler
    {
    public:
        /**
         * @param callback The callback function which handles data processing 
         * @param dataState The dataState on which to filter the messages
         */
        ReadTopicHandler(topic_reader_callback_t callback_function, dds::sub::status::DataState& dataState);
        void operator() (dds::sub::DataReader<DDS::TopicBuiltinTopicData>& dr);

    private:
        dds::sub::status::DataState dataState;
        topic_reader_callback_t callback;
    };

    /**
     * The ReadSubscriberHandler will be called when the ReadCondition triggers.
     * The handler is initialized based on the required data state
     * A callback function is used to handle the case specific processing
     */
    class ReadSubscriberHandler
    {
    public:
        /**
         * @param callback The callback function which handles data processing 
         * @param dataState The dataState on which to filter the messages
         */
        ReadSubscriberHandler(sub_reader_callback_t callback_function, dds::sub::status::DataState& dataState);
        void operator() (dds::sub::DataReader<DDS::SubscriptionBuiltinTopicData>& dr);

    private:
        dds::sub::status::DataState dataState;
        sub_reader_callback_t callback;
    };

    /**
     * The ReadPublisherHandler will be called when the ReadCondition triggers.
     * The handler is initialized based on the required data state
     * A callback function is used to handle the case specific processing
     */
    class ReadPublisherHandler
    {
    public:
        /**
         * @param callback The callback function which handles data processing 
         * @param dataState The dataState on which to filter the messages
         */
        ReadPublisherHandler(pub_reader_callback_t callback_function, dds::sub::status::DataState& dataState);
        void operator() (dds::sub::DataReader<DDS::PublicationBuiltinTopicData>& dr);

    private:
        dds::sub::status::DataState dataState;
        pub_reader_callback_t callback;
    };

    /**
     * The DeletionHandler will be called when the StatusCondition triggers.
     * It will output a message showing users that have left and the number of messages
     * they have sent.
     */
    class DeletionHandler
    {
    public:
        /**
         * @param callback The callback function which handles data processing 
         * @param dataState The dataState on which to filter the messages
         */
        DeletionHandler(deletion_callback_t callback_function, dds::sub::status::DataState& dataState);

        void operator() (dds::core::Entity& e);

    private:
        dds::sub::status::DataState dataState;
        deletion_callback_t callback;

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


