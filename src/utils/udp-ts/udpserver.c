/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/net_tstamp.h>
#include <sys/uio.h>
#include <time.h> /* for clock_gettime */
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <net/if.h>
#include <sys/ioctl.h> 

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

/**
 * Try to enable kernel timestamping, otherwise fall back to software.
 *
 * Run setsockopt SO_TIMESTAMPING with SOFTWARE on socket 'sock'. 
 * 
 * \param[in] sock  The socket to activate SO_TIMESTAMPING on (the UDP)
 * \warning         Requires CONFIG_NETWORK_PHY_TIMESTAMPING Linux option
 * \warning         Requires drivers fixed with skb_tx_timestamp() 
 */
void tstamp_mode_kernel(int sock) {
  int f = 0; /* flags to setsockopt for socket request */
  socklen_t slen;
  
  slen = (socklen_t)sizeof f;
  f |= SOF_TIMESTAMPING_RX_SOFTWARE;
  f |= SOF_TIMESTAMPING_SOFTWARE;
  if (setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPING, &f, slen) < 0) {
    printf("SO_TIMESTAMPING not possible\n");
    return;
  }
  printf("Using kernel timestamps\n");
}

/**
 * Try to enable hardware timestamping, otherwise fall back to kernel.
 *
 * Run ioctl() on 'iface' (supports Intel 82580 right now) and run
 * setsockopt SO_TIMESTAMPING with RAW_HARDWARE on socket 'sock'. 
 * 
 * \param[in] sock  The socket to activate SO_TIMESTAMPING on (the UDP)
 * \param[in] iface The interface name to ioctl SIOCSHWTSTAMP on
 * \warning         Requires CONFIG_NETWORK_PHY_TIMESTAMPING Linux option
 * \warning         Supports only Intel 82580 right now
 * \bug             Intel 82580 has bugs such as IPv4 only, and RX issues
 */
void tstamp_mode_hardware(int sock, char *iface) {
  struct ifreq dev; /* request to ioctl */
  struct hwtstamp_config hwcfg, req; /* hw tstamp cfg to ioctl req */
  int f = 0; /* flags to setsockopt for socket request */
  socklen_t slen;
  
  slen = (socklen_t)sizeof f;
  /* STEP 1: ENABLE HW TIMESTAMP ON IFACE IN IOCTL */
  memset(&dev, 0, sizeof dev);
  /*@ -mayaliasunique Trust me, iface and dev doesn't share storage */
  strncpy(dev.ifr_name, iface, sizeof dev.ifr_name);
  /*@ +mayaliasunique */
  /*@ -immediatetrans Yes, we might overwrite ifr_data */
  dev.ifr_data = (void *)&hwcfg;
  memset(&hwcfg, 0, sizeof hwcfg); 
  /*@ +immediatetrans */
  /* enable tx hw tstamp, ptp style, intel 82580 limit */
  hwcfg.tx_type = HWTSTAMP_TX_ON; 
  /* enable rx hw tstamp, all packets, yey! */
  hwcfg.rx_filter = HWTSTAMP_FILTER_ALL;
  req = hwcfg;
    /* Check that one is root */
  if (getuid() != 0)
    printf("Hardware timestamps requires root privileges\n");
  /* apply by sending to ioctl */
  if (ioctl(sock, SIOCSHWTSTAMP, &dev) < 0) {
    printf("ioctl: SIOCSHWTSTAMP: error\n");
    printf("Verify that %s supports hardware timestamp\n",iface);
    /* otherwise, try kernel timestamps (socket only) */ 
    printf("Falling back to kernel timestamps\n");
    tstamp_mode_kernel(sock);
    return;
  }

  if (memcmp(&hwcfg, &req, sizeof(hwcfg))) {
    printf("driver changed our HWTSTAMP options\n");
    printf("tx_type   %d not %d\n", hwcfg.tx_type, req.tx_type);
    printf("rx_filter %d not %d\n", hwcfg.rx_filter, req.rx_filter);
    return;
  }
  /* STEP 2: ENABLE NANOSEC TIMESTAMPING ON SOCKET */
  f |= SOF_TIMESTAMPING_TX_SOFTWARE;
  f |= SOF_TIMESTAMPING_TX_HARDWARE;
  f |= SOF_TIMESTAMPING_RX_SOFTWARE;
  f |= SOF_TIMESTAMPING_RX_HARDWARE;
  f |= SOF_TIMESTAMPING_RAW_HARDWARE;
  if (setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPING, &f, slen) < 0) {
    /* bail to userland timestamps (socket only) */ 
    printf("SO_TIMESTAMPING: error\n");
    return;
  }
  printf("Using hardware timestamps\n");
}



