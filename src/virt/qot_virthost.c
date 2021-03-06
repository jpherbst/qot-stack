/**
 * @file qot_virthost.c
 * @brief Provides a socket interface for a guest OS to send timeline metadata to the host (using QEMU-KVM Virtserial Interface)
 * @author Sandeep D'souza
 * 
 * Copyright (c) Carnegie Mellon University, 2017. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *      1. Redistributions of source code must retain the above copyright notice, 
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
 *
 */

#include <stdio.h> 
#include <string.h>   //strlen 
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>   //close 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/un.h> 
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 
#include <fcntl.h>
#include <signal.h> // SIGINT
#include <poll.h>
#include <pthread.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>

// Virtualization Datatypes
#include "qot_virt.h"
    
#define TRUE   1 
#define FALSE  0 
#define SOCKET_PATH "/tmp/qot_virthost"

// Binding Count per timeline
timeline_virt_t vtimeline[MAX_TIMELINES];

static int running = 1;

static void exit_handler(int s)
{
    printf("Exit requested \n");
    running = 0;
}

int get_timeline_list_id(qot_timeline_t *timeline)
{
    int i;
    int retval = -1;
    for (i=0; i < MAX_TIMELINES; i++)
    {
        if (vtimeline[i].index == timeline->index)
        {
            retval = i;
            break;
        }
    }
    return retval;
}

int add_timeline_to_list(qot_timeline_t *timeline)
{
    int i;
    int retval;
    for (i=0; i < MAX_TIMELINES; i++)
    {
        if (vtimeline[i].index == -1)
        {
            vtimeline[i].index = timeline->index;
            vtimeline[i].bind_count = 0;
            vtimeline[i].running = 1;
            sprintf(vtimeline[i].filename, "/dev/timeline%d", timeline->index);
            retval = i;
            break;
        }
    }
    return retval;
}

void remove_timeline_from_list(qot_timeline_t *timeline)
{
    int i;
    for (i=0; i < MAX_TIMELINES; i++)
    {
        if (vtimeline[i].index == timeline->index)
        {
            vtimeline[i].index = -1;
            vtimeline[i].bind_count = 0;
            vtimeline[i].running = 0;
            break;
        }
    }
}

void increment_bind_count(qot_timeline_t *timeline)
{
    int i;
    for (i=0; i < MAX_TIMELINES; i++)
    {
        if (vtimeline[i].index == timeline->index)
        {
            vtimeline[i].bind_count++;
            break;
        }
    }  
}

void decrement_bind_count(qot_timeline_t *timeline)
{
    int i;
    for (i=0; i < MAX_TIMELINES; i++)
    {
        if (vtimeline[i].index == timeline->index)
        {
            vtimeline[i].bind_count--;
            if(vtimeline[i].bind_count < 0)
                printf("Something bad has happened\n");
            break;
        }
    }  
}

int get_bind_count(qot_timeline_t *timeline)
{
    int i;
    for (i=0; i < MAX_TIMELINES; i++)
    {
        if (vtimeline[i].index == timeline->index)
        {
            return vtimeline[i].bind_count;
        }
    }   
}

/* Thread function which writes timeline parameters to shared memory */
void *write_timeline_params(void *data)
{
    int i;
    int retval;
    int timeline_fd;
    int shm_fd;
    struct pollfd poll_tl[1];
    tl_clockparams_t *parameters;
    timeline_virt_t *tl_ptr = (timeline_virt_t*) data;
    void *shmem_ptr;

    printf("Thread: New thread spawned for timeline %d\n", tl_ptr->index);
    // Open timeline file descriptor (/dev/timelineX)
    timeline_fd = open(tl_ptr->filename, O_RDWR);
    if (!timeline_fd)
    {
        printf("Thread: Unable to open timeline file descriptor\n");
        return NULL;
    }

    // Open Shared Memory Location -> Created by ivshmem server
    shm_fd = shm_open("ivshmem", O_RDWR, S_IRWXU);
    if (shm_fd < 0) {
        printf("cannot open shm file %s\n", strerror(errno));
        printf("Thread: Unable to open shared memory region\n");
        goto err_close_timeline;
    }

    // Map memory to a pointer
    shmem_ptr = mmap(0, MAX_TIMELINES*sizeof(tl_clockparams_t), PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shmem_ptr == MAP_FAILED)
    {
        printf("Thread: Memory Mapping failed\n");
        goto err_close_shm;
    }

    // Increment pointer to correct location (based on timeline index)
    shmem_ptr = shmem_ptr + tl_ptr->index*(sizeof(tl_clockparams_t));

    // Typecast void pointer to tl_clockparams_t pointer
    parameters = (tl_clockparams_t*) shmem_ptr;

    // Set Initial Parameters
    ioctl(timeline_fd, TIMELINE_GET_PARAMETERS, &parameters->translation);

    // Polling data structures
    poll_tl[0].fd = timeline_fd;
    poll_tl[0].events = POLLIN;

    printf("Thread: New thread spawned polling %s\n", tl_ptr->filename);
    while(running == 1 && tl_ptr->running == 1)
    {
        // Poll the timeline character device
        retval = poll(poll_tl, 1, 1000);
        if (retval > 0)
        {
            if (poll_tl[0].revents & POLLIN)
            {
                ioctl(timeline_fd, TIMELINE_GET_PARAMETERS, &parameters->translation);
                // Write code to write new params to shared memory :TODO
                // parameters->translation.mult = 1;
                // parameters->translation.last = 1;
                // printf("New parameters are %lld %lld\n", parameters->translation.mult, parameters->translation.last);
            }
        }
    }
err_close_shm:
    close(shm_fd);
err_close_timeline:
    close(timeline_fd);
    printf("Thread for timeline%d terminating\n", tl_ptr->index);
    return NULL;
}
    
