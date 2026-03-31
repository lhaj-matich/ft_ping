#include "header.h"

bool pingloop = true;
bool send_packet = true;

void handler(int signum)
{
    if (signum == SIGINT)
    {
        pingloop = false;
    }
    else if (signum == SIGALRM)
    {
        send_packet = true;
    }
}

struct rtt_node *create_new_rtt_node(struct packet_data *pd, void *buffer)
{
    struct rtt_node *head = pd->rtt_head;
    struct rtt_node *new_elemt;
    struct timeval *t_packet_send;
    struct timeval t_packet_revd;

    t_packet_send = get_sent_time(buffer);
    if (gettimeofday(&t_packet_revd, NULL) == -1)
    {
        printf("Error: Could not get current time.\n");
        return NULL;
    }

    new_elemt = (struct rtt_node *)malloc(sizeof(struct rtt_node));
    if (!new_elemt)
    {
        return NULL;
    }

    timersub(&t_packet_revd, t_packet_send, &new_elemt->rtt);

    new_elemt->next = NULL;
    if (head != NULL)
    {
        pd->rtt_tail->next = new_elemt;
        pd->rtt_tail = new_elemt;
    }
    else
    {
        pd->rtt_head = new_elemt;
        pd->rtt_tail = new_elemt;
    }
}

void clean_rtts_list(struct packet_data *pd)
{
    struct rtt_node *current = pd->rtt_head;
    while (current != NULL)
    {
        struct rtt_node *next = current->next;
        free(current);
        current = next;
    }
    pd->rtt_head = NULL;
    pd->rtt_tail = NULL;
}

static inline struct icmphdr *get_icmp_header_format(void *buffer)
{
    struct iphdr *ip = (struct iphdr *)buffer;
    return (struct icmphdr *)((uint8_t *)buffer + (ip->ihl * 4));
}

static inline struct timeval *get_sent_time(void *buffer)
{
    return (struct timeval *)((uint8_t *)buffer + IP_HDR_SIZE + ICMP_HDR_SIZE);
}

void print_rtts(struct packet_data *pd)
{
    struct rtt_node *head = pd->rtt_head;
    int i = 1;

    while (head != NULL)
    {
        printf("RTT for packet %d: %ld.%06ld seconds\n", i, head->rtt.tv_sec, head->rtt.tv_usec);
        head = head->next;
        i++;
    }
}

struct rtt_node *get_last_rtt_node(struct packet_data *pd)
{
    if (pd->rtt_tail != NULL)
    {
        return pd->rtt_tail;
    }
    return NULL;
}