int main(int argc, char **argv) {
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */
  char cmsgbuf[BUFSIZE]; /* ancillary info buf */
  char *iface; /* hadrware interface to receive packets on */
  int multicast_recvflag = 0; /* multicast receive flag */
  char* multicast_recvaddr; /* multicast receive address */
  struct ip_mreq mreq; /* multicast recv request */
  char* multicast_bindip; /* IP address of interface on which to receive multicast */

  /* 
   * check command line arguments 
   */
  if (argc < 4) {
    fprintf(stderr, "usage: %s <port> <iface> <multicast_recvflag>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);
  iface = argv[2];
  multicast_recvflag = atoi(argv[3]);
  if(multicast_recvflag)
  {
     if(argc != 6)
     {
        fprintf(stderr, "usage: %s <port> <iface> <multicast_recvflag> <multicast_recvaddr> <multicast_bindip>\n", argv[0]);
        printf("Multicast address should be valid between 224.0.0.0 and 239.255.255.255");
        printf("Multicast Bind IP address should be a valid IP of an interface\n");
        exit(1);
     }
     else
     {
        multicast_recvaddr = argv[4];
        multicast_bindip = argv[5];
     }

  }

  /* 
   * socket: create the parent socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sockfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));
	
  /* Configure timestamping */
  tstamp_mode_hardware(sockfd, iface);

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  if(multicast_recvflag)
  {
     /* use setsockopt() to request that the kernel join a multicast group */
    bzero((char *) &mreq, sizeof(mreq));
     mreq.imr_multiaddr.s_addr=inet_addr(multicast_recvaddr);
     mreq.imr_interface.s_addr=inet_addr(multicast_bindip);//htonl(INADDR_ANY);
     if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt");
        exit(1);
     }
  }
  
  /* Setp message header */
  clientlen = sizeof(clientaddr);
  struct msghdr msg;
  struct iovec iov[1];
  iov[0].iov_base=buf;
  iov[0].iov_len=sizeof(buf);

  msg.msg_name=&clientaddr;
  msg.msg_namelen=sizeof(clientaddr);
  msg.msg_iov=iov;
  msg.msg_iovlen=1;
  msg.msg_control=(caddr_t)cmsgbuf;
  msg.msg_controllen=sizeof(cmsgbuf);
  /* 
   * main loop: wait for a datagram, then echo it
   */
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);
    bzero(cmsgbuf, BUFSIZE);
    n = recvmsg(sockfd, &msg, 0);
    if (n < 0)
      printf("ERROR in recvmsg %d\n", n);
    else
      printf("received packet\n");

    /* 
     * gethostbyaddr: determine who sent the datagram
     */
    printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    
    /* Get Packet Timestamp */
    int level, type;
    struct cmsghdr *cm;
    struct timespec *ts = NULL;
    for (cm = CMSG_FIRSTHDR(&msg); cm != NULL; cm = CMSG_NXTHDR(&msg, cm))
    {
        level = cm->cmsg_level;
        type  = cm->cmsg_type;
        if (SOL_SOCKET == level && SO_TIMESTAMPING == type) {
           ts = (struct timespec *) CMSG_DATA(cm);
           struct timespec now, nowreal;
           clock_gettime(CLOCK_MONOTONIC, &now);
           clock_gettime(CLOCK_REALTIME, &nowreal);
           printf("HW TIMESTAMP    %ld.%09ld\n", (long)ts[2].tv_sec, (long)ts[2].tv_nsec);
           printf("HWX TIMESTAMP   %ld.%09ld\n", (long)ts[1].tv_sec, (long)ts[1].tv_nsec);
           printf("SW TIMESTAMP    %ld.%09ld\n", (long)ts[0].tv_sec, (long)ts[0].tv_nsec);
           printf("CLOCK_MONOTONIC %ld.%09ld\n", (long)now.tv_sec, (long)now.tv_nsec);
           printf("CLOCK_REALTIME  %ld.%09ld\n", (long)nowreal.tv_sec, (long)nowreal.tv_nsec);

        }
    }
    
    if(multicast_recvflag == 0)
    {
        /* 
         * sendto: echo the input back to the client 
         */
        n = sendto(sockfd, buf, strlen(buf), 0, 
             (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) 
          error("ERROR in sendto");
    }
  }
    
}
