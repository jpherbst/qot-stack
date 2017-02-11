/*
 * @file ClusterManager.cpp
 * @brief Messaging Entity classes for the Opensplice DDS based messaging framework
 * @author Sandeep D'souza
 * 
 * Copyright (c) Carnegie Mellon University, 2016. All rights reserved.
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

#include <msg/QoT_DCPS.hpp>

#ifndef MSGING_ENTITIES_HPP
#define MSGING_ENTITIES_HPP

namespace qot {

#define TERMINATION_MESSAGE -1

/**
 * This class serves as a container holding initialised entities used for publishing.
 */
class PubEntities
{
public:
    /**
     * This constructor initialises the entities used for publishing.
     */
    PubEntities(const std::string &node_name, const std::string &timeline_uuid):
        MessageWriter(dds::core::null), nameServiceWriter(dds::core::null)
    {
        std::ostringstream timeline_partition;
        timeline_partition << "Messaging_" << "tl" << timeline_uuid;
        /** A dds::domain::DomainParticipant is created for the default domain. */
        dds::domain::DomainParticipant participant
            = dds::domain::DomainParticipant(org::opensplice::domain::default_id());

        /** A dds::pub::Publisher is created on the domain participant. */
        dds::pub::qos::PublisherQos pubQos
            = participant.default_publisher_qos()
                << dds::core::policy::Partition(timeline_partition.str());
        dds::pub::Publisher publisher(participant, pubQos);

        /**
         * A dds::topic::qos::TopicQos is created with Reliability set to Reliable to
         * guarantee delivery.
         */
        dds::topic::qos::TopicQos reliableTopicQos
            = participant.default_topic_qos() << dds::core::policy::Reliability::Reliable();

        /** Set the Reliable TopicQos as the new default */
        participant.default_topic_qos(reliableTopicQos);

        /**
         * A dds::topic::Topic is created for the qot_msgs::TimelineMsgingType sample type on the
         * domain participant.
         */
        dds::topic::Topic<qot_msgs::TimelineMsgingType> MessageTopic
            = dds::topic::Topic<qot_msgs::TimelineMsgingType>(participant, "QoT_Messaging", reliableTopicQos);

        /** A dds::pub::DataWriter is created for the qot_msgs::TimelineMsgingType Topic with the TopicQos */
        MessageWriter
            = dds::pub::DataWriter<qot_msgs::TimelineMsgingType>(publisher, MessageTopic, MessageTopic.qos());

        /**
         * A dds::topic::qos::TopicQos is created with Durability set to Transient to
         * ensure that if a subscriber joins after the sample is written then DDS
         * will still retain the sample for it.
         */
        dds::topic::qos::TopicQos transientTopicQos
            = participant.default_topic_qos() << dds::core::policy::Durability::Transient();

        /**
         * A dds::topic::Topic is created for the qot_msgs::NameService sample type on the
         * domain participant.
         */
        dds::topic::Topic<qot_msgs::NameService> nameServiceTopic
            = dds::topic::Topic<qot_msgs::NameService>(participant, "QoT_NameService", transientTopicQos);

        /** A dds::pub::DataWriter is created for the qot_msgs::NameService topic with a modififed Qos. */
        dds::pub::qos::DataWriterQos dwQos = nameServiceTopic.qos();
        dwQos << dds::core::policy::WriterDataLifecycle::ManuallyDisposeUnregisteredInstances();
        nameServiceWriter = dds::pub::DataWriter<qot_msgs::NameService>(publisher, nameServiceTopic, dwQos);
    }

public:
    dds::pub::DataWriter<qot_msgs::TimelineMsgingType> MessageWriter;
    dds::pub::DataWriter<qot_msgs::NameService> nameServiceWriter;
};

/**
 * This class serves as a container holding initialised entities used for subscribing.
 */
class SubEntities
{
public:
    /**
     * This constructor initialises the entities for subscribing.
     */
    SubEntities(const std::string &node_name, const std::string &timeline_uuid) :
        MessageReader(dds::core::null), nameServiceReader(dds::core::null)
    {
        std::ostringstream timeline_partition;
        timeline_partition << "Messaging_" << "tl" << timeline_uuid;
        /** A dds::domain::DomainParticipant is created for the default domain. */
        dds::domain::DomainParticipant participant
            = dds::domain::DomainParticipant(org::opensplice::domain::default_id());

        /** A dds::sub::Subscriber is created on the domain participant. */
        dds::sub::qos::SubscriberQos subQos
            = participant.default_subscriber_qos()
                << dds::core::policy::Partition(timeline_partition.str());
        dds::sub::Subscriber subscriber(participant, subQos);

        /**
         * A dds::topic::qos::TopicQos is created with Reliability set to Reliable to
         * guarantee delivery.
         */
        dds::topic::qos::TopicQos reliableTopicQos
            = participant.default_topic_qos() << dds::core::policy::Reliability::Reliable();

        /** Set the Reliable TopicQos as the new default */
        participant.default_topic_qos(reliableTopicQos);

        /**
         * A dds::topic::Topic is created for the qot_msgs::TimelineMsgingType sample type on the
         * domain participant.
         */
        dds::topic::Topic<qot_msgs::TimelineMsgingType> MessageTopic
            = dds::topic::Topic<qot_msgs::TimelineMsgingType>(participant, "QoT_Messaging", reliableTopicQos);

        // Define the filter expression -> Such that messages sent by the node are rejected
        std::string expression = "(name <> %0)";

        // Define the filter parameters
        std::vector<std::string> params = {node_name};

        // Create the filter for the content-filtered-topic
        dds::topic::Filter filter(expression, params);

        // Create the ContentFilteredTopic
        dds::topic::ContentFilteredTopic<qot_msgs::TimelineMsgingType> cfMessageTopic
         = dds::topic::ContentFilteredTopic<qot_msgs::TimelineMsgingType>(MessageTopic,"CFQoT_Messaging",filter);

        /**
         * A dds::sub::qos::DataReaderQos is created with the history set to KeepAll so
         * that all messages are kept
         */
        dds::sub::qos::DataReaderQos drQos = MessageTopic.qos();
        drQos << dds::core::policy::History::KeepAll();

        /** A dds::sub::DataReader is created for the qot_msgs::TimelineMsgingType topic with a modified Qos. */
        MessageReader = dds::sub::DataReader<qot_msgs::TimelineMsgingType>(subscriber, cfMessageTopic, drQos);

        /** A dds::topic::qos::TopicQos is created with Durability set to Transient */
        dds::topic::qos::TopicQos transientTopicQos
            = participant.default_topic_qos() << dds::core::policy::Durability::Transient();

        /**
         * A dds::topic::Topic is created for the qot_msgs::NameService sample type on the
         * domain participant.
         */
        dds::topic::Topic<qot_msgs::NameService> nameServiceTopic
            = dds::topic::Topic<qot_msgs::NameService>(participant, "QoT_NameService", transientTopicQos);

        /** A dds::sub::DataReader is created for the qot_msgs::NameService topic with the TopicQos. */
        nameServiceReader
            = dds::sub::DataReader<qot_msgs::NameService>(subscriber, nameServiceTopic, nameServiceTopic.qos());
    }

public:
    dds::sub::DataReader<qot_msgs::TimelineMsgingType> MessageReader;
    dds::sub::DataReader<qot_msgs::NameService> nameServiceReader;
};

}

#endif
