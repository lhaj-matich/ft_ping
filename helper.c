#include "header.h"

double calc_packet_loss(struct packet_data *pd)
{
	if (pd->sequence_number <= 0)
		return (0.0);
	return (double)(pd->sequence_number - pd->seccesfully_received) / pd->sequence_number * 100.0;
}

float convert_time_to_ms(struct timeval *time)
{
    return (float)(time->tv_sec * 1000.0 + time->tv_usec / 1000.0);
}

bool is_user_root(void)
{
    return (getuid() == 0 ? true : false);
}

char *get_icmp_error_message(int type, int code) {
    switch (type) {

    case ICMP_DEST_UNREACH:
        switch(code) {
        case ICMP_NET_UNREACH:
            return "Destination Net Unreachable";
        case ICMP_HOST_UNREACH:
            return "Destination Host Unreachable";
        case ICMP_PROT_UNREACH:
            return "Destination Protocol Unreachable";
        case ICMP_PORT_UNREACH:
            return "Destination Port Unreachable";
        case ICMP_FRAG_NEEDED:
            return "Frag needed";
        case ICMP_SR_FAILED:
            return "Source Route Failed";
        case ICMP_NET_UNKNOWN:
            return "Destination Net Unknown";
        case ICMP_HOST_UNKNOWN:
            return "Destination Host Unknown";
        case ICMP_HOST_ISOLATED:
            return "Source Host Isolated";
        case ICMP_NET_ANO:
            return "Destination Net Prohibited";
        case ICMP_HOST_ANO:
            return "Destination Host Prohibited";
        case ICMP_NET_UNR_TOS:
            return "Destination Net Unreachable for TOS";
        case ICMP_HOST_UNR_TOS:
            return "Destination Host Unreachable for TOS";
        case ICMP_PKT_FILTERED:
            return "Packet filtered";
        case ICMP_PREC_VIOLATION:
            return "Precedence Violation";
        case ICMP_PREC_CUTOFF:
            return "Precedence Cutoff";
        default:
            return "Dest Unreachable, Bad Code";
        }

    case ICMP_SOURCEQUENCH:
        return "Source Quench";

    case ICMP_REDIRECT:
        switch(code) {
        case ICMP_REDIR_NET:
            return "Redirect Network";
        case ICMP_REDIR_HOST:
            return "Redirect Host";
        case ICMP_REDIR_NETTOS:
            return "Redirect TOS and Network";
        case ICMP_REDIR_HOSTTOS:
            return "Redirect TOS and Host";
        default:
            return "Redirect, Bad Code";
        }

    case ICMP_TIMXCEED:
        switch(code) {
        case ICMP_TIMXCEED_INTRANS:
            return "TTL expired";
        case ICMP_TIMXCEED_REASS:
            return "Reassembly time exceeded";
        default:
            return "Time exceeded, Bad Code";
        }

    case ICMP_PARAMETERPROB:
        switch(code) {
        case 0:
            return "Bad pointer";
        case 1:
            return "Bad length";
        case 2:
            return "Required option absent";
        default:
            return "Parameter problem, Bad Code";
        }

    default:
        return "Unknown ICMP type";
    }
}

struct ft_icmp_hdr *get_icmp_header_format(void *buffer)
{
    struct ft_ipv4_hdr *ip = (struct ft_ipv4_hdr *)buffer;
    unsigned int ihl = ip->ver_ihl & 0x0Fu;

    return (struct ft_icmp_hdr *)((uint8_t *)buffer + ihl * 4);
}

struct timeval *get_sent_time(void *buffer)
{
    uint8_t *icmp_start = (uint8_t *)buffer;
    struct ft_ipv4_hdr *ip = (struct ft_ipv4_hdr *)buffer;
    unsigned int ip_header_len = (ip->ver_ihl & 0x0F) * 4;

    icmp_start += ip_header_len;

    return (struct timeval *)(icmp_start + ICMP_HDR_SIZE);
}

void *skip_ip_header(void *buffer)
{
    struct ft_ipv4_hdr *ip = (struct ft_ipv4_hdr *)buffer;
    unsigned int ip_header_len = (ip->ver_ihl & 0x0F) * 4;

    return ((uint8_t *)buffer + ip_header_len);
}