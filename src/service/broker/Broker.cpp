/**
 * @file Broker.cpp
 * @brief DDS Broker for interfacing with WAN-scale Pub-Sub
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
 *
 * The code in this file is an adaptation of the ISO C++ DCPS API BuiltinTopics example from Opensplice DDS 
 * Note: When a topic, publisher or subscriber is created we "read" the data,
 *       and when they are destroyed we "take" the available data  
 *
 */
#include "Broker.hpp"
#include "Handlers.hpp"

#include <iostream>
#include <string>
#include <iterator>
#include <algorithm>
#include <signal.h>

using namespace qot_broker;

/* Vector of Discovered Nodes */
std::vector<NodeInfo> nodes;

/* Vector of Discovered Topics */
std::vector<TopicInfo> topics;

/* NodeInfo Class Constructor */
NodeInfo::NodeInfo(const DDS::ParticipantBuiltinTopicData& participantData)
{
    nodeID_ = participantData.key[0];
}

/**
 * Finds a node a node with an ID obtained from the ParticipantBuiltinTopicData, updates the
 * hostname and returns the node. If no node is found then a new one is created and returned.
 *
 * @param participantData The ParticipantBuiltinTopicData of the node
 * @return The found/created node
 */
NodeInfo& getNodeInfo(const DDS::ParticipantBuiltinTopicData& participantData)
{
    /** Find and return the node if it already exists */
    for(std::vector<NodeInfo>::iterator it = nodes.begin(); it < nodes.end(); it++)
    {
        if(it->nodeID_ == participantData.key[0])
        {
            /** Update the hostname */
            if(participantData.user_data.value.length() > 0
            && *participantData.user_data.value.get_buffer() != '<' )
            {
                it->hostName_ = std::string(reinterpret_cast<const char*> (participantData.user_data.value.get_buffer()),
                                        participantData.user_data.value.length());
            }
            return *it;
        }
    }

    /** Create a new node and return it if no node existed */
    NodeInfo node(participantData);
    nodes.push_back(node);
    return nodes.back();
}

/* TopicInfo Class Constructor */
TopicInfo::TopicInfo(const DDS::TopicBuiltinTopicData& topicData)
{
    topicID_ = topicData.key[0];
    topicName_ = topicData.name;
}

/* Function to return topic name */
std::string TopicInfo::getTopicName()
{
    return topicName_;
}

/* Function to return topic ID */
int32_t TopicInfo::getTopicID()
{
    return topicID_;
}

/* Add a subscriber */
int TopicInfo::addSubscriber(int32_t participant_ID, int32_t dr_ID)
{
    if(subscribers.find(dr_ID) != subscribers.end())
    {
        std::cout << "Subscriber "<< dr_ID << " already exists for this topic" << std::endl;
        return -1;
    }
    subscribers[dr_ID] = participant_ID;
    return 0;
}

/* Add a publisher */
int TopicInfo::addPublisher(int32_t participant_ID, int32_t dw_ID)
{
    if(publishers.find(dw_ID) != publishers.end())
    {
        std::cout << "Publisher "<< dw_ID << " already exists for this topic" << std::endl;
        return -1;
    }
    publishers[dw_ID] = participant_ID;
    return 0;
}

/* Remove a subscriber */
int TopicInfo::removeSubscriber(int32_t participant_ID, int32_t dr_ID)
{
    std::map<int32_t, int32_t>::iterator it;
    it = subscribers.find(dr_ID);
    if(it == subscribers.end())
    {
        std::cout << "Subscriber "<< dr_ID <<  " does not exist for this topic" << std::endl;
        return -1;
    }
    subscribers.erase(it);
    return 0;
}

/* Remove a publisher */
int TopicInfo::removePublisher(int32_t participant_ID, int32_t dw_ID)
{
    std::map<int32_t, int32_t>::iterator it;
    it = publishers.find(dw_ID);
    if(it == publishers.end())
    {
        std::cout << "Publisher "<< dw_ID << " does not exist for this topic" << std::endl;
        return -1;
    }
    publishers.erase(it);
    return 0;
}

