#include <stdlib.h>		/* free */
#include <stdio.h> 		/* printf */
#include <unistd.h>             /* fgets */
#include <string.h>		/* memset */
#include <sys/socket.h>		/* socket */
#include <fcntl.h>
#include <sys/epoll.h>          /* epoll */
#include <linux/if_packet.h>	/* AF_PACKET */
#include <net/ethernet.h>	/* ETH_* */
#include <arpa/inet.h>		/* htons */
#include <ifaddrs.h>		/* getifaddrs */
#include <time.h>  /* time */
#include <strings.h> /* bzero */


#define MAX_EVENTS 10
#define BUF_SIZE 1450
#define DST_MAC_ADDR {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}
#define ETH_BROADCAST_ADDR {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}
#define ETHERNET_TYPE_PROTO 0xffff
struct ether_frame {
    uint8_t dst_addr[6];
    uint8_t src_addr[6];
    uint8_t eth_proto[2];
    uint8_t contents[0];
} __attribute__((packed));


int create_socket();
void get_mac_from_interface(struct sockaddr_ll *so_name);
int send_message(int raw_sock, struct sockaddr_ll so_name);
int send_raw_packet(int sd, struct sockaddr_ll *so_name, uint8_t *buf, size_t len);


int main(int argc, char *argv[])
{
	int     raw_sock;
	struct  sockaddr_ll so_name;

	/* Step 1: Set up Raw socket */

    raw_sock = create_socket();

    /* Step 2: address info of the last interface enumerated is stored in so_name*/
	get_mac_from_interface(&so_name);

    /* Step 3: send messages to the receiver */
    send_message(raw_sock, so_name);

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


int send_message(int raw_sock, struct sockaddr_ll so_name){
    int rc;
    struct epoll_event ev, events[MAX_EVENTS];
    int epollfd;
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);

    epollfd = epoll_create1(0);
	ev.events = EPOLLIN;
	ev.data.fd = 0;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, 0, &ev);

	while(1) {

		rc = epoll_wait(epollfd, events, MAX_EVENTS, -1);

		if (events->data.fd == 0) {

			fgets(buf, sizeof(buf), stdin);
			send_raw_packet(raw_sock, &so_name, buf, strlen(buf));
            bzero(buf, sizeof(buf));
            if(strcmp(buf,"close")==0)
            {
                break;
            }

		}
	}

	close(raw_sock);

	return 0;
}

int send_raw_packet(int sd, struct sockaddr_ll *so_name, uint8_t *buf, size_t len)
{
    int    rc;
    struct ether_frame frame_hdr;
    struct msghdr      *msg;
    struct iovec       msgvec[2];

    /* Fill in Ethernet header */
    uint8_t dst_addr[] = DST_MAC_ADDR;
    memcpy(frame_hdr.dst_addr, dst_addr, 6);
    memcpy(frame_hdr.src_addr, so_name->sll_addr, 6);
    /* Match the ethertype in packet_socket.c: */
    frame_hdr.eth_proto[0] = frame_hdr.eth_proto[1] = 0xFF;

    /* Point to frame header */
    msgvec[0].iov_base = &frame_hdr;
    msgvec[0].iov_len  = sizeof(struct ether_frame);
    /* Point to frame payload */
    msgvec[1].iov_base = buf;
    msgvec[1].iov_len  = len;

    /* Allocate a zeroed-out message info struct */
    msg = (struct msghdr *)calloc(1, sizeof(struct msghdr));

    /* Fill out message metadata struct */
    memcpy(so_name->sll_addr, dst_addr, 6);
    msg->msg_name    = so_name;
    msg->msg_namelen = sizeof(struct sockaddr_ll);
    msg->msg_iovlen  = 2;
    msg->msg_iov     = msgvec;

    /* Construct and send message */
    rc = sendmsg(sd, msg, 0);
    if (rc == -1) {
        perror("sendmsg");
        free(msg);
        return 1;
    }

    /* Remember that we allocated this on the heap; free it */
    free(msg);

    return rc;
}


