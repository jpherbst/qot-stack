/*
 * @file zk_logic.c
 * @brief Edge Broker Management Zookeeper Logic
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

#include "zk_logic.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************************/
/*
 * Auxiliary functions
 */

/* Make a path to a znode by combining multiple strings */
char * make_path(int num, ...) 
{
    const char * tmp_string;
    
    va_list arguments;
    va_start ( arguments, num );
    
    int total_length = 0;
    int x;
    for ( x = 0; x < num; x++ ) {
        tmp_string = va_arg ( arguments, const char * );
        if(tmp_string != NULL) {
            LOG_DEBUG(("Counting path with this path %s (%d)", tmp_string, num));
            total_length += strlen(tmp_string);
        }
    }

    va_end ( arguments );

    char * path = (char*) malloc(total_length * sizeof(char) + 1);
    path[0] = '\0';
    va_start ( arguments, num );
    
    for ( x = 0; x < num; x++ ) {
        tmp_string = va_arg ( arguments, const char * );
        if(tmp_string != NULL) {
            LOG_DEBUG(("Counting path with this path %s",
                       tmp_string));
            strcat(path, tmp_string);
        }
    }

    return path;
}

/* Make a copy of a string vector */
struct String_vector* make_copy( const struct String_vector* vector ) 
{
    struct String_vector* tmp_vector = (struct String_vector*) malloc(sizeof(struct String_vector));
    
    tmp_vector->data = (char**) malloc(vector->count * sizeof(const char *));
    tmp_vector->count = vector->count;
    
    int i;
    for( i = 0; i < vector->count; i++) {
        tmp_vector->data[i] = strdup(vector->data[i]);
    }
    
    return tmp_vector;
}


/*
 * Allocate String_vector, copied from zookeeper.jute.c
 */
int allocate_vector(struct String_vector *v, int32_t len) 
{
    if (!len) {
        v->count = 0;
        v->data = 0;
    } else {
        v->count = len;
        v->data = (char**) calloc(sizeof(*v->data), len);
    }
    return 0;
}


/*
 * Functions to free memory
 */
void free_vector(struct String_vector* vector) 
{
    int i;
    
    // Free each string
    for(i = 0; i < vector->count; i++) {
        free(vector->data[i]);
    }
    
    // Free data
    free(vector -> data);
    
    // Free the struct
    free(vector);
}

/*
 * Functions to find if a node is contained within a string vector
 */

int contains(const char * child, const struct String_vector* children) 
{
  int i;
  for(i = 0; i < children->count; i++) {
    if(!strcmp(child, children->data[i])) {
      return 1;
    }
  }

  return 0;
}

/*
 * This function returns the elements that are new in current
 * compared to previous and update previous.
 */
struct String_vector* added_and_set(const struct String_vector* current,
                                    struct String_vector** previous) 
{
    struct String_vector* diff = (struct String_vector*) malloc(sizeof(struct String_vector));
    
    int count = 0;
    int i;
    for(i = 0; i < current->count; i++) {
        if (!contains(current->data[i], (*previous))) {
            count++;
        }
    }
    
    allocate_vector(diff, count);
    
    int prev_count = count;
    count = 0;
    for(i = 0; i < current->count; i++) {
        if (!contains(current->data[i], (* previous))) {
            diff->data[count] = (char*) malloc(sizeof(char) * strlen(current->data[i]) + 1);
            memcpy(diff->data[count++],
                   current->data[i],
                   strlen(current->data[i]));
        }
    }
    
    assert(prev_count == count);
    
    free_vector((struct String_vector*) *previous);
    (*previous) = make_copy(current);
    
    return diff; 
}

/*
 * This function returns the elements that are have been removed in
 * compared to previous and update previous.
 */
struct String_vector* removed_and_set(const struct String_vector* current,
                                      struct String_vector** previous) 
{    
    struct String_vector* diff = (struct String_vector*) malloc(sizeof(struct String_vector));
    
    int count = 0;
    int i;
    for(i = 0; i < (* previous)->count; i++) {
        if (!contains((* previous)->data[i], current)) {
            count++;
        }
    }
    
