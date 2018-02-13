/**
 * @file PubSub.cpp
 * @brief Library implementation to manage Topic-based Publish Subscribe for Distributed QoT based Coordinated Applications
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

#include <iostream>

// Pub-Sub Header
#include "PubSub.hpp"

using namespace qot;

// Helper function to reshape topic name based on topic type
void PublisherImpl::reshapeTopicName()
{
    std::string prefix;
    // Create topic prefix based on type, default type is local
    switch(topic_type) {
        case TOPIC_LOCAL : 
                prefix = "lo_";
                break;
        case TOPIC_GLOBAL : 
                prefix = "gl_";
                break;
        case TOPIC_GLOBAL_OPT : 
                prefix = "go_";
                break;
        default:
                prefix = "lo_";
                break;
    }
    // Append Prefix to Topic Name
    topic_name = prefix + topic_name;
}

/**
 * Publisher Constructor
 */
PublisherImpl::PublisherImpl(const std::string &topicName, const qot::TopicType topicType, const std::string &nodeName, const std::string &timelineUUID)
 : timeline_uuid(timelineUUID), topic_type(topicType), topic_name(topicName), node_name(nodeName), dataWriter(dds::core::null)
{
    int result = 0;
    try
    {
        /** A dds::domain::DomainParticipant is created for the default domain. */
        dds::domain::DomainParticipant dp(org::opensplice::domain::default_id());

        /** The Durability::Transient policy is specified as a dds::topic::qos::TopicQos
         * so that even if the subscriber does not join until after the sample is written
         * then the DDS will still retain the sample for it. The Reliability::Reliable
         * policy is also specified to guarantee delivery. */
        dds::topic::qos::TopicQos topicQos
             = dp.default_topic_qos()
                << dds::core::policy::Durability::Transient()
                << dds::core::policy::Reliability::Reliable();

        /** Reshape Topic Name based on type **/
        reshapeTopicName();

        /** A dds::topic::Topic is created for our messaging type on the domain participant. */
        dds::topic::Topic<qot_msgs::TimelineMsgingType> topic(dp, topicName, topicQos);

        /** A dds::pub::Publisher is created on the domain participant. 
            Set the partition specific to the timeline **/
        dds::pub::qos::PublisherQos pubQos
            = dp.default_publisher_qos()
                << dds::core::policy::Partition(timeline_uuid);
        dds::pub::Publisher pub(dp, pubQos);

        /** The dds::pub::qos::DataWriterQos is derived from the topic qos and the
         * WriterDataLifecycle::ManuallyDisposeUnregisteredInstances policy is
         * specified as an addition. This is so the publisher can optionally be run (and
         * exit) before the subscriber. It prevents the middleware default 'clean up' of
         * the topic instance after the writer deletion, this deletion implicitly performs
         * DataWriter::unregister_instance */
        dds::pub::qos::DataWriterQos dwqos = topic.qos();
        dwqos << dds::core::policy::WriterDataLifecycle::ManuallyDisposeUnregisteredInstances();

        /** A dds::pub::DataWriter is created on the Publisher & Topic with the modififed Qos. */
        dataWriter = dds::pub::DataWriter<qot_msgs::TimelineMsgingType>(pub, topic, dwqos);
    }
    catch (const dds::core::Exception& e)
    {
        std::cerr << "ERROR: Exception: " << e.what() << std::endl;
    }
}

/**
 * Publisher Destructor
 */
PublisherImpl::~PublisherImpl()
{
    std::cout << "Publisher of topic " << topic_name << " has terminated" << std::endl;
}

/* Public function to publish a message to a topic */
qot_return_t PublisherImpl::Publish(const qot_message_t msg)
{
    qot_msgs::UncertainTimestamp utimestamp;

    // Unroll Message Timestamp (resolution is in ns -> convert to asec later)
    utimestamp.timestamp()     = msg.timestamp.estimate.sec*nSEC_PER_SEC + msg.timestamp.estimate.asec/ASEC_PER_NSEC;
    utimestamp.uncertainty_u() = msg.timestamp.interval.above.sec*nSEC_PER_SEC + msg.timestamp.interval.above.asec/ASEC_PER_NSEC;
    utimestamp.uncertainty_l() = msg.timestamp.interval.below.sec*nSEC_PER_SEC + msg.timestamp.interval.below.asec/ASEC_PER_NSEC;
    
    // Unroll message parameters
    timeline_msg.name() = (std::string) node_name;            // Our name
    timeline_msg.uuid() = (std::string) timeline_uuid;        // Timeline UUID
    timeline_msg.type() = (qot_msgs::MsgType) msg.type;       // Msg Type
    timeline_msg.data() = std::string(msg.data);              // Msg Data
    timeline_msg.utimestamp() = utimestamp;                   // Timestamp Associated with Msg
    
    // Publish the message
    dataWriter.write(timeline_msg);
    return QOT_RETURN_TYPE_OK;  
}