int main(int argc , char *argv[])  
{  
    int opt = TRUE;  
    int master_socket , addrlen , new_socket , client_socket[30] , 
          max_clients = 30 , activity, i , valread , sd;  
    int max_sd;  
    struct sockaddr_un address;

    int tl_id;  
        
    char buffer[1025];  //data buffer of 1K 

    // Timeline file name (/dev/timelineX)
    char qot_timeline_filename[15];
    int tl_count = 0;

    // File descriptor for /dev/qotusr
    int qotusr_fd;

    // File descriptor for /dev/timelineX
    int timeline_fd;
        
    //set of socket descriptors 
    fd_set readfds; 

    // timeval for select timeout
    struct timeval timeout = {5,0};

    //QoT Virtualization Message
    qot_virtmsg_t virtmsg; 

    // QoT Binding (qot_virtd)
    qot_binding_t binding;
    
    sprintf(binding.name,  "qot_virtd");

    /* Thread which will write timeline parameters to shared memory */
    pthread_t tl_shmem_thread[MAX_TIMELINES];

    // Open the /dev/qotusr file
    qotusr_fd = open("/dev/qotusr", O_RDWR);
    if (qotusr_fd < 0)
    {
        printf("Error: Invalid qotusr file\n");
        return QOT_RETURN_TYPE_ERR;
    }
    
    //initialise all client_socket[] to 0 so not checked 
    for (i = 0; i < max_clients; i++)  
    {  
        client_socket[i] = 0;  
    } 

    // Initialize all timeline virt structures
    for (i = 0; i < MAX_TIMELINES; i++)  
    {  
        vtimeline[i].index = -1;  
        vtimeline[i].bind_count = 0;  
    } 
        
    //create a master socket 
    if( (master_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == 0)  
    {  
        perror("socket failed");  
        exit(EXIT_FAILURE);  
    }  
    
    //set master socket to allow multiple connections , 
    //this is just a good habit, it will work without this 
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )  
    {  
        perror("setsockopt");  
        exit(EXIT_FAILURE);  
    }  
    
    //type of socket created 
    address.sun_family = AF_UNIX;  
    //socket path
    snprintf(address.sun_path, sizeof(address.sun_path), SOCKET_PATH);
        
    //bind the socket to localhost port 8888 
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)  
    {  
        perror("bind failed");  
        exit(EXIT_FAILURE);  
    }  
    printf("Listening for connections ...\n");  
        
    //try to specify maximum of 3 pending connections for the master socket 
    if (listen(master_socket, 3) < 0)  
    {  
        perror("listen");  
        exit(EXIT_FAILURE);  
    }  
        
    //accept the incoming connection 
    addrlen = sizeof(address);  
    puts("Waiting for connections ...");  

    // Install SIGINT Signal Handler for exit
    signal(SIGINT, exit_handler);
       
    while(running)  
    {  
        //clear the socket set 
        FD_ZERO(&readfds);  
    
        //add master socket to set 
        FD_SET(master_socket, &readfds);  
        max_sd = master_socket;  
            
        //add child sockets to set 
        for (i = 0; i < max_clients; i++)  
        {  
            //socket descriptor 
            sd = client_socket[i];  
                
            //if valid socket descriptor then add to read list 
            if(sd > 0)  
                FD_SET(sd, &readfds);
                
            //highest file descriptor number, need it for the select function 
            if(sd > max_sd)  
                max_sd = sd;
        }  
    
        // wait for an activity on one of the sockets, timeout is set to 5s (to enable program termination), 
        activity = select(max_sd + 1, &readfds, NULL, NULL, &timeout);  
        // Reset Timeout
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        if ((activity < 0) && (errno!=EINTR))  
        {  
            printf("select error");  
        }  
        else
        {
            //If something happened on the master socket , 
            //then its an incoming connection 
            if (FD_ISSET(master_socket, &readfds))  
            {  
                if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)  
                {  
                    perror("accept");  
                    exit(EXIT_FAILURE);  
                }  
                
                //inform user of socket number - used in send and receive commands 
                printf("New connection, socket fd is %d\n", new_socket);   
                    
                //add new socket to array of sockets 
                for (i = 0; i < max_clients; i++)  
                {  
                    //if position is empty 
                    if( client_socket[i] == 0 )  
                    {  
                        client_socket[i] = new_socket;  
                        printf("Adding to list of sockets as %d\n" , i);        
                        break;  
                    }  
                }  
            }  
                
            //else its some IO operation on some other socket
            for (i = 0; i < max_clients; i++)  
            {  
                sd = client_socket[i];  
                    
                if (FD_ISSET(sd, &readfds))  
                {  
                    //Check if it was for closing, and also read the incoming message 
                    if ((valread = read(sd, &virtmsg, sizeof(virtmsg))) == 0)  
                    {  
                        //Somebody disconnected , get his details and print 
                        getpeername(sd , (struct sockaddr*)&address , \
                            (socklen_t*)&addrlen);  
                        printf("Host disconnected fd is %d \n", sd); 
                            
                        //Close the socket and mark as 0 in list for reuse 
                        close(sd);  
                        client_socket[i] = 0;  
                    }   
                    // Parse the message and send data to kernel module
                    else
                    {  
                        // Try to create a new timeline if none exists
                        virtmsg.retval = QOT_RETURN_TYPE_OK;
                        printf("Message Received\n");
                        printf("Type           : %d\n", virtmsg.msgtype);
                        printf("Guest TL ID    : %d\n", virtmsg.info.index);
                        printf("Guest TL Name  : %s\n", virtmsg.info.name);
                        switch(virtmsg.msgtype)
                        {
                            case TIMELINE_CREATE:
                                // If timeline exists try to get information
                                if(ioctl(qotusr_fd, QOTUSR_GET_TIMELINE_INFO, &virtmsg.info) < 0)
                                {
                                    // Timeline does not exist -> Check if max timelines exceeded
                                    if(tl_count < MAX_TIMELINES)
                                    {
                                        // Try to create a new timeline
                                        if(ioctl(qotusr_fd, QOTUSR_CREATE_TIMELINE, &virtmsg.info) < 0)
                                        {
                                            virtmsg.retval = QOT_RETURN_TYPE_ERR;
                                        }
                                        else
                                        {
                                            // New timeline created
                                            if(tl_count < MAX_TIMELINES)
                                                tl_count++;
                                            tl_id = add_timeline_to_list(&virtmsg.info);
                                            increment_bind_count(&virtmsg.info);
                                            // Bind the qot_virt daemon to the new timeline
                                            sprintf(qot_timeline_filename, "/dev/timeline%d", virtmsg.info.index);
                                            timeline_fd = open(qot_timeline_filename, O_RDWR);
                                            if (!timeline_fd)
                                            {
                                                printf("Cant open /dev/timeline%d\n", virtmsg.info.index);
                                            }
                                            printf("Binding %s %s %d\n", binding.name, qot_timeline_filename, timeline_fd);
                                            binding.demand = virtmsg.demand;
                                            if(ioctl(timeline_fd, TIMELINE_BIND_JOIN, &binding) < 0)
                                            {
                                                virtmsg.retval = QOT_RETURN_TYPE_ERR;
                                            }
                                            close(timeline_fd);
                                            printf("Creating thread for timeline %d\n", tl_id);
                                            if(pthread_create(&tl_shmem_thread[tl_id], NULL, write_timeline_params, (void*)&vtimeline[tl_id])) 
                                            {
                                                printf("Error creating thread\n");
                                            }
                                            printf("Thread created for timeline %d\n", tl_id);
                                        }
                                    }
                                    else
                                    {
                                        virtmsg.retval = QOT_RETURN_TYPE_ERR;
                                    }
                                }
                                else
                                {
                                    increment_bind_count(&virtmsg.info);
                                }
                                break;
                            case TIMELINE_DESTROY:
                                // Check if timeline is valid
                                if(ioctl(qotusr_fd, QOTUSR_GET_TIMELINE_INFO, &virtmsg.info) < 0)
                                {
                                    virtmsg.retval = QOT_RETURN_TYPE_ERR;
                                }
                                else
                                {
                                    // Timeline is valid, check if all reference to it are done
                                    decrement_bind_count(&virtmsg.info);
                                    printf("Bind count is %d\n", get_bind_count(&virtmsg.info));
                                    if(get_bind_count(&virtmsg.info) == 0)
                                    {
                                        // If all bindings are gone, destroy the timeline
                                        tl_id = get_timeline_list_id(&virtmsg.info);
                                        printf("Cancelling thread for timeline %d\n", tl_id);
                                        vtimeline[tl_id].running = 0; // Set running to false to force thread to terminate gracefully
                                        if(pthread_join(tl_shmem_thread[tl_id], NULL)) 
                                        {
                                            printf("Error joining thread\n");
                                            return -1;
                                        }
                                        printf("Thread cancelled for timeline %d\n", tl_id);
                                        // unbind the qot_virt daemon to the new timeline
                                        sprintf(qot_timeline_filename, "/dev/timeline%d", virtmsg.info.index);
                                        timeline_fd = open(qot_timeline_filename, O_RDWR);
                                        if (!timeline_fd)
                                        {
                                            printf("Cant open /dev/timeline%d\n", virtmsg.info.index);
                                        }
                                        printf("Unbinding %s %s %d\n", binding.name, qot_timeline_filename, timeline_fd);
                                        binding.demand = virtmsg.demand;
                                        if(ioctl(timeline_fd, TIMELINE_BIND_LEAVE, &binding) < 0)
                                        {
                                            virtmsg.retval = QOT_RETURN_TYPE_ERR;
                                        }
                                        close(timeline_fd);
                                        if(ioctl(qotusr_fd, QOTUSR_DESTROY_TIMELINE, &virtmsg.info) < 0)
                                        {
                                            printf("Unable to destroy timeline %d\n", virtmsg.info.index);
                                            virtmsg.retval = QOT_RETURN_TYPE_ERR;
                                        }
                                        else
                                        {
                                            printf("File descriptor cancelled for timeline %d\n", tl_id);
                                            if(tl_count > 0)
                                                tl_count--;
                                            remove_timeline_from_list(&virtmsg.info);
                                        }
                                    }
                                }
                                break;
                            case TIMELINE_UPDATE:
                                // Check if timeline exists
                                if(ioctl(qotusr_fd, QOTUSR_GET_TIMELINE_INFO, &virtmsg.info) < 0)
                                {
                                    virtmsg.retval = QOT_RETURN_TYPE_ERR;
                                }
                                else
                                {
                                    // open timeline file and update QoT requirements
                                    sprintf(qot_timeline_filename, "/dev/timeline%d", virtmsg.info.index);
                                    timeline_fd = open(qot_timeline_filename, O_RDWR);
                                    if(timeline_fd < 0)
                                    {
                                        virtmsg.retval = QOT_RETURN_TYPE_ERR;
                                        break;
                                    }
                                    binding.demand = virtmsg.demand;
                                    if(ioctl(timeline_fd, TIMELINE_BIND_UPDATE, &binding) < 0)
                                    {
                                        virtmsg.retval = QOT_RETURN_TYPE_ERR;
                                    }
                                    close(timeline_fd);
                                }
                                break;
                            default:
                                virtmsg.retval = QOT_RETURN_TYPE_ERR;
                                break;
                        }
                        // Send Populated message struct back to the user
                        printf("Generated Reply\n");
                        printf("Type          : %d\n", virtmsg.msgtype);
                        printf("Host TL ID    : %d\n", virtmsg.info.index);
                        printf("Host TL Name  : %s\n", virtmsg.info.name);
                        printf("Retval        : %d\n", virtmsg.retval);
                        send(sd, &virtmsg , sizeof(virtmsg), 0); 
                    }  
                }  
            }
        }  
    }  

    /* wait for the tl_shmem_thread to finish */
    printf("Waiting for all threads to join...\n");
    for (i-0; i < MAX_TIMELINES; i++)
    {
        if(pthread_join(tl_shmem_thread[i], NULL)) 
        {
            printf("Error joining thread\n");
            return -1;
        }
    }
    close(qotusr_fd);  
    unlink(SOCKET_PATH);
    return 0;  
}