    allocate_vector(diff, count);
    
    int prev_count = count;
    count = 0;
    for(i = 0; i < (* previous)->count; i++) {
        if (!contains((* previous)->data[i], current)) {
            diff->data[count] = (char*) malloc(sizeof(char) * strlen((* previous)->data[i]));
            strcpy(diff->data[count++], (* previous)->data[i]);
        }
    }

    assert(prev_count == count);

    free_vector((struct String_vector*) *previous);
    (*previous) = make_copy(current);
    
    return diff;
}

/*
 * End of auxiliary functions, it is all related to zookeeper
 * from this point on.
 */
/*********************************************************************************/
/* 
 * Functions to get the status of the zookeeper session 
 */

/* Function to check if connected to zookeeper cluster */
int is_connected() 
{
    return connected;
}

/* Function to check if zookeeper session has expired */
int is_expired() 
{
    return expired;
}

/*********************************************************************************/


/**
 * Watcher we use to process session events. In particular,
 * when it receives a ZOO_CONNECTED_STATE event, we set the
 * connected variable so that we know that the session has
 * been established.
 */
void main_watcher (zhandle_t *zkh,
                   int type,
                   int state,
                   const char *path,
                   void* context)
{
    /*
     * zookeeper_init might not have returned, so we
     * use zkh instead.
     */
    if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_CONNECTED_STATE) {
            connected = 1;

            LOG_DEBUG(("Received a connected event."));
        } else if (state == ZOO_CONNECTING_STATE) {
            if(connected == 1) {
                LOG_WARN(("Disconnected."));
            }
            connected = 0;
        } else if (state == ZOO_EXPIRED_SESSION_STATE) {
            expired = 1;
            connected = 0;
            zookeeper_close(zkh);
        }
    }
    LOG_DEBUG(("Event: %s, %d", type2string(type), state));
}

/*********************************************************************************/
/* 
 * Edge Broker Master Election Routines 
 */

/* Take leadership as the master edge broker */
void take_leadership() 
{
    leader = 1;
}

/*
 * In the case of errors when trying to create the /master lock, we
 * need to check if the znode is there and its content.
 */

void master_check_completion (int rc, const char *value, int value_len,
                              const struct Stat *stat, const void *data) 
{
    int master_id;
    switch (rc) {
        case ZCONNECTIONLOSS:
        case ZOPERATIONTIMEOUT:
            check_master();
            
            break;
        case ZOK:
            sscanf(value, "%x", &master_id );
            if(master_id == server_id) {
                take_leadership();
                LOG_DEBUG(("Elected primary master"));
            } else {
                master_exists();
                LOG_DEBUG(("The primary is some other process"));
            }
            
            break;
        case ZNONODE:
            run_for_master();
            
            break;
        default:
	    LOG_ERROR(("Something went wrong when checking the master lock: %s", rc2string(rc)));
            
            break;
    }
}

void check_master () 
{
    char* path = make_path(2, "/brokers/" , brokerGroup);
    zoo_aget(zh,
             path,
             0,
             master_check_completion,
             NULL);
    free(path);
}


/*
 * Run for master.
 */

void run_for_master();

void master_exists_watcher (zhandle_t *zh,
                            int type,
                            int state,
                            const char *path,
                            void *watcherCtx) 
{
    if( type == ZOO_DELETED_EVENT) {
    	char* broker_path = make_path(2, "/brokers/" , brokerGroup);
        assert( !strcmp(path, broker_path) );
        run_for_master();
        free(broker_path);
    } else {
        LOG_DEBUG(("Watched event: ", type2string(type)));
    }
}

void master_exists_completion (int rc, const struct Stat *stat, const void *data) 
{
    switch (rc) {
        case ZCONNECTIONLOSS:
        case ZOPERATIONTIMEOUT:
            master_exists();
            
            break;
            
        case ZOK:
            break;
        case ZNONODE:
            LOG_INFO(("Previous master is gone, running for master"));
            run_for_master();

            break;
        default:
            LOG_WARN(("Something went wrong when executing exists: %s", rc2string(rc)));

            break;
    }
}

