/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define BUFSIZE 1024

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    int multicast_flag = 0;

    /* check command line arguments */
    if (argc != 4) {
       fprintf(stderr,"usage: %s <hostname>/<multicast_ip> <port> <multicast_flag>\n", argv[0]);
       printf("Multicast address should be valid between 224.0.0.0 and 239.255.255.255");
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);
    multicast_flag = atoi(argv[3]);


    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) 
        error("ERROR opening socket");

    if(multicast_flag == 0)
    {
        /* gethostbyname: get the server's DNS entry */
        server = gethostbyname(hostname);
        if (server == NULL) {
            fprintf(stderr,"ERROR, no such host as %s\n", hostname);
            exit(0);
        }
         /* build the server's Internet address */
        bzero((char *) &serveraddr, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, 
          (char *)&serveraddr.sin_addr.s_addr, server->h_length);
        serveraddr.sin_port = htons(portno);
    }
    else
    {
        /* set up destination address */
        memset(&serveraddr,0,sizeof(serveraddr));
        serveraddr.sin_family=AF_INET;
        serveraddr.sin_addr.s_addr=inet_addr(hostname);
        serveraddr.sin_port=htons(portno); 
    }

    /* set a msg to send */
    bzero(buf, BUFSIZE);
    sprintf(buf, "QoT Rocks !");

    /* send the message to the server infintely periodically*/
    while(1)
    {
        serverlen = sizeof(serveraddr);
        n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
        if (n < 0) 
          error("ERROR in sendto");
        
        /* Wait for server to reply if not in multicast mode*/
        if(multicast_flag == 0)
        {
            /* print the server's reply */
            n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
            if (n < 0) 
              error("ERROR in recvfrom");
            printf("Echo from server: %s", buf);
        }
        sleep(5);
    }
    return 0;
}