/**
 * Check if a topic is a "global" topic
 *
 * @param topicData The TopicBuiltinTopicData of the node
 * @return bool indicating success/failure
 */
 bool checkTopicInfo(const DDS::TopicBuiltinTopicData& topicData)
 {
    std::string topicName_(topicData.name);
    std::size_t pos_gl = topicName_.find("gl_");
    std::size_t pos_go = topicName_.find("go_");

    if (pos_gl == 0 || pos_go == 0)
        return true;
    return false;
 }

/**
 * Finds a topic with an ID obtained from the TopicBuiltinTopicData, updates the
 * hostname and returns the node. If no node is found then a new one is created and returned.
 *
 * @param topicData The TopicBuiltinTopicData of the node
 * @return The found/created node
 */
TopicInfo& getTopicInfo(const DDS::TopicBuiltinTopicData& topicData)
{
    /** Find and return the node if it already exists */
    for(std::vector<TopicInfo>::iterator it = topics.begin(); it < topics.end(); it++)
    {
        if(it->getTopicID() == topicData.key[0])
        {
            /* If the topic is found return the topic */
            return *it;
        }
    }

    /** Create a new node and return it if no node existed */
    TopicInfo node(topicData);
    topics.push_back(node);
    return topics.back();
}

/**
 * Finds a topic with an ID obtained from the TopicBuiltinTopicData
 * If a node is found then it is removed from the vector, and destroyed.
 *
 * @param topicData The TopicBuiltinTopicData of the node
 * @return The found/created node
 */
bool deleteTopicInfo(const DDS::TopicBuiltinTopicData& topicData)
{
    /** Find and return the node if it already exists */
    for(std::vector<TopicInfo>::iterator it = topics.begin(); it < topics.end(); it++)
    {
        if(it->getTopicID() == topicData.key[0])
        {
            /* If the topic is found erase the topic and return true */
            topics.erase(it);
            return true;
        }
    }

    /** Return false if the node does not exist */
    return false;
}

/**
 * Finds a topic with a name obtained from the PublicationBuiltinTopicData, updates the
 * hostname and returns the node. If no node is found then a new one is created and returned.
 *
 * @param topicData The TopicBuiltinTopicData of the node
 * @return The found/created node
 */
TopicInfo* getPublicationTopicInfo(const DDS::PublicationBuiltinTopicData& publisherData)
{
    /** Find and return the node if it already exists */
    for(std::vector<TopicInfo>::iterator it = topics.begin(); it < topics.end(); it++)
    {
        if(it->getTopicName().compare(publisherData.topic_name) == 0)
        {
            /* If the topic is found return the topic */
            return &(*it);
        }
    }
    return NULL;
}

/**
 * Finds a topic with a name obtained from the SubscriptionBuiltinTopicData, updates the
 * hostname and returns the node. If no node is found then a new one is created and returned.
 *
 * @param topicData The TopicBuiltinTopicData of the node
 * @return The found/created node
 */
TopicInfo* getSubscriptionTopicInfo(const DDS::SubscriptionBuiltinTopicData& subscriberData)
{
    /** Find and return the node if it already exists */
    for(std::vector<TopicInfo>::iterator it = topics.begin(); it < topics.end(); it++)
    {
        if(it->getTopicName().compare(subscriberData.topic_name) == 0)
        {
            /* If the topic is found return the topic */
            return &(*it);
        }
    }
    return NULL;
}

/* Constructor of the Broker Class */
Broker::Broker()
    : dp(DDS::DOMAIN_ID_DEFAULT), kill(false), builtinSubscriber(dds::sub::builtin_subscriber(dp))
{
    std::cout << "Pub Sub Broker starting up\n";

    // We can now start polling, because the broker is setup
    //participant_thread = boost::thread(boost::bind(&Broker::ParticipantThread, this));
    topic_thread = boost::thread(boost::bind(&Broker::TopicThread, this));
    publisher_thread = boost::thread(boost::bind(&Broker::PublisherThread, this));
    subscriber_thread = boost::thread(boost::bind(&Broker::SubscriberThread, this));
}

Broker::~Broker()
{
    // Kill all broker threads and wait for them to join before destroying the class
    std::cout << "Pub Sub Broker being destroyed\n";
    this->kill = true;
    //this->participant_thread.join();
    this->topic_thread.join();
    this->publisher_thread.join();
    this->subscriber_thread.join();
}