void master_exists() 
{
	char* path = make_path(2, "/brokers/" , brokerGroup);
	leader = 0;
    zoo_awexists(zh,
                 path,
                 master_exists_watcher,
                 NULL,
                 master_exists_completion,
                 NULL);
    free(path);
}

void master_create_completion (int rc, const char *value, const void *data) 
{
    switch (rc) {
        case ZCONNECTIONLOSS:
            check_master();
            
            break;
            
        case ZOK:
            take_leadership();
            
            break;
            
        case ZNODEEXISTS:
            master_exists();
            
            break;
            
        default:
            LOG_ERROR(("Something went wrong when running for master."));

            break;
    }
}

/* Edge Broker Registration function, and active/standby master election */
void run_for_master() 
{
    if(!connected) {
        LOG_WARN(("Client not connected to ZooKeeper"));

        return;
    }
    char* path = make_path(2, "/brokers/" , brokerGroup);
    
    printf("*********** Registering Broker *****************\n");
    
    char server_id_string[9];
    snprintf(server_id_string, 9, "%x", server_id);
    zoo_acreate(zh,
                path,
                (const char *) server_id_string,
                strlen(server_id_string) + 1,
                &ZOO_OPEN_ACL_UNSAFE,
                ZOO_EPHEMERAL,
                master_create_completion,
                NULL);
    free(path);
}

/*********************************************************************************/
/*
 * Routines to bootstrap initial parent znodes 
 */

/* Create a parent node */
void create_parent(const char * path, const char * value);

/*
 * Completion function for creating parent znodes.
 */
void create_parent_completion (int rc, const char * value, const void * data) 
{
    switch (rc) {
        case ZCONNECTIONLOSS:
            create_parent(value, (const char *) data);
            
            break;
        case ZOK:
            LOG_INFO(("Created parent node", value));

            break;
        case ZNODEEXISTS:
            LOG_WARN(("Node already exists"));
            
            break;
        default:
	  		LOG_ERROR(("Something went wrong when creating a parent node: %s, %s", value, rc2string(rc)));
            
            break;
    }
}

/* Create a parent node */
void create_parent(const char * path, const char * value) 
{
    zoo_acreate(zh,
                path,
                value,
                0,
                &ZOO_OPEN_ACL_UNSAFE,
                0,
                create_parent_completion,
                NULL);
}

/* Create a parent node synchronous call*/
int create_parent_sync(const char * path, const char * value) 
{
    int rc = ZNONODE;
    while (rc == ZNONODE)
    {
        rc = zoo_create(zh,
                        path,
                        value,
                        0,
                        &ZOO_OPEN_ACL_UNSAFE,
                        0,
                        NULL,
                        0);
    }
    return rc;
}

/*********************************************************************************/
/*
 * Routines to delete a znode
 */

/* znode deletion function declaration */
void delete_znode(const char *path) ;

/* Completion function called when the request to delete znode returns. */
void delete_znode_completion(int rc, const void *data) 
{
    switch (rc) {
        case ZCONNECTIONLOSS:
        case ZOPERATIONTIMEOUT:
            delete_znode((const char *) data);
        break;
        case ZOK:
            LOG_DEBUG(("Deleted node: %s", (char *) data));
            free((char *) data);
        break;
        default:
            LOG_ERROR(("Something went wrong when deleting node: %s",
            rc2string(rc)));
        break;
    }
}

/* znode deletion function*/
void delete_znode(const char *path) 
{
    if(path == NULL) 
        return;
    char * tmp_path = strdup(path);
    zoo_adelete(zh,
                tmp_path,
                -1,
                delete_znode_completion,
                (const void*) tmp_path);
}

/* znode deletion function*/
int sync_delete_znode(const char *path) 
{
    int rc;
    if(path == NULL) 
        return ZBADARGUMENTS;
    rc = zoo_delete(zh, path, -1);

    return rc;
}


/*********************************************************************************/
/*
 * Timeline Handling Logic
 */

