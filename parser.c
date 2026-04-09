#include "header.h"

int parse_args(int argc, char **argv, struct ping_options *options, char **hostname_out)
{
    int i;
    char *argument;

    if (!argv || !options || !hostname_out || argc < 1)
        return (-1);
    memset(options, 0, sizeof(*options));
    *hostname_out = NULL;
    if (argc == 1)
        options->help = true;
    for (i = 1; i < argc; i++) {
        argument = argv[i];
        if (!argument)
            return (fprintf(stderr, "ft_ping: invalid argument\n"), 1);
        if (argument[0] != '-') {
            if (*hostname_out)
                return (fprintf(stderr, "ft_ping: extra host: %s\n", argument), 1);
            *hostname_out = argument;
            continue;
        }
        if (!strcmp(argument, "-h") || !strcmp(argument, "--help")) options->help = true;
        else if (!strcmp(argument, "-q") || !strcmp(argument, "--quiet")) options->quiet = true;
        else if (!strcmp(argument, "-v") || !strcmp(argument, "--verbose")) options->verbose = true;
        else return (fprintf(stderr, "ft_ping: unknown option: %s\n", argument), 1);
    }
    if (!options->help && (!*hostname_out || !**hostname_out))
        return (fprintf(stderr, "ft_ping: missing host\n"), 1);
    return (0);
}
