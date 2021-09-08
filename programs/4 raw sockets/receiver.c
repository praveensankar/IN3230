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

int send_raw_packet(int sd,
		    struct sockaddr_ll *so_name,
		    uint8_t *buf,
		    size_t len)
{
	struct ether_frame frame_hdr;
	struct msghdr      *msg;
	struct iovec       msgvec[2];
	int    rc;

	/* Fill in Ethernet header */
	memcpy(frame_hdr.dst_addr, dst_addr, 6);
	memcpy(frame_hdr.src_addr, so_name->sll_addr, 6);
	/* Match the ethertype in packet_socket.c: */
	frame_hdr.eth_proto[0] = frame_hdr.eth_proto[1] = 0xFF;

	/* Point to frame header */
	msgvec[0].iov_base = &frame_hdr;
	msgvec[0].iov_len = sizeof(struct ether_frame);
	/* Point to frame payload */
	msgvec[1].iov_base = buf;
	msgvec[1].iov_len  = len;

	/* Allocate a zeroed-out message info struct */
	msg = (struct msghdr *)calloc(1, sizeof(struct msghdr));

	/* Fill out message metadata struct */
	memcpy(so_name->sll_addr, dst_addr, 6);
	msg->msg_name 	 = so_name;
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
	if (getifaddrs(&ifaces)) {
		perror("getifaddrs");
		exit(-1);
	}

	/* Walk the list looking for ifaces interesting to us */
	for (ifp = ifaces; ifp != NULL; ifp = ifp->ifa_next) {
		/* We make certain that the ifa_addr member is actually set: */
		if (ifp->ifa_addr != NULL && ifp->ifa_addr->sa_family == AF_PACKET && (strcmp("lo", ifp->ifa_name)))
			/* Copy the address info into our temp. variable */
			memcpy(so_name, (struct sockaddr_ll*)ifp->ifa_addr, sizeof(struct sockaddr_ll));
	}
	/* After the loop, the address info of the last interface
	   enumerated is stored in so_name. */

	/* Free the interface list */
	freeifaddrs(ifaces);

	return;
}


int main(int argc, char *argv[])
{
	struct    sockaddr_ll so_name;
	uint8_t   buf[BUF_SIZE];
	uint8_t   username[8];
	int       raw_sock, rc;

	struct epoll_event ev, events[MAX_EVENTS];
	int epollfd;

	short unsigned int protocol = 0xFFFF;

	/* Set up a raw AF_PACKET socket without ethertype filtering */
	raw_sock = socket(AF_PACKET, SOCK_RAW, htons(protocol));
	if (raw_sock == -1) {
		perror("socket");
		return -1;
	}

	rc = recv_raw_packet(raw_sock, username, sizeof(username));
	if (rc == -1) {
		perror("recv_raw_packet");
		return -1;
	}

	printf("\n%s knocked on the door...\n", username);

	/* Fill the fields of so_name with info from interface */
	get_mac_from_interface(&so_name);

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

	ev.data.fd = 0;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, 0, &ev) == -1) {
		perror("epoll_ctl: stdin");
		exit(EXIT_FAILURE);
	}

	while(1) {
		rc = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (rc == -1) {
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		}

		if (events->data.fd == 0) {
			printf("Someone knocked on stdin...\n");
			memset(buf, 0, BUF_SIZE);
			fgets((char *)buf, sizeof(buf), stdin);
			if (strstr((char *)buf, "ADIOS") != NULL) {
				printf("ADIOS\n");
				break;
			}
			send_raw_packet(raw_sock, &so_name, buf, strlen(buf));
		} else if (events->data.fd == raw_sock) {
			printf("Someone knocked on the raw socket...\n");
			rc = recv_raw_packet(raw_sock, buf, BUF_SIZE);
			if (rc < 1) {
				perror("recv");
				return -1;
			}

			printf("\n<%s>: %s\n",username, buf);
		}
	}

	close(raw_sock);
	
	return 0;
}