static unsigned short checksum(unsigned short *ptr, int length)
{
    unsigned long sum;
    unsigned short oddbyte;

    sum = 0;
    while (length > 1)
    {
        sum += *ptr++;
        length -= 2;
    }
    if (length == 1)
    {
        oddbyte = 0;
        *((unsigned char *)&oddbyte) = *(unsigned char *)ptr;
        sum += oddbyte;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (unsigned short)~sum;
}

static int fill_icmp_header(u_int8_t *buffer, int sequence_number)
{
    struct icmphdr *icmp_header = (struct icmphdr *)buffer;
    struct timeval *time_sent = (struct timeval *)(buffer + ICMP_HDR_SIZE);

    if (gettimeofday(time_sent, NULL) == -1)
    {
        printf("Error: Could not get current time.\n");
        return -1;
    }

    icmp_header->type = ICMP_ECHO;
    icmp_header->code = 0;
    icmp_header->un.echo.id = htons(getpid() & 0xFFFF);
    icmp_header->un.echo.sequence = htons(sequence_number);
    icmp_header->checksum = 0;

    icmp_header->checksum = checksum(
        (unsigned short *)buffer,
        ICMP_HDR_SIZE + ICMP_PAYLOAD_SIZE);

    return 0;
}

static int send_icmp_echo_request(int socket_fd, struct socket_info *si, int sequence_number)
{
    u_int8_t buffer[ICMP_PAYLOAD_SIZE + sizeof(struct icmphdr)];
    memset(buffer, 0, sizeof(struct icmphdr) + ICMP_PAYLOAD_SIZE);
    fill_icmp_header(buffer, sequence_number);

    if (sendto(socket_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&si->remote_address, sizeof(si->remote_address)) == -1)
    {
        printf("Error: Could not send ICMP echo request.\n");
        return -1;
    }
    return (0);
}

int prepare_socket_address(struct socket_info *si)
{
    struct addrinfo hint;
    struct addrinfo *temp;

    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_RAW;
    hint.ai_protocol = IPPROTO_ICMP;

    if (getaddrinfo(si->host, NULL, &hint, &temp) != 0)
    {
        printf("Error: could not get host address.\n");
        return -1;
    }

    si->remote_address = *(struct sockaddr_in *)temp->ai_addr;

    if (inet_ntop(AF_INET, &si->remote_address.sin_addr, si->str_sin_addr, INET_ADDRESS_LENGTH) == NULL)
    {
        printf("Error: could not convert host address to string.\n");
        return -1;
    }
    return 0;
}

int init_socket(int *socket_fd, struct socket_info *si, char *host, int ttl)
{
    si->host = host;
    if (prepare_socket_address(si) == -1)
        return -1;

    if ((*socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
    {
        printf("Could not create socket");
        return -1;
    }

    if (setsockopt(*socket_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) == -1)
    {
        printf("Error: Setting socket options");
        close(*socket_fd);
        return -1;
    }
    return (0);
}

static inline void *skip_ip_header(void *buffer)
{
    return (void *)((uint8_t *)buffer + IP_HDR_SIZE);
}

int filter_icmp_reply(uint8_t *buffer)
{
    struct icmphdr *hdr_rep = get_icmp_header_format(buffer);

    if (hdr_rep->type != ICMP_ECHOREPLY)
    {
        printf("The type is not ICMP_ECHOREPLY, it is: %d\n", hdr_rep->type);
        return 0;
    }

    return ntohs(hdr_rep->un.echo.id) == (getpid() & 0xFFFF);
}

void print_ping_result(void *buffer, size_t nbytes, struct packet_data *pd)
{
    char ip_str[INET_ADDRESS_LENGTH];
    struct iphdr *ip_header;
    struct icmphdr *icmp_header;
    struct rtt_node *last_rtt_node;
    unsigned int seq_num;
    double time_ms;

    last_rtt_node = get_last_rtt_node(pd);
    ip_header = (struct iphdr *)buffer;
    icmp_header = skip_ip_header(buffer);

    inet_ntop(AF_INET, &ip_header->saddr, ip_str, INET_ADDRESS_LENGTH);
    seq_num = ntohs(icmp_header->un.echo.sequence);

    if (last_rtt_node)
    {
        time_ms = last_rtt_node->rtt.tv_sec * 1000.0 + last_rtt_node->rtt.tv_usec / 1000.0;
    }
    else
    {
        time_ms = 0.0;
    }

    printf("%zu bytes from %s (%s): icmp_seq=%u ttl=%d time=%.1f ms\n",
           nbytes - IP_HDR_SIZE, ip_str, ip_str, seq_num, ip_header->ttl, time_ms);
}

int receive_icmp_reply(int socket_fd, struct packet_data *pd)
{
    uint8_t buff[REPLY_PACKET_SIZE] = {};
    ssize_t nb_bytes;
    struct iovec iov[1];
    struct msghdr msg = {};

    iov[0].iov_base = buff;
    iov[0].iov_len = sizeof(buff);

    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    nb_bytes = recvmsg(socket_fd, &msg, MSG_DONTWAIT);
    if (nb_bytes > 0)
    {
        if (filter_icmp_reply(buff))
        {
            pd->seccesfully_received = pd->seccesfully_received + 1;
            if (create_new_rtt_node(pd, buff) == NULL)
            {
                printf("Error: Could not create new RTT node.\n");
                return -1;
            }
            print_ping_result(buff, nb_bytes, pd);
            // print_rtts(pd);
            // print_last_rtt(pd);
        }
    }
    if (errno != EAGAIN && errno != EWOULDBLOCK)
    {
        perror("recvmsg");
        return -1;
    }
    return (-1);
}

int main(int argc, char **argv)
{
    struct socket_info si;
    struct packet_data pd = {0};
    int socket_file_descriptor;

    if (argc < 2)
    {
        print_help();
        return (1);
    }

    signal(SIGINT, handler);
    signal(SIGALRM, handler);
    if (init_socket(&socket_file_descriptor, &si, argv[1], IP_TTL_VALUE) == -1)
    {
        printf("Error: Could not initialize socket.\n");
        return (1);
    }
    print_start_info(&si);
    while (pingloop)
    {
        if (send_packet)
        {
            int ret = send_icmp_echo_request(socket_file_descriptor, &si, pd.sequence_number);
            if (ret == -1)
            {
                printf("Error: Could not send ICMP echo request.\n");
                close(socket_file_descriptor);
                return (1);
            }
            pd.sequence_number++;
            send_packet = false;
            alarm(1);
        }
        receive_icmp_reply(socket_file_descriptor, &pd);
    }
    return (0);
}