/* Add a new Timeline to zookeeper*/
int zk_timeline_create(const char* timelineName) 
{
    if(!connected) {
        LOG_WARN(("Client not connected to ZooKeeper"));

        return ZCONNECTIONLOSS;
    }
    
   	char* path = make_path(2, "/timelines/" , timelineName);
    
    printf("Registering Timeline %s\n", timelineName);
    
    char* tmp_string;
    int rc = ZNONODE;

    /* Wait untill the node creation occurs */
    while (rc == ZNONODE)
    {
       rc = zoo_create(zh, 
                       path,
                       tmp_string,
                       0, 
                       &ZOO_OPEN_ACL_UNSAFE, 
                       0,
                       NULL, 
                       0); 
    }
    /* Possible Return Codes
     * ZOK operation completed successfully
     * ZNONODE the parent node does not exist.
     * ZNODEEXISTS the node already exists
     * ZNOAUTH the client does not have permission.
     */
    free(path);
    return rc;
}

void zk_timeline_delete(const char* timelineName) 
{
    if(!connected) {
        LOG_WARN(("Client not connected to ZooKeeper"));

        return;
    }
    
    char* path = make_path(2, "/timelines/" , timelineName);
    printf("Deleting Timeline %s\n", timelineName);
    delete_znode(path);
}

/*********************************************************************************/
/*
 * Topic Handling Logic
 */


/* Re-try to create a topic node */
void create_topic(const char * path, const char * value);

/*
 * Completion function for creating topic nodes.
 */
void topic_create_completion (int rc, const char * value, const void * data) 
{
    switch (rc) {
        case ZCONNECTIONLOSS:
            create_topic(value, (const char *) data);
            
            break;
        case ZOK:
            {
                LOG_INFO(("Created topic node", value));
                char* pub_path = make_path(2, value, "/publishers");
       			char* sub_path = make_path(2, value, "/subscribers");
                create_parent(pub_path, "");
                create_parent(sub_path, "");
                free(pub_path);
                free(sub_path);
            }
            break;
        case ZNODEEXISTS:
            LOG_WARN(("Node already exists"));
            
            break;
        default:
	  		LOG_ERROR(("Something went wrong when creating the topic: %s, %s", value, rc2string(rc)));
            
            break;
    }
}

/* Re-try to create a topic node */
void create_topic(const char * path, const char * value) 
{
    zoo_acreate(zh,
                path,
                value,
                0,
                &ZOO_OPEN_ACL_UNSAFE,
                0,
                topic_create_completion,
                NULL);   
}

/* Remove a new topic to a timeline (synchronous call)
   Note: Will fail if child node exist (this is desirable)*/
int zk_topic_create(const char* timelineName, const char* topicName) 
{
    if(!connected) {
        LOG_WARN(("Client not connected to ZooKeeper"));

        return ZCONNECTIONLOSS;
    }
    
   	char* path = make_path(4, "/timelines/" , timelineName, "/", topicName);
    printf("Registering Topic %s on Timeline %s\n", timelineName, topicName);
    
    char* tmp_string;
    int rc = ZNONODE;

    /* Wait until the node creation occurs */
    while (rc == ZNONODE)
    {
       rc = zoo_create(zh, 
                       path,
                       tmp_string,
                       0, 
                       &ZOO_OPEN_ACL_UNSAFE, 
                       0,
                       NULL, 
                       0); 
    }
    /* Possible Return Codes
     * ZOK operation completed successfully
     * ZNONODE the parent node does not exist.
     * ZNODEEXISTS the node already exists
     * ZNOAUTH the client does not have permission.
     */

    char* pub_path = make_path(5, "/timelines/" , timelineName, "/", topicName, "/publishers");
    char* sub_path = make_path(5, "/timelines/" , timelineName, "/", topicName, "/subscribers");
    rc = create_parent_sync(pub_path, "");
    rc = create_parent_sync(sub_path, "");
    free(pub_path);
    free(sub_path);

    free(path);
    return rc;
}

