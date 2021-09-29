#include <stdio.h> 		/* printf */
#include <stdlib.h>		/* free */
#include <unistd.h>             /* fgets */
#include <string.h>		/* memset */
#include <sys/socket.h>		/* socket */
#include <sys/epoll.h>          /* epoll */
#include <linux/if_packet.h>	/* AF_PACKET */


#define MAX_EVENTS 10
#define BUF_SIZE 1450
#include "common.h"

// store the physical address of the incoming request
static uint8_t dst_addr[6];

int receive_arp_request(int raw_sock,struct sockaddr_ll source);
int recv_raw_packet(int raw_sock);
int send_raw_pocket(int sd,struct sockaddr_ll source);

int main(int argc, char *argv[])
{
	struct    sockaddr_ll so_name;
	int       raw_sock;

	/* Set up a raw AF_PACKET socket without ethertype filtering */
     raw_sock = create_socket();

	/* Fill the fields of so_name with info from interface */
	get_mac_from_interface(&so_name);

    /* receive the ARP request and send the response */
    int rc = receive_arp_request(raw_sock, so_name);

}


int receive_arp_request(int raw_sock, struct  sockaddr_ll source){
    int rc;
    struct epoll_event ev, events[MAX_EVENTS];
    int epollfd;
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = raw_sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, raw_sock, &ev) == -1) {
        perror("epoll_ctl: raw_sock");
        exit(EXIT_FAILURE);
    }

    memset(buf, 0, sizeof(buf));
    while(1) {
        rc = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (rc == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
        if (events->data.fd == raw_sock) {

            // receive raw packet from the sender
            rc = recv_raw_packet(raw_sock);

            // send the ARP reponse in the raw packet
            rc = send_raw_pocket(raw_sock, source);
            if (rc < 1) {
                perror("send");
                return -1;
            }
            printf("\n ARP response sent... \n ");
            memset(buf, 0, BUF_SIZE);
            break;
        }
    }

    close(raw_sock);

    return 0;
}

int recv_raw_packet(int sd)
{
    struct sockaddr_ll dest_sockaddr;
    struct ether_frame  frame_hdr;
    struct msghdr       msg;
    struct iovec        msgvec[1];
    int 	  	    rc;

    /* construct the frame header */
    msgvec[0].iov_base = &frame_hdr;
    msgvec[0].iov_len  = sizeof(struct ether_frame);


    /* construct the message metadata struct */
    msg.msg_name    = &dest_sockaddr;
    msg.msg_namelen = sizeof(struct sockaddr_ll);
    msg.msg_iovlen  = 1;
    msg.msg_iov     = msgvec;

    rc = recvmsg(sd, &msg, 0);
    if (rc == -1) {
        perror("sendmsg");
        return -1;
    }

    printf("\n ARP requst from : ");
    for (int i = 0; i < 6; i++) {
      printf("%x", frame_hdr.src_addr[i]);
      if(i!=5)
            printf(":");
    }
    /*
    * Copy the src_addr from the received raw packet for future communication
    */
    for (int i = 0; i < 6; i++) {
        dst_addr[i] = dest_sockaddr.sll_addr[i];
    }

    return rc;
}


int send_raw_pocket(int sd, struct sockaddr_ll source_sockaddr)
{
    int    rc;
    struct ether_frame frame_hdr;
    struct msghdr      *msg;
    struct iovec       msgvec[1];

    /* Fill in Ethernet header */

    memcpy(frame_hdr.dst_addr, dst_addr, 6);
    memcpy(frame_hdr.src_addr, source_sockaddr.sll_addr, 6);
    /* Match the ethertype in packet_socket.c: */
  // frame_hdr.eth_proto[0] = frame_hdr.eth_proto[1] = 0xFF;
    frame_hdr.eth_proto = htons(ETH_PROTOCOL);
    /* Point to frame header */
    msgvec[0].iov_base = &frame_hdr;
    msgvec[0].iov_len  = sizeof(struct ether_frame);


    /* Allocate a zeroed-out message info struct */
    msg = (struct msghdr *)calloc(1, sizeof(struct msghdr));

    /* Fill out message metadata struct */
    memcpy(source_sockaddr.sll_addr, dst_addr, 6);
    msg->msg_name    = &source_sockaddr;
    msg->msg_namelen = sizeof(struct sockaddr_ll);
    msg->msg_iovlen  = 1;
    msg->msg_iov     = msgvec;

    /* Construct and send message */
    rc = sendmsg(sd, msg, 0);
    if (rc == -1) {
        perror("sendmsg");
        free(msg);
        return 1;
    }
    printf("\n ARP response sent to : ");
    for (int i = 0; i < 6; i++) {
        printf("%x", dst_addr[i]);
        if(i!=5)
            printf(":");
    }

    /* Remember that we allocated this on the heap; free it */
    free(msg);

    return rc;
}

