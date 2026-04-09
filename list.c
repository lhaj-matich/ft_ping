#include "header.h"

struct rtt_node *create_new_rtt_node(struct packet_data *pd, void *buffer)
{
    struct rtt_node *head = pd->rtt_head;
    struct rtt_node *new_elemt;
    struct timeval *t_packet_send;
    struct timeval t_packet_revd;

    t_packet_send = get_sent_time(buffer);
    if (gettimeofday(&t_packet_revd, NULL) == -1)
    {
        fprintf(stderr, "ping: cannot get current time\n");
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
    return new_elemt;
}

void update_min_max_avg(struct packet_data *pd)
{
    struct rtt_node *current;
    long number_of_rtts = 0;
    long total_seconds = 0;
    long total_microseconds = 0;


    current = pd->rtt_head;
    pd->min = &current->rtt;
    pd->max = &current->rtt;

    while (current != NULL)
    {
        if (timecmp(&current->rtt, pd->min) < 0)
        {
            pd->min = &current->rtt;
        }
        if (timecmp(&current->rtt, pd->max) > 0)
        {
            pd->max = &current->rtt;
        }
        total_seconds += current->rtt.tv_sec;
        total_microseconds += current->rtt.tv_usec;
        if (total_microseconds >= 1000000)
        {
            total_seconds++;
            total_microseconds -= 1000000;
        }
        number_of_rtts++;
        current = current->next;
    };
    pd->avg.tv_sec = total_seconds / number_of_rtts;
    pd->avg.tv_usec = total_microseconds / number_of_rtts;
}

void calculate_stddev(struct packet_data *pd)
{
    if (pd->seccesfully_received == 0){
        pd->std_dev = 0.0;
        return;
    }

    double avg = pd->avg.tv_sec * 1000.0 + pd->avg.tv_usec / 1000.0;

    double variance = 0.0;
    struct rtt_node *current = pd->rtt_head;
    while (current != NULL)
    {
        double rtt = current->rtt.tv_sec * 1000.0 + current->rtt.tv_usec / 1000.0;
        double diff = rtt - avg;
        variance += diff * diff;
        current = current->next;
    }
    variance /= pd->seccesfully_received;
    pd->std_dev = sqrt(variance);
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

struct rtt_node *get_last_rtt_node(struct packet_data *pd)
{
    if (pd->rtt_tail != NULL)
    {
        return pd->rtt_tail;
    }
    return NULL;
}