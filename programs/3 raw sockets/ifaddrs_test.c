//
// Created by praveen on 9/8/21.
//

#include <ifaddrs.h>		/* getifaddrs */
#include <stdio.h>
#include <sys/socket.h>
#include <linux/if_packet.h>

void display_mac(struct ifaddrs *ifp);

void print_mac_from_interface()
{
    struct ifaddrs *ifaces, *ifp;
    /* Enumerate interfaces: */

    getifaddrs(&ifaces);

    /* Walk the list looking for ifaces interesting to us */
    //printf("Interface list:\n");
    for (ifp = ifaces; ifp != NULL; ifp = ifp->ifa_next) {
        /* We make certain that the ifa_addr member is actually set: */
        if (ifp->ifa_addr != NULL &&
            ifp->ifa_addr->sa_family == AF_PACKET &&
            strcmp(ifp->ifa_name,"lo")!=0) {
            display_mac(ifp);
            printf("\n");
        }
    }


    /* Free the interface list */
    freeifaddrs(ifaces);
    return;
}

// run the ifconfig command in the terminal to verify the information
void display_mac(struct ifaddrs *ifp)
{
    struct sockaddr_ll *sock_addr = (struct sockaddr_ll *) ifp->ifa_addr;
    printf("\n -----------new mac address-------------");
    printf("\n interface name : %s", ifp->ifa_name);
    printf("\n Interface address : ");
    printf("\n \t  family : %d", sock_addr->sll_family);
    printf("\n \t interface index : %d ", sock_addr->sll_ifindex);
    printf("\n \t physical address : ");

    /*
     mac address size : 48 bits == 6 ints == 12 hex
     */
    for (int i = 0; i < 6; i++) {
        printf("%x", sock_addr->sll_addr[i]);
        if(i!=5)
            printf(":");
    }
    printf("\n \t packet type : %d ", sock_addr->sll_pkttype);
}



int main()
{
    print_mac_from_interface();
    return 0;
}