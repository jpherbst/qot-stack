 /*
 * @file ZookeeperInterface.hpp
 * @brief Edge Broker Management Zookeeper Interface Header
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

#ifndef _zk_interface_h
#define _zk_interface_h

#ifdef __cplusplus
extern "C"
{
#endif

/* Create a Timeline zk node */
void zk_timeline_create(const char* timelineName);

/* Delete a Timeline zk node */
void zk_timeline_delete(const char* timelineName)
{

}

/* Create a Topic zk node */
void zk_topic_create(const char* timelineName, const char* topicName);

/* Delete a Topic zk node */
void zk_topic_delete(const char* timelineName, const char* topicName);

/* Create a Publisher zk node */ 
void zk_publisher_create(const char* timelineName, const char* topicName, const char* publisherName); 

/* Delete a Publisher zk node */ 
void zk_publisher_delete(const char* timelineName, const char* topicName, const char* publisherName); 

/* Create a Subscriber zk node */ 
void zk_subscriber_create(const char* timelineName, const char* topicName, const char* subscriberName);

/* Delete a Subscriber zk node */ 
void zk_subscriber_delete(const char* timelineName, const char* topicName, const char* subscriberName);

#ifdef __cplusplus
}
#endif
    
#endif