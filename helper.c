#include "header.h"

double calc_packet_loss(struct packet_data *pd)
{
	return (double)(pd->sequence_number - pd->seccesfully_received) / pd->sequence_number * 100;
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
            break;
        case ICMP_HOST_UNREACH:
            return "Destination Host Unreachable";
            break;
        case ICMP_PROT_UNREACH:
            return "Destination Protocol Unreachable";
            break;
        case ICMP_PORT_UNREACH:
            return "Destination Port Unreachable";
            break;
        case ICMP_FRAG_NEEDED:
            return "Frag needed";
            break;
        case ICMP_SR_FAILED:
            return "Source Route Failed";
            break;
        case ICMP_NET_UNKNOWN:
            return "Destination Net Unknown";
            break;
        case ICMP_HOST_UNKNOWN:
            return "Destination Host Unknown";
            break;
        case ICMP_HOST_ISOLATED:
            return "Source Host Isolated";
            break;
        case ICMP_NET_ANO:
            return "Destination Net Prohibited";
            break;
        case ICMP_HOST_ANO:
            return "Destination Host Prohibited";
            break;
        case ICMP_NET_UNR_TOS:
            return "Destination Net Unreachable for Type of Service";
            break;
        case ICMP_HOST_UNR_TOS:
            return "Destination Host Unreachable for Type of Service";
            break;
        case ICMP_PKT_FILTERED:
            return "Packet filtered";
            break;
        case ICMP_PREC_VIOLATION:
            return "Precedence Violation";
            break;
        case ICMP_PREC_CUTOFF:
            return "Precedence Cutoff";
            break;
        default:
            return "Dest Unreachable, Bad Code";
            break;
        }
        break;
    case ICMP_SOURCE_QUENCH:
        return "Source Quench";
        break;
    case ICMP_REDIRECT:
        switch(code) {
        case ICMP_REDIR_NET:
            return "Redirect Network";
            break;
        case ICMP_REDIR_HOST:
            return "Redirect Host";
            break;
        case ICMP_REDIR_NETTOS:
            return "Redirect Type of Service and Network";
            break;
        case ICMP_REDIR_HOSTTOS:
            return "Redirect Type of Service and Host";
            break;
        default:
            return "Redirect, Bad Code";
            break;
        }
        break;
    case ICMP_TIME_EXCEEDED:
        switch(code) {
        case ICMP_TTL_EXPIRED:
            return "TTL expired";
            break;
        case ICMP_REASSEMBLY_TIME_EXCEEDED:
            return "Reassembly time exceeded";
            break;
        default:
            return "Time exceeded, Bad Code";
            break;
        }
        break;
    case ICMP_PARAMETER_PROBLEM:
        switch(code) {
        case ICMP_BAD_OPTION:
            return "Bad option";
            break;
        case ICMP_BAD_LENGTH:
            return "Bad length";
            break;
        case ICMP_BAD_VALUE:
            return "Bad value";
            break;
        default:
            return "Parameter problem, Bad Code";
            break;
        }
        break;
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
    struct ft_icmp_hdr *icmp = get_icmp_header_format(buffer);
    return (struct timeval *)((uint8_t *)icmp + ICMP_HDR_SIZE);
}

void *skip_ip_header(void *buffer)
{
    struct ft_ipv4_hdr *ip = (struct ft_ipv4_hdr *)buffer;
    unsigned int ihl = ip->ver_ihl & 0x0Fu;

    return (void *)((uint8_t *)buffer + ihl * 4);
}