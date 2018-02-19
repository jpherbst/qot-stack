/*
 * @file zk_logic.h
 * @brief Edge Broker Management Zookeeper Logic Header
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

#ifndef _zk_logic_h
#define _zk_logic_h

#ifdef __cplusplus
extern "C"
{
#endif

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>

#include <zookeeper.h>
#include <zookeeper_log.h>
#include <zookeeper.jute.h>

/*
 * Global Variables
 */
static const char *hostPort;					// Comma separated host:port Pairs of Zookeeper servers			
static const char *brokerGroup;					// Globally unique name of the edge broker
static zhandle_t *zh;							// Zookeeper connection handle
static int connected = 0;						// Variable indicating if connected to server
static int expired = 0;							// Variable indicating if connection has expired
static int leader = 0;							// Variable indicating if this edge broker is the local leader (active node)
static int server_id;
static struct String_vector* timelines = NULL;

/*
 * Master Edge Broker Election Function definitions.
 */

void run_for_master();
void check_master();
void master_exists();

/*
 * Master states
 */
enum master_states {
    RUNNING,
    ELECTED,
    NOTELECTED
};

static enum master_states state;

enum master_states get_state () {
    return state;
}

/*
 * Timeline Data Structure to Hold Publishers and Subscribers  
 */

struct timeline {
	char* name;
};

/*
 * Topic Data Structure to Hold Publishers and Subscribers  
 */

struct topic {
	char* name;
	struct String_vector* publishers;
	struct String_vector* subscribers;
};


/*
 * The following two methods convert
 * event types and return codes, respectively,
 * to strings.
 */

static const char * type2string(int type){
    if (type == ZOO_CREATED_EVENT)
        return "CREATED_EVENT";
    if (type == ZOO_DELETED_EVENT)
        return "DELETED_EVENT";
    if (type == ZOO_CHANGED_EVENT)
        return "CHANGED_EVENT";
    if (type == ZOO_CHILD_EVENT)
        return "CHILD_EVENT";
    if (type == ZOO_SESSION_EVENT)
        return "SESSION_EVENT";
    if (type == ZOO_NOTWATCHING_EVENT)
        return "NOTWATCHING_EVENT";
    
    return "UNKNOWN_EVENT_TYPE";
}

static const char * rc2string(int rc){
    if (rc == ZOK) {
        return "OK";
    }
    if (rc == ZSYSTEMERROR) {
        return "System error";
    }
    if (rc == ZRUNTIMEINCONSISTENCY) {
        return "Runtime inconsistency";
    }
    if (rc == ZDATAINCONSISTENCY) {
        return "Data inconsistency";
    }
    if (rc == ZCONNECTIONLOSS) {
        return "Connection to the server has been lost";
    }
    if (rc == ZMARSHALLINGERROR) {
        return "Error while marshalling or unmarshalling data ";
    }
    if (rc == ZUNIMPLEMENTED) {
        return "Operation not implemented";
    }
    if (rc == ZOPERATIONTIMEOUT) {
        return "Operation timeout";
    }
    if (rc == ZBADARGUMENTS) {
        return "Invalid argument";
    }
    if (rc == ZINVALIDSTATE) {
        return "Invalid zhandle state";
    }
    if (rc == ZAPIERROR) {
        return "API error";
    }
    if (rc == ZNONODE) {
        return "Znode does not exist";
    }
    if (rc == ZNOAUTH) {
        return "Not authenticated";
    }
    if (rc == ZBADVERSION) {
        return "Version conflict";
    }
    if (rc == ZNOCHILDRENFOREPHEMERALS) {
        return "Ephemeral nodes may not have children";
    }
    if (rc == ZNODEEXISTS) {
        return "Znode already exists";
    }
    if (rc == ZNOTEMPTY) {
        return "The znode has children";
    }
    if (rc == ZSESSIONEXPIRED) {
        return "The session has been expired by the server";
    }
    if (rc == ZINVALIDCALLBACK) {
        return "Invalid callback specified";
    }
    if (rc == ZINVALIDACL) {
        return "Invalid ACL specified";
    }
    if (rc == ZAUTHFAILED) {
        return "Client authentication failed";
    }
    if (rc == ZCLOSING) {
        return "ZooKeeper session is closing";
    }
    if (rc == ZNOTHING) {
        return "No response from server";
    }
    if (rc == ZSESSIONMOVED) {
        return "Session moved to a different server";
    }

    return "UNKNOWN_EVENT_TYPE";
}

#ifdef __cplusplus
}
#endif

#endif