// Helper function to reshape topic name based on topic type
void SubscriberImpl::reshapeTopicName()
{
    std::string prefix;
    // Create topic prefix based on type, default type is local
    switch(topic_type) {
        case TOPIC_LOCAL : 
                prefix = "lo_";
                break;
        case TOPIC_GLOBAL : 
                prefix = "gl_";
                break;
        case TOPIC_GLOBAL_OPT : 
                prefix = "go_";
                break;
        default:
                prefix = "lo_";
                break;
    }

    // Append Prefix to Topic Name
    topic_name = prefix + topic_name;
}

/**
 * Subscriber Constructor
 */
SubscriberImpl::SubscriberImpl(const std::string &topicName, const qot::TopicType topicType, const std::string &timelineUUID, qot_msg_callback_t callback)
 : timeline_uuid(timelineUUID), topic_name(topicName), topic_type(topicType), dataReader(dds::core::null), msg_callback(NULL)
{
    // Set the subscriber callback function
    if(callback != NULL)
    {
        msg_callback = callback;
    }

    try
    {
        /** A domain participant and topic are created identically as in
         the ::publisher */
        dds::domain::DomainParticipant dp(org::opensplice::domain::default_id());
        dds::topic::qos::TopicQos topicQos = dp.default_topic_qos()
                                                    << dds::core::policy::Durability::Transient()
                                                    << dds::core::policy::Reliability::Reliable();

        /** Reshape Topic Name based on type **/
        reshapeTopicName();

        dds::topic::Topic<qot_msgs::TimelineMsgingType> topic(dp, topic_name, topicQos);

        /** A dds::sub::Subscriber is created on the domain participant. 
            Set the partition specific to the timeline **/
        dds::sub::qos::SubscriberQos subQos
            = dp.default_subscriber_qos()
                << dds::core::policy::Partition(timeline_uuid);
        dds::sub::Subscriber sub(dp, subQos);

        /** The dds::sub::qos::DataReaderQos are derived from the topic qos */
        dds::sub::qos::DataReaderQos drqos = topic.qos();

        /** A dds::sub::DataReader is created on the Subscriber & Topic with the DataReaderQos. */
        dataReader = dds::sub::DataReader<qot_msgs::TimelineMsgingType>(sub, topic, drqos);
    }
    catch (const dds::core::Exception& e)
    {
        std::cerr << "ERROR: Exception: " << e.what() << std::endl;
    }
}

/**
 * Subscriber Destructor
 */
SubscriberImpl::~SubscriberImpl()
{
    std::cout << "Subscriber of topic " << topic_name << " has terminated" << std::endl;
}

/* DDS Callback to receive subscribed messages */
void SubscriberImpl::on_data_available(dds::sub::DataReader<qot_msgs::TimelineMsgingType>& dr) 
{
    // QoT Message type
    qot_message_t qot_msg;
    // get only new/unread data
    dds::sub::status::DataState aliveDataState;
            aliveDataState << dds::sub::status::SampleState::any()
                << dds::sub::status::ViewState::any()
                << dds::sub::status::InstanceState::alive();
    /**
     * Take messages. Using take instead of read removes the messages from
     * the system, preventing resources from being saturated due to a build
     * up of messages
     */
    dds::sub::LoanedSamples<qot_msgs::TimelineMsgingType> messages
        = dr.select().state(aliveDataState).take();

    /** Output the username and content for each message */
    for (dds::sub::LoanedSamples<qot_msgs::TimelineMsgingType>::const_iterator message
            = messages.begin(); message < messages.end(); ++message)
    {
        if(message->info().valid())
        {
            strcpy(qot_msg.name, (message->data().name()).c_str());
            qot_msg.type = (qot_msg_type_t) message->data().type();
            strcpy(qot_msg.data, (message->data().data()).c_str());
            TP_FROM_nSEC(qot_msg.timestamp.estimate, message->data().utimestamp().timestamp());
            TL_FROM_nSEC(qot_msg.timestamp.interval.above, message->data().utimestamp().uncertainty_u());
            TL_FROM_nSEC(qot_msg.timestamp.interval.below, message->data().utimestamp().uncertainty_l());
            if(msg_callback != NULL)
            {
                msg_callback(&qot_msg);
            }
        }
    }
}

void SubscriberImpl::on_liveliness_changed(dds::sub::DataReader<qot_msgs::TimelineMsgingType>& dr, 
    const dds::core::status::LivelinessChangedStatus& status) 
{
    // according to C++ API reference, this function gets called when the
    // 'liveliness' of a DataWriter writing to this DataReader has changed
    // "changed" meaning has become 'alive' or 'not alive'.
    // 
    // I think this can detect when new writers have joined the board/topic
    // and when current writers have died. (maybe)
}



