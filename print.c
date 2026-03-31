#include "header.h"

void print_help()
{
    printf("Usage: ping <host>\n");
    printf("Options:\n");
    printf("  -v, --version\t\tShow version information and exit\n");
    printf("  -q, --quiet\t\tBe quiet (don't print status messages)\n");
    printf("  -h, --help\t\tShow this help message and exit\n");
}

void print_start_info(const struct socket_info *si)
{
    printf("PING %s (%s): %d bytes of data.\n", si->host, si->str_sin_addr, ICMP_PAYLOAD_SIZE);
}