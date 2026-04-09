#include "header.h"

void print_help()
{
    printf("Usage: ping <host>\n");
    printf("Options:\n");
    printf("  -v, --verbose\t\tVerbose output\n");
    printf("  -q, --quiet\t\tBe quiet (don't print status messages)\n");
    printf("  -h, --help\t\tShow this help message and exit\n");
}

void print_start_info(const struct socket_info *si, const struct ping_options *options)
{
    unsigned id = (unsigned)(getpid() & 0xFFFF);

    if (options->verbose)
        printf("PING %s (%s): %d data bytes, id 0x%04x = %u\n",
               si->host, si->str_sin_addr, ICMP_PAYLOAD_SIZE, id, id);
    else
        printf("PING %s (%s): %d bytes of data.\n", si->host, si->str_sin_addr, ICMP_PAYLOAD_SIZE);
}

void print_request_timeout(unsigned seq, const struct ping_options *options)
{
    if (options->quiet)
        return;
    printf("Request timeout for icmp_seq %u\n", seq);
}

void print_end_info(struct socket_info *si, struct packet_data *pd)
{
    printf("\n--- %s ping statistics ---\n", si->host);
    if (pd->sequence_number > 0)
        printf("%d packets transmitted, %d packets received, %.1f%% packet loss\n",
               pd->sequence_number, pd->seccesfully_received, calc_packet_loss(pd));
    else
        printf("0 packets transmitted, 0 packets received, 0.0%% packet loss\n");
    if (pd->seccesfully_received > 0)
    {
        update_min_max_avg(pd);
        calculate_stddev(pd);
        double min_ms = pd->min->tv_sec * 1000.0 + pd->min->tv_usec / 1000.0;
        double avg_ms = pd->avg.tv_sec * 1000.0 + pd->avg.tv_usec / 1000.0;
        double max_ms = pd->max->tv_sec * 1000.0 + pd->max->tv_usec / 1000.0;
        double std_dev_ms = pd->std_dev;
        printf("round-trip min/avg/max/std dev = ");
        printf("%.3f ms/", min_ms);
        printf("%.3f ms/", avg_ms);
        printf("%.3f ms/", max_ms);
        printf("%.3f ms\n", std_dev_ms);
    }
}

void print_ping_result(void *buffer, size_t nbytes, struct packet_data *pd, struct ping_options *options)
{
    char ip_str[INET_ADDRESS_LENGTH];
    struct ft_ipv4_hdr *ip_header;
    struct ft_icmp_hdr *icmp_header;
    struct rtt_node *last_rtt_node;
    unsigned int seq_num;
    double time_ms;

    last_rtt_node = get_last_rtt_node(pd);
    ip_header = (struct ft_ipv4_hdr *)buffer;
    icmp_header = skip_ip_header(buffer);

    inet_ntop(AF_INET, &ip_header->saddr, ip_str, INET_ADDRESS_LENGTH);
    seq_num = ntohs(icmp_header->sequence);

    if (options->quiet == false && icmp_header->type == ICMP_ECHOREPLY) {
        if (last_rtt_node)
            time_ms = convert_time_to_ms(&last_rtt_node->rtt);
        else
            time_ms = 0.0;
        printf("%zu bytes from %s: icmp_seq=%u ttl=%d time=%.3f ms\n",
               nbytes - IP_HDR_SIZE, ip_str, seq_num, ip_header->ttl, time_ms);
    }
    else if (icmp_header->type != ICMP_ECHOREPLY) {
        printf("%ld bytes from %s: %s\n",
               nbytes - IP_HDR_SIZE, ip_str, get_icmp_error_message(icmp_header->type, icmp_header->code));
        if (options->verbose){
            fprintf(stderr, "ICMP: Type: %d, Code: %d\n", icmp_header->type, icmp_header->code);
        }
    }
}