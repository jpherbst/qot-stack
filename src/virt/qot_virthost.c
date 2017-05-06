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

// Virtualization Datatypes
#include "qot_virt.h"
    
#define TRUE   1 
#define FALSE  0 
#define MAX_TIMELINES 10

// Binding Count per timeline
timeline_virt_t vtimeline[MAX_TIMELINES];

void add_timeline_to_list(qot_timeline_t *timeline)
{
    int i;
    for (i=0; i < MAX_TIMELINES; i++)
    {
        if (vtimeline[i].index == -1)
        {
            vtimeline[i].index = timeline->index;
            vtimeline[i].bind_count = 0;
            break;
        }
    }
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
    
int main(int argc , char *argv[])  
{  
    int opt = TRUE;  
    int master_socket , addrlen , new_socket , client_socket[30] , 
          max_clients = 30 , activity, i , valread , sd;  
    int max_sd;  
    struct sockaddr_un address;  
        
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

    //QoT Virtualization Message
    qot_virtmsg_t virtmsg; 

    // QoT Binding (qot_virtd)
    qot_binding_t binding;
    
    sprintf(binding.name, "qot_virtd");
        
    //a message 
    char *message = "Quartz-V Host Daemon v1.0 \r\n";  

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
    snprintf(address.sun_path, sizeof(address.sun_path), "/tmp/qot_virthost");
        
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
        
    while(TRUE)  
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
                FD_SET( sd , &readfds);  
                
            //highest file descriptor number, need it for the select function 
            if(sd > max_sd)  
                max_sd = sd;  
        }  
    
        //wait for an activity on one of the sockets, timeout is NULL, 
        //so wait indefinitely 
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);  
      
        if ((activity < 0) && (errno!=EINTR))  
        {  
            printf("select error");  
        }  
            
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
            printf("New connection , socket fd is %d\n", new_socket);  
          
            //send new connection greeting message 
            if(send(new_socket, message, strlen(message), 0) != strlen(message))  
            {  
                perror("send");  
            }  
                
            puts("Welcome message sent successfully");  
                
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
                    switch(virtmsg.msgtype)
                    {
                        virtmsg.retval = QOT_RETURN_TYPE_OK;
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
                                        add_timeline_to_list(&virtmsg.info);
                                        increment_bind_count(&virtmsg.info);
                                        // Bind the qot_virt daemon to the new timeline
                                        sprintf(qot_timeline_filename, "/dev/timeline%d", virtmsg.info.index);
                                        timeline_fd = open(qot_timeline_filename, O_RDWR);
                                        binding.demand = virtmsg.demand;
                                        if(ioctl(timeline_fd, TIMELINE_BIND_JOIN, &binding) < 0)
                                        {
                                            virtmsg.retval = QOT_RETURN_TYPE_ERR;
                                        }
                                        close(timeline_fd);
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
                                if(get_bind_count(&virtmsg.info) == 0)
                                {
                                    // If all bindings are gone, destroy the timeline
                                    if(ioctl(qotusr_fd, QOTUSR_DESTROY_TIMELINE, &virtmsg.info) < 0)
                                    {
                                        virtmsg.retval = QOT_RETURN_TYPE_ERR;
                                    }
                                    else
                                    {
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
                    send(sd, &virtmsg , sizeof(virtmsg), 0); 
                }  
            }  
        }  
    }  
    close(qotusr_fd);   
    return 0;  
}