/* Participant Thread */
void Broker::ParticipantThread()
{
    // Wait until the main thread sets up the binding and posix clock
    std::cout << "Polling for participants" << std::endl;

    /** Get the ParticipantBuiltinTopicDataReader */
    std::string topicName = "DCPSParticipant";
    std::vector<dds::sub::DataReader<DDS::ParticipantBuiltinTopicData> > prv;
    dds::sub::DataReader<DDS::ParticipantBuiltinTopicData> participantReader(dds::core::null);
    if(dds::sub::find<dds::sub::DataReader<DDS::ParticipantBuiltinTopicData>,
        std::back_insert_iterator<std::vector<dds::sub::DataReader<DDS::ParticipantBuiltinTopicData> > > >(
            builtinSubscriber, topicName,
            std::back_inserter<std::vector<dds::sub::DataReader<DDS::ParticipantBuiltinTopicData> > >(
                prv)) > 0)
    {
        participantReader = prv[0];
    }
    else
    {
        throw "Could not get ParticipantBuiltinTopicDataReader";
    }

    std::cout << "=== [BuiltInTopicsDataSubscriber] Waiting for historical data ... " << std::endl;

    /* Make sure all historical data is delivered in the DataReader */
    participantReader.wait_for_historical_data(dds::core::Duration::infinite());

    std::cout << "=== [BuiltInTopicsDataSubscriber] Done" << std::endl;

    /* Create a new ReadCondition for the reader that matches all samples.*/
    dds::sub::cond::ReadCondition readCond(participantReader, dds::sub::status::DataState::any());

    /* Create a WaitSet and attach the ReadCondition created above */
    dds::core::cond::WaitSet waitSet;
    waitSet += readCond;

    std::cout << "=== [BuiltInTopicsDataSubscriber] Ready ..." << std::endl;

    /*
    * Block the current thread until the attached condition becomes true
    * or the user interrupts.
    */
    waitSet.wait();

    bool done = false;
    // Start polling
    while (!this->kill && !done)
    {
        /**
        * Take all data from the reader and iterate through it
        */
        dds::sub::LoanedSamples<DDS::ParticipantBuiltinTopicData> samples = participantReader.take();
        for (dds::sub::LoanedSamples<DDS::ParticipantBuiltinTopicData>::const_iterator sample = samples.begin();
            sample < samples.end(); ++sample)
        {
            if(sample->info().valid())
            {
                NodeInfo& nodeInfo = getNodeInfo(sample->data());

                /**
                * Check if the participant is alive and add it to the nodeInfo
                */
                if(sample->info().state().instance_state() == dds::sub::status::InstanceState::alive())
                {
                    nodeInfo.participants.push_back(sample->data().key[1]);
                    /**
                    * If this is the first participant, output that the node is running
                    */
                    if(nodeInfo.participants.size() == 1)
                    {
                        std::cout << "=== [BuiltInTopicsDataSubscriber] Node '"
                                    << nodeInfo.nodeID_ << "' started (Total nodes running: "
                                    << nodes.size() << ")" << std::endl;
                    }

                    /**
                    * Output info for each node
                    */
                    if(nodeInfo.hostName_.length() > 0
                        && sample->data().user_data.value.length() > 0
                        && *(sample->data().user_data.value.get_buffer()) != '<')
                    {
                        std::cout << "=== [BuiltInTopicsDataSubscriber] Hostname for node '"
                                    << nodeInfo.nodeID_ << "' is '" << nodeInfo.hostName_
                                    << "'." << std::endl;
                    }
                }
                /**
                * If the participant is not alive remove it from the nodeInfo
                */
                else
                {
                    nodeInfo.participants.erase(std::remove(nodeInfo.participants.begin(),
                                                            nodeInfo.participants.end(),
                                                            sample->data().key[1]),
                                                nodeInfo.participants.end());

                    /**
                    * If no more participants exist, output that the node has stopped
                    */
                    if(nodeInfo.participants.size() == 0)
                    {
                        std::cout << "=== [BuiltInTopicsDataSubscriber] Node "
                                    << nodeInfo.nodeID_ << " (" << nodeInfo.hostName_
                                    << ") stopped (Total nodes running: "
                                    << nodes.size() << ")" << std::endl;
                    }
                }
            }
        }

        for(std::vector<NodeInfo>::iterator it = nodes.begin(); it < nodes.end(); it++)
        {
            if(it->participants.size() > 0)
            {
                std::cout << "=== [BuiltInTopicsDataSubscriber] Node '" << it->nodeID_
                            << "' has '" << it->participants.size()
                            << "' participants." << std::endl;
            }
        }

        /* Block the current thread until the attached condition becomes
        *  true or the user interrupts.
        */
        std::cout << "=== [BuiltInTopicsDataSubscriber] Waiting ... " << std::endl;
        try
        {
            waitSet.wait();
        }
        catch(...)
        {
            done = true;
        }
    }
}

