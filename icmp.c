#include "header.h"

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
    struct ft_icmp_hdr *icmp_header = (struct ft_icmp_hdr *)buffer;
    struct timeval *time_sent = (struct timeval *)(buffer + ICMP_HDR_SIZE);

    if (gettimeofday(time_sent, NULL) == -1)
    {
        printf("ping: cannot get current time\n");
        return -1;
    }

    icmp_header->type = ICMP_ECHO;
    icmp_header->code = 0;
    icmp_header->id = htons(getpid() & 0xFFFF);
    icmp_header->sequence = htons(sequence_number);
    icmp_header->checksum = 0;

    icmp_header->checksum = checksum(
        (unsigned short *)buffer,
        ICMP_HDR_SIZE + ICMP_PAYLOAD_SIZE);

    return 0;
}

int prepare_socket_address(struct socket_info *si)
{
    struct addrinfo hint;
    struct addrinfo *temp;

    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_RAW;
    hint.ai_protocol = IPPROTO_ICMP;

    int ret = getaddrinfo(si->host, NULL, &hint, &temp);
    if (ret != 0)
    {
        printf("ping: cannot resolve %s: Unknown host\n", si->host);
        return -1;
    }

    si->remote_address = *(struct sockaddr_in *)temp->ai_addr;

    if (inet_ntop(AF_INET, &si->remote_address.sin_addr, si->str_sin_addr, INET_ADDRESS_LENGTH) == NULL)
    {
        printf("ping: cannot convert host address to string\n");
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
        printf("ping: cannot create socket\n");
        return -1;
    }

    if (setsockopt(*socket_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) == -1)
    {
        printf("ping: cannot set socket options\n");
        close(*socket_fd);
        return -1;
    }
    return (0);
}


int receive_icmp_reply(int socket_fd, struct packet_data *pd, struct ping_options *options)
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
                printf("ping: cannot create new RTT node\n");
                return -1;
            }
            print_ping_result(buff, nb_bytes, pd, options);
        }
    }
    if (errno != EAGAIN && errno != EWOULDBLOCK)
    {
        perror("recvmsg");
        return -1;
    }
    return (-1);
}

int send_icmp_echo_request(int socket_fd, struct socket_info *si, int sequence_number)
{
    u_int8_t buffer[ICMP_PAYLOAD_SIZE + sizeof(struct ft_icmp_hdr)];
    memset(buffer, 0, sizeof(struct ft_icmp_hdr) + ICMP_PAYLOAD_SIZE);
    fill_icmp_header(buffer, sequence_number);

    if (sendto(socket_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&si->remote_address, sizeof(si->remote_address)) == -1)
    {
        printf("ping: cannot send ICMP echo request\n");
        return -1;
    }
    return (0);
}

int filter_icmp_reply(uint8_t *buffer)
{
    struct ft_icmp_hdr *hdr_rep = get_icmp_header_format(buffer);

    if (hdr_rep->type != ICMP_ECHOREPLY)
    {
        return 0;
    }

    return ntohs(hdr_rep->id) == (getpid() & 0xFFFF);
}