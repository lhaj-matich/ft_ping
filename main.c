#include "header.h"

volatile sig_atomic_t pingloop = 1;
volatile sig_atomic_t send_packet = 1;

void handler(int signum)
{
    if (signum == SIGINT)
    {
        pingloop = 0;
    }
    else if (signum == SIGALRM)
    {
        send_packet = 1;
    }
}

int main(int argc, char **argv)
{
    struct socket_info si;
    struct packet_data pd = {0};
    struct ping_options options;
    char *target_host;
    int socket_file_descriptor;
    int parse_status;

    if (!is_user_root()) {
        fprintf(stderr, "ping: you must be root to run this program\n");
        return (1);
    }
    parse_status = parse_args(argc, argv, &options, &target_host);
    if (parse_status == -1) {
        fprintf(stderr, "ping: invalid invocation\n");
        return (1);
    }
    if (parse_status != 0)
        return (parse_status);
    if (options.help)
    {
        print_help();
        return (0);
    }
    signal(SIGINT, handler);
    signal(SIGALRM, handler);
    if (init_socket(&socket_file_descriptor, &si, target_host, IP_TTL_VALUE) == -1)
        return (1);
    print_start_info(&si);
    while (pingloop)
    {
        if (send_packet)
        {
            if (pd.sequence_number > 0 && pd.awaiting_echo_reply && !pd.got_echo_reply)
                print_request_timeout((unsigned)(pd.sequence_number - 1), &options);
            int ret = send_icmp_echo_request(socket_file_descriptor, &si, pd.sequence_number);
            if (ret == -1)
            {
                fprintf(stderr, "ping: cannot send ICMP echo request\n");
                close(socket_file_descriptor);
                return (1);
            }
            pd.awaiting_echo_reply = true;
            pd.got_echo_reply = false;
            pd.sequence_number++;
            send_packet = 0;
            alarm(1);
        }
        receive_icmp_reply(socket_file_descriptor, &pd, &options);
        if (!pingloop)
            break;
        usleep(5000);
    }
    print_end_info(&si, &pd);
    clean_rtts_list(&pd);
    return (0);
}