/* Callback Function for handling new topics */
void newTopicCallback(dds::sub::DataReader<DDS::TopicBuiltinTopicData>& topicReader, dds::sub::status::DataState& dataState)
{
    dds::sub::LoanedSamples<DDS::TopicBuiltinTopicData> samples = topicReader.select().state(dataState).read();
    for (dds::sub::LoanedSamples<DDS::TopicBuiltinTopicData>::const_iterator sample = samples.begin();
        sample < samples.end(); ++sample)
    {
        if(sample->info().valid())
        {
            /* Check if a topic is designated as global or global optimized */
            bool status = checkTopicInfo(sample->data());

            if (status)
            {
                TopicInfo& topicInfo = getTopicInfo(sample->data());

            
                std::cout << "=== [Topic BuiltInTopics Thread] Created Topic '"
                                    << topicInfo.getTopicID() << "' name: "
                                    << topicInfo.getTopicName() << std::endl;
            }  
        }
    }
}

/* Callback Function for handling topic deletion */
void deletedTopicCallback(dds::core::Entity& e, dds::sub::status::DataState& dataState)
{
    dds::sub::DataReader<DDS::TopicBuiltinTopicData>& topicReader
        = (dds::sub::DataReader<DDS::TopicBuiltinTopicData>&)e;

    dds::sub::LoanedSamples<DDS::TopicBuiltinTopicData> samples = topicReader.select().state(dataState).take();
    for (dds::sub::LoanedSamples<DDS::TopicBuiltinTopicData>::const_iterator sample = samples.begin();
        sample < samples.end(); ++sample)
    {
        if(sample->info().valid())
        {
            /* Check if a topic is designated as global or global optimized */
            bool found = checkTopicInfo(sample->data());

            if (found)
            {
                bool status = deleteTopicInfo(sample->data());

                int32_t topicID_ = sample->data().key[0];                         
                std::string topicName_(sample->data().name);     
                
                std::cout << "=== [Topic BuiltInTopics Thread] Destroyed Topic '"
                                    << topicID_ << "' name: "
                                    << topicName_ << std::endl;
            }
        }
    }
}

/* Topic Thread */
void Broker::TopicThread()
{
    // Wait until the main thread sets up the binding and posix clock
    std::cout << "Polling for topics" << std::endl;

    std::string topicName = "DCPSTopic";
    std::vector<dds::sub::DataReader<DDS::TopicBuiltinTopicData> > prv;
    dds::sub::DataReader<DDS::TopicBuiltinTopicData> topicReader(dds::core::null);
    if(dds::sub::find<dds::sub::DataReader<DDS::TopicBuiltinTopicData>,
        std::back_insert_iterator<std::vector<dds::sub::DataReader<DDS::TopicBuiltinTopicData> > > >(
            builtinSubscriber, topicName,
            std::back_inserter<std::vector<dds::sub::DataReader<DDS::TopicBuiltinTopicData> > >(
                prv)) > 0)
    {
        topicReader = prv[0];
    }
    else
    {
        throw "Could not get TopicBuiltinTopicDataReader";
    }

    std::cout << "=== [Topic BuiltInTopics Thread] Waiting for historical data ... " << std::endl;

    /* Make sure all historical data is delivered in the DataReader */
    topicReader.wait_for_historical_data(dds::core::Duration::infinite());

    std::cout << "=== [Topic BuiltInTopics Thread] Done" << std::endl;

    /* Specify the state to look for new data */
    dds::sub::status::DataState newDataState;
    newDataState << dds::sub::status::SampleState::not_read()
            << dds::sub::status::ViewState::new_view()
            << dds::sub::status::InstanceState::alive();

    /* Create the Handler for new data */
    ReadTopicHandler newDataHandler(newTopicCallback, newDataState);

    /* Create a new ReadCondition for the new data reader */
    dds::sub::cond::ReadCondition dataNewCond(topicReader, newDataState, newDataHandler);
    
    /* Specify the state to look for deleted data */
    dds::sub::status::DataState notAliveDataState;
    notAliveDataState << dds::sub::status::SampleState::any()
            << dds::sub::status::ViewState::any()
            << dds::sub::status::InstanceState::not_alive_mask();

    /* Create the Handler for deleted data */
    DeletionHandler deletedDataHandler(deletedTopicCallback, notAliveDataState);

    /* Create a new StatusCondition for the deleted data reader */
    dds::core::cond::StatusCondition dataDeletedCond(topicReader, deletedDataHandler);
    dataDeletedCond.enabled_statuses(dds::core::status::StatusMask::data_available());

    /* Create a WaitSet and attach the ReadCondition (s) created above */
    dds::core::cond::WaitSet waitSet;
    waitSet += dataDeletedCond;
    waitSet += dataNewCond;

    std::cout << "=== [Topic BuiltInTopics Thread] Ready ..." << std::endl;

    bool done = false;
    // Start polling
    while (!this->kill && !done)
    {
        /* Block the current thread until the attached condition becomes
        *  true or the user interrupts.
        */
        try
        {
            /* Dispatch Waitset */
            waitSet.dispatch();
        }
        catch(...)
        {
            done = true;
        }
    }
}

