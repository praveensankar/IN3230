//
// Created by praveen on 9/15/21.
//
#include <stdlib.h>		/* free */
#include <stdio.h> 		/* printf */
#include <string.h>		/* memset */
#include <sys/socket.h>		/* socket */
#include <linux/if_packet.h>	/* AF_PACKET */
#include <net/ethernet.h>	/* ETH_* */
#include <arpa/inet.h>		/* htons */
#include <ifaddrs.h>		/* getifaddrs */
#include <strings.h> /* bzero */
#include "common.h"



int create_socket()
{
    // 0x88b5 and 0x88b6 ethernet type protocols have been reserved for private and experimental purposes
    // ETH_P_ALL -  allows any EtherType to be received without using multiple sockets
    // 0xffff - broadcast address
   // int protocol = 0x88b5;
    int raw_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_PROTOCOL));
    if(raw_sock == -1)
    {
        perror("raw socket");
        exit(EXIT_FAILURE);
    }
    return raw_sock;
}


// run the ifconfig command in the terminal to verify the information
void display_mac(struct sockaddr_ll *sock_addr)
{
    /*
     mac address size : 48 bits == 6 ints == 12 hex
     */
    for (int i = 0; i < 6; i++) {
        printf("%x", sock_addr->sll_addr[i]);
        if(i!=5)
            printf(":");
    }
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
            ifp->ifa_addr->sa_family == AF_PACKET && strcmp(ifp->ifa_name, "lo") != 0) {
            /* Copy the address info into our temp. variable */
            memcpy(so_name, (struct sockaddr_ll *) ifp->ifa_addr, sizeof(struct sockaddr_ll));
            printf("\n Interface address : ");
            display_mac(so_name);
            break;
        }
    }
    /* After the loop, the address info of the last interface
       enumerated is stored in so_name. */

    /* Free the interface list */
    freeifaddrs(ifaces);

    return;
}



