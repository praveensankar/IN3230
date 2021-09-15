#include <stdio.h> 		/* printf */
#include <stdlib.h>		/* free */
#include <unistd.h>             /* fgets */
#include <string.h>		/* memset */
#include <sys/socket.h>		/* socket */
#include <sys/epoll.h>          /* epoll */
#include <linux/if_packet.h>	/* AF_PACKET */
#include <net/ethernet.h>	/* ETH_* */
#include <arpa/inet.h>		/* htons */
#include <ifaddrs.h>		/* getifaddrs */

#define MAX_EVENTS 10
#define BUF_SIZE 1450


static uint8_t dst_addr[6];

struct ether_frame {
    uint8_t dst_addr[6];
    uint8_t src_addr[6];
    uint8_t eth_proto[2];
    uint8_t contents[0];
} __attribute__((packed));


int create_socket();
void get_mac_from_interface(struct sockaddr_ll *so_name);
int receive_message(int raw_sock);
int recv_raw_packet(int sd, uint8_t *buf, size_t len);


int main(int argc, char *argv[])
{
	struct    sockaddr_ll so_name;
	int       raw_sock;



	/* Set up a raw AF_PACKET socket without ethertype filtering */
     raw_sock = create_socket();

	/* Fill the fields of so_name with info from interface */
	get_mac_from_interface(&so_name);

    receive_message(raw_sock);

}


int create_socket()
{
    // 0x88b5 and 0x88b6 ethernet type protocols have been reserved for private and experimental purposes
    // ETH_P_ALL -  allows any EtherType to be received without using multiple sockets
    // 0xffff - broadcast address
    int protocol = 0xffff;
    int raw_sock = socket(AF_PACKET, SOCK_RAW, htons(protocol));
    if(raw_sock == -1)
    {
        perror("raw socket");
        exit(EXIT_FAILURE);
    }
    return raw_sock;
}



/*
 * This function gets a pointer to a struct sockaddr_ll
 * and fills it with necessary address info from the interface device.
 */
void get_mac_from_interface(struct sockaddr_ll *so_name)
{
    struct ifaddrs *ifaces, *ifp;
    /* Enumerate interfaces: */
    /* Note in man getifaddrs that this function dynamically allocates
       memory. It becomes our responsability to free it! */
    getifaddrs(&ifaces);

    /* Walk the list looking for ifaces interesting to us */
    //printf("Interface list:\n");
    for (ifp = ifaces; ifp != NULL; ifp = ifp->ifa_next) {
        /* We make certain that the ifa_addr member is actually set: */
        if (ifp->ifa_addr != NULL &&
            ifp->ifa_addr->sa_family == AF_PACKET)
            /* Copy the address info into our temp. variable */
            memcpy(so_name, (struct sockaddr_ll*)ifp->ifa_addr, sizeof(struct sockaddr_ll));
    }
    /* After the loop, the address info of the last interface
       enumerated is stored in so_name. */

    /* Free the interface list */
    freeifaddrs(ifaces);

    return;
}


int receive_message(int raw_sock){
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
            printf("you have a new message through raw socket ...\n");
            rc = recv_raw_packet(raw_sock, buf, BUF_SIZE);
            if (rc < 1) {
                perror("recv");
                return -1;
            }

            printf("\nmsg: %s\n", buf);
            if(strcmp(buf, "close")==0)
            {
                break;
            }
            memset(buf, 0, BUF_SIZE);
        }
    }

    close(raw_sock);

    return 0;
}

int recv_raw_packet(int sd, uint8_t *buf, size_t len)
{
    struct sockaddr_ll so_name;
    struct ether_frame  frame_hdr;
    struct msghdr       msg;
    struct iovec        msgvec[2];
    int 	  	    rc;

    /* Point to frame header */
    msgvec[0].iov_base = &frame_hdr;
    msgvec[0].iov_len  = sizeof(struct ether_frame);
    /* Point to frame payload */
    msgvec[1].iov_base = buf;
    msgvec[1].iov_len  = len;

    /* Fill out message metadata struct */
    msg.msg_name    = &so_name;
    msg.msg_namelen = sizeof(struct sockaddr_ll);
    msg.msg_iovlen  = 2;
    msg.msg_iov     = msgvec;

    rc = recvmsg(sd, &msg, 0);
    if (rc == -1) {
        perror("sendmsg");
        return -1;
    }
    /*
     * Copy the src_addr of the current frame to the global dst_addr
     * We need that address as a dst_addr for the next frames we're going to send from the server
     */
    memcpy(dst_addr, frame_hdr.src_addr, 6);

    return rc;
}