/* Callback Function for handling new publishers */
void newPubCallback(dds::sub::DataReader<DDS::PublicationBuiltinTopicData>& topicReader, dds::sub::status::DataState& dataState)
{
    dds::sub::LoanedSamples<DDS::PublicationBuiltinTopicData> samples = topicReader.select().state(dataState).read();
    for (dds::sub::LoanedSamples<DDS::PublicationBuiltinTopicData>::const_iterator sample = samples.begin();
        sample < samples.end(); ++sample)
    {
        if(sample->info().valid())
        {
            TopicInfo* topicInfo = getPublicationTopicInfo(sample->data());

            // Check if the topic is relevant (global or global optimized) to be shared
            if(topicInfo)
            {
                std::cout << "=== [Publisher BuiltInTopics Thread] Publisher added on Topic '"
                                << topicInfo->getTopicID() << "' name: "
                                << topicInfo->getTopicName() << std::endl;
                topicInfo->addPublisher(sample->data().participant_key[0], sample->data().key[0]); 
            }    
               
        }
    }
}

/* Callback Function for handling publisher deletion */
void deletedPubCallback(dds::core::Entity& e, dds::sub::status::DataState& dataState)
{
    dds::sub::DataReader<DDS::PublicationBuiltinTopicData>& topicReader
        = (dds::sub::DataReader<DDS::PublicationBuiltinTopicData>&)e;

    dds::sub::LoanedSamples<DDS::PublicationBuiltinTopicData> samples = topicReader.select().state(dataState).take();
    for (dds::sub::LoanedSamples<DDS::PublicationBuiltinTopicData>::const_iterator sample = samples.begin();
        sample < samples.end(); ++sample)
    {
        if(sample->info().valid())
        {
            TopicInfo* topicInfo = getPublicationTopicInfo(sample->data());

            // Check if the topic is relevant (global or global optimized) to be shared
            if(topicInfo)
            {
                std::cout << "=== [Publisher BuiltInTopics Thread] Publisher removed on Topic '"
                                << topicInfo->getTopicID() << "' name: "
                                << topicInfo->getTopicName() << std::endl;
                topicInfo->removePublisher(sample->data().participant_key[0], sample->data().key[0]); 
            }  
        }
    }
}


