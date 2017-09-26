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

// Include the QoT API
#include "../../api/c/qot.h"

// Basic onfiguration
#define TIMELINE_UUID    "my_test_timeline"
#define APPLICATION_NAME "udpserver"

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

static int running = 1;

/* Exit Handler on Ctrl+C */
static void exit_handler(int s)
{
  printf("Exit requested \n");
  running = 0;
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
  int filewrite_flag = 0; /* flag whether data should be written to file */
  char* timestamp_file;  /* timestamping file */
  FILE* ts_fd;             /* file descriptor for timestamp file */
  int64_t offset = 0;    /* timestamp offset */

  // Timeline-related Variable declaration
  timeline_t *my_timeline;
  qot_return_t retval;
  utimepoint_t nowtl;
  stimepoint_t tl_stp;
  tl_translation_t params;
  char qot_timeline_filename[15];
  int timeline_fd;


  // Timeline Name
  const char *u = TIMELINE_UUID;
 
  // Application Name
  const char *m = APPLICATION_NAME;

  timelength_t resolution = { .sec = 0, .asec = 1e9 }; // 1nsec
  timeinterval_t accuracy = { .below.sec = 0, .below.asec = 1e15, .above.sec = 0, .above.asec = 1e15 }; // 100usec

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
     if(argc < 6)
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

  /* Check for a filename to write timestamps to */
  if (argc >= 7)
  {
    filewrite_flag = 1;
    timestamp_file = argv[6];
    printf("Writing timestamp data to file %s\n", timestamp_file);
    ts_fd = fopen(timestamp_file, "w");
    if(ts_fd < 0)
    {
      printf("Unable to open file, terminating ...\n");
      exit(1);
    }
    fprintf(ts_fd, "Message\t\tSW Timestamp\t\tTL Timestamp\t\tBelowQoT\t\tAboveQoT\n");
  }

  /* Check if the timestamp needs to have an offset */
  if (argc >= 8)
  {
    //offset = (int64_t) atoi(argv[7]);
    sscanf(argv[7], "%lld", &offset);
    printf("Chosen an offset of %lld\n", offset);
  }

  // Initialize the timeline object
  my_timeline = timeline_t_create();
  if(!my_timeline)
  {
    printf("Unable to create the timeline_t data structure\n");
    return QOT_RETURN_TYPE_ERR;
  }

  // Bind to a timeline
  printf("Binding to timeline %s ........\n", u);
  if(timeline_bind(my_timeline, u, m, resolution, accuracy))
  {
    printf("Failed to bind to timeline %s\n", u);
    timeline_t_destroy(my_timeline);
    return QOT_RETURN_TYPE_ERR;
  }

  // Register exit signal handler
  signal(SIGINT, exit_handler);

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
  while (running) {

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
           timeline_gettime(my_timeline, &nowtl);
           printf("HW TIMESTAMP        %ld.%09ld\n", (long)ts[2].tv_sec + (offset/1000000000), (long)ts[2].tv_nsec + (offset%1000000000));
           printf("HWX TIMESTAMP       %ld.%09ld\n", (long)ts[1].tv_sec + (offset/1000000000), (long)ts[1].tv_nsec + (offset%1000000000));
           printf("SW TIMESTAMP        %ld.%09ld\n", (long)ts[0].tv_sec + (offset/1000000000), (long)ts[0].tv_nsec + (offset%1000000000));
           printf("CLOCK_MONOTONIC     %ld.%09ld\n", (long)now.tv_sec, (long)now.tv_nsec);
           printf("CLOCK_REALTIME      %ld.%09ld\n", (long)nowreal.tv_sec, (long)nowreal.tv_nsec);
           printf("TIMELINE_TIME       %lld.%18llu\n", nowtl.estimate.sec, nowtl.estimate.asec);
           printf("Uncertainity below  %llu.%18llu\n", nowtl.interval.below.sec, nowtl.interval.below.asec);
           printf("Uncertainity above  %llu.%18llu\n", nowtl.interval.above.sec, nowtl.interval.above.asec);
           tl_stp.estimate.sec = (int64_t)ts[0].tv_sec;
           tl_stp.estimate.asec = ((uint64_t)ts[0].tv_nsec)*nSEC_PER_SEC;
           timeline_core2rem(my_timeline, &tl_stp);
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

    if(filewrite_flag)
    {
      /* Write message and timestamps to file*/
      fprintf(ts_fd, "%s\t%ld.%09ld\t%lld.%18llu\t%llu.%18llu\t%llu.%18llu\n", buf, (long)ts[0].tv_sec + (offset/1000000000), (long)ts[0].tv_nsec + (offset%1000000000), tl_stp.estimate.sec, tl_stp.estimate.asec, nowtl.interval.below.sec, nowtl.interval.above.asec, nowtl.interval.below.sec, nowtl.interval.above.asec);
      fflush(ts_fd);
    }
  }

  /* Close Timestamp file */
  if (filewrite_flag)
  {
    fclose(ts_fd);
  }

err:
  // Unbind from timeline
  if(timeline_unbind(my_timeline))
  {
    printf("Failed to unbind from timeline %s\n", u);
    timeline_t_destroy(my_timeline);
    return QOT_RETURN_TYPE_ERR;
  }
  
  printf("Unbound from timeline %s\n", u);

  // Free the timeline data structure
  timeline_t_destroy(my_timeline);
    
}
