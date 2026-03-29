#ifndef HEADER_H
#define HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <errno.h>
#include <sys/time.h>

#define IP_HDR_SIZE (sizeof(struct iphdr))
#define ICMP_HDR_SIZE (sizeof(struct icmphdr))
#define INET_ADDRESS_LENGTH 16
#define IP_TTL_VALUE 64
#define ICMP_PAYLOAD_SIZE 56
#define REPLY_PACKET_SIZE ((IP_HDR_SIZE + ICMP_HDR_SIZE) * 2 + ICMP_PAYLOAD_SIZE + 1)

struct socket_info
{
    char *host;
    struct sockaddr_in remote_address;
    char str_sin_addr[INET_ADDRESS_LENGTH];
};

struct rtt_node {
    struct timeval rtt;
    struct rtt_node *next;
};

struct packet_data {
    int sequence_number;
    int seccesfully_received;
    struct timeval *min;
    struct timeval *max;
    struct timeval avg;

    struct rtt_node *rtt_head;
    struct rtt_node *rtt_tail;
};

#endif