/* Publisher Thread */
void Broker::PublisherThread()
{
    // Wait until the main thread sets up the binding and posix clock
    std::cout << "Polling for publishers" << std::endl;

    std::string topicName = "DCPSPublication";
    std::vector<dds::sub::DataReader<DDS::PublicationBuiltinTopicData> > prv;
    dds::sub::DataReader<DDS::PublicationBuiltinTopicData> publisherReader(dds::core::null);
    if(dds::sub::find<dds::sub::DataReader<DDS::PublicationBuiltinTopicData>,
        std::back_insert_iterator<std::vector<dds::sub::DataReader<DDS::PublicationBuiltinTopicData> > > >(
            builtinSubscriber, topicName,
            std::back_inserter<std::vector<dds::sub::DataReader<DDS::PublicationBuiltinTopicData> > >(
                prv)) > 0)
    {
        publisherReader = prv[0];
    }
    else
    {
        throw "Could not get PublicationBuiltinTopicDataReader";
    }

    std::cout << "=== [Publisher BuiltInTopics Thread] Waiting for historical data ... " << std::endl;

    /* Make sure all historical data is delivered in the DataReader */
    publisherReader.wait_for_historical_data(dds::core::Duration::infinite());

    std::cout << "=== [Publisher BuiltInTopics Thread] Done" << std::endl;

    /* Specify the state to look for new data */
    dds::sub::status::DataState newDataState;
    newDataState << dds::sub::status::SampleState::not_read()
            << dds::sub::status::ViewState::new_view()
            << dds::sub::status::InstanceState::alive();

    /* Create the Handler for new data */
    ReadPublisherHandler newDataHandler(newPubCallback, newDataState);

    /* Create a new ReadCondition for the new data reader */
    dds::sub::cond::ReadCondition dataNewCond(publisherReader, newDataState, newDataHandler);
    
    /* Specify the state to look for deleted data */
    dds::sub::status::DataState notAliveDataState;
    notAliveDataState << dds::sub::status::SampleState::any()
            << dds::sub::status::ViewState::any()
            << dds::sub::status::InstanceState::not_alive_mask();

    /* Create the Handler for deleted data */
    DeletionHandler deletedDataHandler(deletedPubCallback, notAliveDataState);

    /* Create a new StatusCondition for the deleted data reader */
    dds::core::cond::StatusCondition dataDeletedCond(publisherReader, deletedDataHandler);
    dataDeletedCond.enabled_statuses(dds::core::status::StatusMask::data_available());

    /* Create a WaitSet and attach the ReadCondition (s) created above */
    dds::core::cond::WaitSet waitSet;
    waitSet += dataDeletedCond;
    waitSet += dataNewCond;

    std::cout << "=== [Publisher BuiltInTopics Thread] Ready ..." << std::endl;

    bool done = false;
    // Start polling
    while (!this->kill && !done)
    {
        /* Block the current thread until the attached condition becomes
        *  true or the user interrupts.
        */
        try
        {
            /* Dispatch Waitset */
            waitSet.dispatch();
        }
        catch(...)
        {
            done = true;
        }
    }
}

/* Callback Function for handling new publishers */
void newSubCallback(dds::sub::DataReader<DDS::SubscriptionBuiltinTopicData>& topicReader, dds::sub::status::DataState& dataState)
{
    dds::sub::LoanedSamples<DDS::SubscriptionBuiltinTopicData> samples = topicReader.select().state(dataState).read();
    for (dds::sub::LoanedSamples<DDS::SubscriptionBuiltinTopicData>::const_iterator sample = samples.begin();
        sample < samples.end(); ++sample)
    {
        if(sample->info().valid())
        {
            TopicInfo* topicInfo = getSubscriptionTopicInfo(sample->data());

            // Check if the topic is relevant (global or global optimized) to be shared
            if(topicInfo)
            {
                std::cout << "=== [Subscriber BuiltInTopics Thread] Subsciber added on Topic '"
                                << topicInfo->getTopicID() << "' name: "
                                << topicInfo->getTopicName() << std::endl;
                topicInfo->addSubscriber(sample->data().participant_key[0], sample->data().key[0]); 
            }    
               
        }
    }
}

/* Callback Function for handling publisher deletion */
void deletedSubCallback(dds::core::Entity& e, dds::sub::status::DataState& dataState)
{
    dds::sub::DataReader<DDS::SubscriptionBuiltinTopicData>& topicReader
        = (dds::sub::DataReader<DDS::SubscriptionBuiltinTopicData>&)e;

    dds::sub::LoanedSamples<DDS::SubscriptionBuiltinTopicData> samples = topicReader.select().state(dataState).take();
    for (dds::sub::LoanedSamples<DDS::SubscriptionBuiltinTopicData>::const_iterator sample = samples.begin();
        sample < samples.end(); ++sample)
    {
        if(sample->info().valid())
        {
            TopicInfo* topicInfo = getSubscriptionTopicInfo(sample->data());

            // Check if the topic is relevant (global or global optimized) to be shared
            if(topicInfo)
            {
                std::cout << "=== [Subscriber BuiltInTopics Thread] Subsciber removed on Topic '"
                                << topicInfo->getTopicID() << "' name: "
                                << topicInfo->getTopicName() << std::endl;
                topicInfo->removeSubscriber(sample->data().participant_key[0], sample->data().key[0]); 
            }  
        }
    }
}