/* Add a new topic to a timeline (synchronous call)*/
void zk_topic_delete(const char* timelineName, const char* topicName) 
{
    if(!connected) {
        LOG_WARN(("Client not connected to ZooKeeper"));
        return;
    }

    char* pub_path = make_path(5, "/timelines/" , timelineName, "/", topicName, "/publishers");
    char* sub_path = make_path(5, "/timelines/" , timelineName, "/", topicName, "/subscribers");
    delete_znode(pub_path);
    delete_znode(sub_path);
    free(pub_path);
    free(sub_path);

    sleep(2);
    
    char* path = make_path(4, "/timelines/" , timelineName, "/", topicName);
    printf("Deleting zk node Topic %s on Timeline %s\n", timelineName, topicName);
    
    delete_znode(path);
    free(path);
    return;
}

/*********************************************************************************/
/*
 *  Logic to listen for relevant subscribers to a topic
 */

/* Function to get subscribers to the topic */
void get_subscribers(const char * path);

/* Completion function looking for subscribers, to match with publishers */
void get_subscribers_completion (int rc,
                         const struct String_vector *strings,
                         const void *data) 
{
    switch (rc) {
        case ZCONNECTIONLOSS:
        case ZOPERATIONTIMEOUT:
            get_subscribers((char*) data);
            
            break;
        case ZOK:
            if(strings->count == 1) {
                LOG_DEBUG(("Got %d subscriber", strings->count));
            } else {
                LOG_DEBUG(("Got %d subscribers", strings->count));
            }

            // struct String_vector *tmp_workers = removed_and_set(strings, &workers);
            // free_vector(tmp_workers);
            //get_tasks();

            break;
        default:
            LOG_ERROR(("Something went wrong when checking subscribers: %s", rc2string(rc)));
            
            break;
    }
}

/* Subscriber Watch return function */
void subscribers_watcher (zhandle_t *zh, int type, int state, const char *path,void *watcherCtx) {
    if( type == ZOO_CHILD_EVENT) {
        get_subscribers(path);
    } else {
        LOG_DEBUG(("Watched event: ", type2string(type)));
    }
}

/* Pre-process the path for the get_subscribers() function */
char* process_sub_topic_string(const char* path)
{
	char* new_path = (char*) malloc((strlen(path)+1)*sizeof(char));
	char* replace_ptr;

	// Find "/publishers/" string
	replace_ptr = (char*) strstr(path, "/publishers/");
	if (replace_ptr != NULL)
	{
		size_t len = replace_ptr - path;
		strncpy(new_path, path, len);
		new_path[len] = '\0';
		strcat(new_path, "/subscribers");
		return new_path;
	}
	strcpy(new_path, path);
	return new_path;
}

/* Function to get subscribers to the topic */
void get_subscribers(const char * path){
	char *new_path = process_sub_topic_string(path);
    zoo_awget_children(zh,
                       new_path,
                       subscribers_watcher,
                       NULL,
                       get_subscribers_completion,
                       (void*) new_path);
    free(new_path);
}

/*********************************************************************************/
/*
 * Logic to handle publisher creation
 */

/* Re-try to create a publisher node */
void create_publisher(const char * path, const char * value);

/*
 * Completion function for creating publishers/subscribers
 */
void publisher_create_completion (int rc, const char * value, const void * data) {
    switch (rc) {
        case ZCONNECTIONLOSS:
            create_publisher(value, (const char *) data);
            
            break;
        case ZOK:
            LOG_INFO(("Created publisher node", value));
            // Setup a poll for subscribers for the same topic
            get_subscribers(value);

            break;
        case ZNODEEXISTS:
            LOG_WARN(("Node already exists"));
            
            break;
        default:
	  		LOG_ERROR(("Something went wrong when creating the topic: %s, %s", value, rc2string(rc)));
            
            break;
    }
}

/* Re-try to create a publisher node */
void create_publisher(const char* path, const char* value) {    
    zoo_acreate(zh,
                path,
                value,
                strlen(value) + 1,
                &ZOO_OPEN_ACL_UNSAFE,
                ZOO_EPHEMERAL,
                publisher_create_completion,
                NULL);    
}

