#ifndef HEADER_H
#define HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <math.h>

#include <errno.h>
#include <sys/time.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>

struct ft_ipv4_hdr
{
    uint8_t ver_ihl;
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
};

struct ft_icmp_hdr
{
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
};

#define IP_HDR_SIZE (sizeof(struct ft_ipv4_hdr))
#define ICMP_HDR_SIZE (sizeof(struct ft_icmp_hdr))
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

struct ping_options {
    bool quiet;
    bool verbose;
    bool help;
};

struct rtt_node {
    struct timeval rtt;
    struct rtt_node *next;
};

struct packet_data {
    int sequence_number;
    int seccesfully_received;
    bool awaiting_echo_reply;
    bool got_echo_reply;
    struct timeval *min;
    struct timeval *max;
    struct timeval avg;
    double std_dev;

    struct rtt_node *rtt_head;
    struct rtt_node *rtt_tail;
};

void print_help(void);
int parse_args(int argc, char **argv, struct ping_options *options, char **hostname_out);
void print_start_info(const struct socket_info *si);
void print_end_info(struct socket_info *si, struct packet_data *pd);
void print_ping_result(void *buffer, size_t nbytes, struct packet_data *pd, struct ping_options *options);
void print_request_timeout(unsigned seq, const struct ping_options *options);

double calc_packet_loss(struct packet_data *pd);
float convert_time_to_ms(struct timeval *time);
char *get_icmp_error_message(int type, int code);
struct ft_icmp_hdr *get_icmp_header_format(void *buffer);
struct timeval *get_sent_time(void *buffer);
void *skip_ip_header(void *buffer);

int prepare_socket_address(struct socket_info *si);
int init_socket(int *socket_fd, struct socket_info *si, char *host, int ttl);
int receive_icmp_reply(int socket_fd, struct packet_data *pd, struct ping_options *options);
int send_icmp_echo_request(int socket_fd, struct socket_info *si, int sequence_number);
int filter_icmp_reply(uint8_t *buffer);

struct rtt_node *create_new_rtt_node(struct packet_data *pd, void *buffer);
void update_min_max_avg(struct packet_data *pd);
void clean_rtts_list(struct packet_data *pd);
struct rtt_node *get_last_rtt_node(struct packet_data *pd);
bool is_user_root(void);
void calculate_stddev(struct packet_data *pd);
void handler(int signum);

static inline int timecmp(const struct timeval *a, const struct timeval *b)
{
    if (a->tv_sec != b->tv_sec)
        return (a->tv_sec > b->tv_sec) - (a->tv_sec < b->tv_sec);
    if (a->tv_usec != b->tv_usec)
        return (a->tv_usec > b->tv_usec) - (a->tv_usec < b->tv_usec);
    return 0;
}

#endif