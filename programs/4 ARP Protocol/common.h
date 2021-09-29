//
// Created by praveen on 9/15/21.
//

#ifndef COMMON_H
#define COMMON_H

#define ETH_PROTOCOL 0x88b5

struct ether_frame {
    uint8_t dst_addr[6];
    uint8_t src_addr[6];
    uint16_t eth_proto;
} __attribute__((packed));




#endif // COMMON_H