/* Add a new publisher to a topic*/
void zk_publisher_create(const char* timelineName, const char* topicName, const char* publisherName) {
    if(!connected) {
        LOG_WARN(("Client not connected to ZooKeeper"));

        return;
    }
    
   	char* path = make_path(6, "/timelines/" , timelineName, "/", topicName, "/publishers/", publisherName);
    printf("Registering the existense of a Publisher on Topic %s on Timeline %s\n", timelineName, topicName);

    zoo_acreate(zh,
                path,
                brokerGroup,
                strlen(brokerGroup) + 1,
                &ZOO_OPEN_ACL_UNSAFE,
                ZOO_EPHEMERAL,
                publisher_create_completion,
                NULL);
    free(path);
}

/* Delete a new publisher from a topic*/
void zk_publisher_delete(const char* timelineName, const char* topicName, const char* publisherName) {
    if(!connected) {
        LOG_WARN(("Client not connected to ZooKeeper"));

        return;
    }
    
    char* path = make_path(6, "/timelines/" , timelineName, "/", topicName, "/publishers/", publisherName);
    printf("Deleting znode of a Publisher on Topic %s on Timeline %s\n", timelineName, topicName);

    sync_delete_znode(path);
    free(path);
}

/*********************************************************************************/
/*
 * Logic to listen to relevant publishers to a topic
 */

/* Function to get publishers to the topic */
void get_publishers(const char * path);

/* Completion function looking for publishers, to match with subscribers */
void get_publishers_completion (int rc,
                         const struct String_vector *strings,
                         const void *data) {
    switch (rc) {
        case ZCONNECTIONLOSS:
        case ZOPERATIONTIMEOUT:
            get_publishers((char*) data);
            
            break;
        case ZOK:
            if(strings->count == 1) {
                LOG_DEBUG(("Got %d publisher", strings->count));
            } else {
                LOG_DEBUG(("Got %d publishers", strings->count));
            }

            // struct String_vector *tmp_workers = removed_and_set(strings, &workers);
            // free_vector(tmp_workers);
            //get_tasks();

            break;
        default:
            LOG_ERROR(("Something went wrong when checking publishers: %s", rc2string(rc)));
            
            break;
    }
}

/* Publisher Watch return function */
void publishers_watcher (zhandle_t *zh, int type, int state, const char *path,void *watcherCtx) {
    if( type == ZOO_CHILD_EVENT) {
        get_publishers(path);
    } else {
        LOG_DEBUG(("Watched event: ", type2string(type)));
    }
}

/* Pre-process the path for the get_publishers() function */
char* process_pub_topic_string(const char* path)
{
	char* new_path = (char*) malloc((strlen(path)+1)*sizeof(char));
	char* replace_ptr;

	// Find "/subscribers/" string
	replace_ptr = (char*) strstr(path, "/subscribers/");
	if (replace_ptr != NULL)
	{
		size_t len = replace_ptr - path;
		strncpy(new_path, path, len);
		new_path[len] = '\0';
		strcat(new_path, "/publishers");
		return new_path;
	}
	strcpy(new_path, path);
	return new_path;
}

/* Function to get publishers to the topic */
void get_publishers(const char * path){
	char *new_path = process_pub_topic_string(path);
    zoo_awget_children(zh,
                       new_path,
                       publishers_watcher,
                       NULL,
                       get_publishers_completion,
                       (void*) new_path);
    free(new_path);
}

/*********************************************************************************/
/*
 * Routines to handle subscriber creation
 */

/* Re-try to create a subscriber node */
void create_subscriber(const char * path, const char * value);

/*
 * Completion function for creating publishers/subscribers
 */
void subscriber_create_completion (int rc, const char * value, const void * data) {
    switch (rc) {
        case ZCONNECTIONLOSS:
            create_subscriber(value, (const char *) data);
            
            break;
        case ZOK:
            LOG_INFO(("Created subscriber node", value));
            // Setup a poll for publishers for the same topic
            get_publishers(value);

            break;
        case ZNODEEXISTS:
            LOG_WARN(("Node already exists"));
            
            break;
        default:
	  		LOG_ERROR(("Something went wrong when creating the topic: %s, %s", value, rc2string(rc)));
            
            break;
    }
}