/* Participant Thread */
void Broker::SubscriberThread()
{
    // Wait until the main thread sets up the binding and posix clock
    std::cout << "Polling for subscribers" << std::endl;
    
    std::string topicName = "DCPSSubscription";
    std::vector<dds::sub::DataReader<DDS::SubscriptionBuiltinTopicData> > prv;
    dds::sub::DataReader<DDS::SubscriptionBuiltinTopicData> subscriberReader(dds::core::null);
    if(dds::sub::find<dds::sub::DataReader<DDS::SubscriptionBuiltinTopicData>,
        std::back_insert_iterator<std::vector<dds::sub::DataReader<DDS::SubscriptionBuiltinTopicData> > > >(
            builtinSubscriber, topicName,
            std::back_inserter<std::vector<dds::sub::DataReader<DDS::SubscriptionBuiltinTopicData> > >(
                prv)) > 0)
    {
        subscriberReader = prv[0];
    }
    else
    {
        throw "Could not get SubscriptionBuiltinTopicDataReader";
    }

    std::cout << "=== [Subsciber BuiltInTopics Thread] Waiting for historical data ... " << std::endl;

    /* Make sure all historical data is delivered in the DataReader */
    subscriberReader.wait_for_historical_data(dds::core::Duration::infinite());

    /* Specify the state to look for new data */
    dds::sub::status::DataState newDataState;
    newDataState << dds::sub::status::SampleState::not_read()
            << dds::sub::status::ViewState::new_view()
            << dds::sub::status::InstanceState::alive();

    /* Create the Handler for new data */
    ReadSubscriberHandler newDataHandler(newSubCallback, newDataState);

    /* Create a new ReadCondition for the new data reader */
    dds::sub::cond::ReadCondition dataNewCond(subscriberReader, newDataState, newDataHandler);
    
    /* Specify the state to look for deleted data */
    dds::sub::status::DataState notAliveDataState;
    notAliveDataState << dds::sub::status::SampleState::any()
            << dds::sub::status::ViewState::any()
            << dds::sub::status::InstanceState::not_alive_mask();

    /* Create the Handler for deleted data */
    DeletionHandler deletedDataHandler(deletedSubCallback, notAliveDataState);

    /* Create a new StatusCondition for the deleted data reader */
    dds::core::cond::StatusCondition dataDeletedCond(subscriberReader, deletedDataHandler);
    dataDeletedCond.enabled_statuses(dds::core::status::StatusMask::data_available());

    /* Create a WaitSet and attach the ReadCondition (s) created above */
    dds::core::cond::WaitSet waitSet;
    waitSet += dataDeletedCond;
    waitSet += dataNewCond;

    std::cout << "=== [Subsciber BuiltInTopics Thread] Ready ..." << std::endl;

    bool done = false;
    // Start polling
    while (!this->kill && !done)
    {
        /* Block the current thread until the attached condition becomes
        *  true or the user interrupts.
        */
        try
        {
            /* Dispatch Waitset */
            waitSet.dispatch();
        }
        catch(...)
        {
            done = true;
        }
    }
}

static int running = 1;

static void exit_handler(int s)
{
    printf("Exit requested \n");
    running = 0;
}

/**
 * Main Function
 */
int main(int argc, char *argv[])
{
    int result = 0;
    bool automatic = true;
    if (argc > 1)
    {
        automatic = !(strcmp(argv[1], "false") == 0);
    }

    // Install SIGINT Signal Handler for exit
    signal(SIGINT, exit_handler);

    try
    {
        Broker broker;
        while(running)
        {
            sleep(5);
        }
        // Destroy threads
        broker.~Broker();
    }
    catch (const dds::core::Exception& e)
    {
        std::cerr << "ERROR: Exception: " << e.what() << std::endl;
        result = 1;
    }
    catch (const std::string& e)
    {
        std::cerr << "ERROR: " << e << std::endl;
        result = 1;
    }
    return result;
}


