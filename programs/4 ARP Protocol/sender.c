#include <stdlib.h>		/* free */
#include <stdio.h> 		/* printf */
#include <unistd.h>             /* fgets */
#include <string.h>		/* memset */
#include <sys/socket.h>		/* socket */
#include <fcntl.h>
#include <sys/epoll.h>          /* epoll */
#include <linux/if_packet.h>	/* AF_PACKET */

#include "common.h"

#define MAX_EVENTS 10
#define BUF_SIZE 1450
#define DST_MAC_ADDR {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}
#define ETH_BROADCAST_ADDR {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}



int send_raw_packet(int raw_sock, struct sockaddr_ll so_name);
int receive_arp_response(int raw_sock, struct sockaddr_ll dst_addr);
int recv_raw_packet(int raw_sock, struct sockaddr_ll dst_addr);

int main(int argc, char *argv[])
{
	int     raw_sock;
	struct  sockaddr_ll so_name;

	/* Step 1: Set up Raw socket */
    raw_sock = create_socket();

    /* Step 2: address info of the last interface enumerated is stored in so_name*/
	get_mac_from_interface(&so_name);

    /* Step 3: send messages to the receiver */
    send_raw_packet(raw_sock, so_name);

    receive_arp_response(raw_sock, so_name);
}

int send_raw_packet(int sd, struct sockaddr_ll so_name)
{
    int    rc;
    struct ether_frame frame_hdr;
    struct msghdr      *msg;
    struct iovec       msgvec[1];

    printf("\n sending arp request");
    /* Fill in Ethernet header */
    uint8_t dst_addr[] = DST_MAC_ADDR;
    memcpy(frame_hdr.dst_addr, dst_addr, 6);
    memcpy(frame_hdr.src_addr, so_name.sll_addr, 6);
    /* Match the ethertype in packet_socket.c: */
   // frame_hdr.eth_proto[0] = frame_hdr.eth_proto[1] = 0xFF;
    frame_hdr.eth_proto = htons(ETH_PROTOCOL);
    /* Point to frame header */
    msgvec[0].iov_base = &frame_hdr;
    msgvec[0].iov_len  = sizeof(struct ether_frame);

    /* Allocate a zeroed-out message info struct */
    msg = (struct msghdr *)calloc(1, sizeof(struct msghdr));

    /* Fill out message metadata struct */
    memcpy(so_name.sll_addr, dst_addr, 6);
    msg->msg_name    = &so_name;
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
    printf("\n ARP request broadcasted through : ");
    for (int i = 0; i < 6; i++) {
        printf("%x", dst_addr[i]);
        if(i!=5)
            printf(":");
    }

    /* Remember that we allocated this on the heap; free it */
    free(msg);

    return rc;
}


int receive_arp_response(int raw_sock,  struct  sockaddr_ll dest_sockaddr){
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
            rc = recv_raw_packet(raw_sock, dest_sockaddr);
            printf("\n ARP response received... \n ");
            break;
            if (rc < 1) {
                perror("recv");
                return -1;
            }

            memset(buf, 0, BUF_SIZE);
        }
    }

    close(raw_sock);

    return 0;
}

int recv_raw_packet(int sd, struct sockaddr_ll dest_sockaddr)
{
    struct ether_frame  frame_hdr;
    struct msghdr       msg;
    struct iovec        msgvec[1];
    int 	  	    rc;
       /* get the frame header */
    msgvec[0].iov_base = &frame_hdr;
    msgvec[0].iov_len  = sizeof(struct ether_frame);


    /* get the message metadata struct */
    msg.msg_name    = &dest_sockaddr;
    msg.msg_namelen = sizeof(struct sockaddr_ll);
    msg.msg_iovlen  = 1;
    msg.msg_iov     = msgvec;

    rc = recvmsg(sd, &msg, 0);
    if (rc == -1) {
        perror("sendmsg");
        return -1;
    }
    /*
     * Copy the src_addr from the received raw packet for future communication
     */
    printf("\n ARP response from : ");
    for (int i = 0; i < 6; i++) {
        printf("%x",dest_sockaddr.sll_addr[i]);
        if(i!=5)
            printf(":");
    }


    return rc;
}