/* Re-try to create a subscriber node */
void create_subscriber(const char * path, const char * value) {    
    zoo_acreate(zh,
                path,
                value,
                strlen(value) + 1,
                &ZOO_OPEN_ACL_UNSAFE,
                ZOO_EPHEMERAL,
                subscriber_create_completion,
                NULL);    
}

/* Add a new subscriber to a topic*/
void zk_subscriber_create(const char* timelineName, const char* topicName, const char* subscriberName) {
    if(!connected) {
        LOG_WARN(("Client not connected to ZooKeeper"));

        return;
    }
    
   	char* path = make_path(6, "/timelines/" , timelineName, "/", topicName, "/subscribers/", subscriberName);
    printf("Registering the existense of a Subscriber on Topic %s on Timeline %s\n", timelineName, topicName);

    zoo_acreate(zh,
                path,
                brokerGroup,
                strlen(brokerGroup) + 1,
                &ZOO_OPEN_ACL_UNSAFE,
                ZOO_EPHEMERAL,
                subscriber_create_completion,
                NULL);
    free(path);
}

/* Delete a subscriber from a topic*/
void zk_subscriber_delete(const char* timelineName, const char* topicName, const char* subscriberName) {
    if(!connected) {
        LOG_WARN(("Client not connected to ZooKeeper"));

        return;
    }
    
    char* path = make_path(6, "/timelines/" , timelineName, "/", topicName, "/subscribers/", subscriberName);
    printf("Deleting the znode of a Subscriber on Topic %s on Timeline %s\n", timelineName, topicName);

    sync_delete_znode(path);
    free(path);
}

/*********************************************************************************/

/* Bootstrapping function, create parent nodes, if not already created */
void bootstrap() {
    if(!connected) {
      LOG_WARN(("Client not connected to ZooKeeper"));
        return;
    }
    
    create_parent("/timelines", "");
    create_parent("/brokers", "");

    // Initialize timeline vector
    timelines = (struct String_vector*) malloc(sizeof(struct String_vector));
    allocate_vector(timelines, 0);
}

/*********************************************************************************/
/* Initialize the Zookeeper connection -> takes in host:port pair to initiate connection*/
int init (char * hostPort) 
{
    srand(time(NULL));
    server_id  = rand();
    
    zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);
    
    zh = zookeeper_init(hostPort,
                        main_watcher,
                        15000,
                        0,
                        0,
                        0);
    
    return errno;
}

/*********************************************************************************/

/* Logic to initialize the Zookeeper Client Logic */
/* Requires argc as 3 and a char array of 3 arguments
   argv[1] = host:port -> comma separate list of host:port pairs of zookeeper ensemble
   argv[2] = lan_name  -> unique name of the lan which the edge broker is representing */
int init_zk_logic (int argc, char * argv[]) 
{
    LOG_DEBUG(("THREADED defined"));

    /* Get Broker Group Name from command line*/
    brokerGroup = argv[2];
    
    /*
     * Initialize ZooKeeper session
     */
    if(init(argv[1])){
        LOG_ERROR(("Error while initializing the master: ", errno));
    }
    
    /*
     * Wait until connected
     */
    while(!is_connected()) {
        sleep(1);
    }
    
    LOG_DEBUG(("Connected, going to bootstrap and run for master"));
    
    /*
     * Create parent znodes, if they don't already exist
     * if they already exist then will print a message
     */
    bootstrap();

    /* Register a Broker, and join the master edge broker election process 
       Note: each LAN can have multiple edge brokers in active/standby mode, 
       with one of them being the master */
    run_for_master();

    /* Sleep until the leadership role is obtained */
    while (leader != 1)
    {
        sleep(2);
    }

    // zk_timeline_create("test");
    // zk_topic_create("test", "blah");
    // zk_publisher_create("test", "blah","test-pub");
    // zk_subscriber_create("test", "blah","test-sub");
    
    // /*
    //  * Run until session expires
    //  */
    
    // while(!is_expired()) {
    //     sleep(1);
    // }    
    return 0; 
}

#ifdef __cplusplus
}
#endif

