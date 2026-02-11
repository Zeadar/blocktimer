#include "libmemhandle/libmemhandle.h"
#include "blocktimer.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/socket.h>

#define NOT_ROOT_WARNING "Not running as root. iptables commands that would be run will be printed instead\n"
#define WAIT_ON_FAIL_TIME_SEC 10

#define IPV4 "iptables"
#define IPV6 "ip6tables"

const char f_check_rule[] = "%s -C OUTPUT -d %s -j REJECT 2>/dev/null";
const char f_add_rule[] = "%s -A OUTPUT -d %s -j REJECT";
const char f_remove_rule[] = "%s -D OUTPUT -d %s -j REJECT";

int is_not_root;
Slice events;

void destroy_block_events(void *eu) {
    struct event_unit *event_unit = eu;
    hashy_destroy(&event_unit->addresses);
}

int by_time(const void *a, const void *b) {
    const struct event_unit *a_event = a;
    const struct event_unit *b_event = b;
    return a_event->time - b_event->time;
}

void add_addr(char *k, void *v) {
    char check[1024];
    char add[1024];
    int ret;
    int ip_type = *(int *) v;

    if (ip_type == AF_INET) {
        sprintf(check, f_check_rule, IPV4, k);
        sprintf(add, f_add_rule, IPV4, k);
    } else {
        sprintf(check, f_check_rule, IPV6, k);
        sprintf(add, f_add_rule, IPV6, k);
    }

    if (is_not_root) {
        printf("%s\n", check);
        printf("%s\n", add);
        return;
    }

    ret = system(check);
    if (ret != 0)
        ret = system(add);
    if (ret != 0)
        exit(ret);
}

void del_addr(char *k, void *v) {
    char check[1024];
    char remove[1024];
    int ret;
    int ip_type = *(int *) v;

    if (ip_type == AF_INET) {
        sprintf(check, f_check_rule, IPV4, k);
        sprintf(remove, f_remove_rule, IPV4, k);
    } else {
        sprintf(check, f_check_rule, IPV6, k);
        sprintf(remove, f_remove_rule, IPV6, k);
    }

    if (is_not_root) {
        printf("%s\n", check);
        printf("%s\n", remove);
        return;
    }

    ret = system(check);
    if (ret == 0)
        ret = system(remove);
    if (ret != 0)
        exit(ret);
}

void waiter(void *eu) {
    struct event_unit *event_unit = eu;
    struct tm today = { 0 };
    time_t now_epoch = time(0);
    localtime_r(&now_epoch, &today);
    int now = today.tm_hour * 3600 + today.tm_min * 60 + today.tm_sec;

    // previously: 0
    // in future: n
    sleep(event_unit->time < now ? 0 : event_unit->time - now);

    // TODO: skip if days

    if (event_unit->type == START) {
        hashy_foreach(&event_unit->addresses, add_addr);
    } else {
        hashy_foreach(&event_unit->addresses, del_addr);
    }
}

void translate_block_to_event(void *bu) {
    struct block_unit *block_unit = bu;
    MapResult mr = fetch_addresses(&block_unit->domains);
    handle_errors((const struct result *) &mr, OK_MAP);
    if (mr.status == ERROR_ADDRINFO_TEMPORARY) {
        hashy_destroy(&mr.mapresult.map);
        fprintf(stderr,
                "Temporary failure detected. Will try again in %d seconds...\n",
                WAIT_ON_FAIL_TIME_SEC);
        sleep(WAIT_ON_FAIL_TIME_SEC);
        translate_block_to_event(bu);
        return;
    }

    struct event_unit *e = slice_allocate(&events);
    e->addresses = mr.mapresult.map;
    e->days = block_unit->days;
    e->time = block_unit->start;
    e->type = START;

    e = slice_allocate(&events);
    e->addresses = mr.mapresult.map;
    e->days = block_unit->days;
    e->time = block_unit->stop;
    e->type = STOP;
}

void *scheduler(void *d) {
    struct thrd_data *data = d;
    slice_foreach(&events, destroy_block_events);
    events.head = events.begin; // reset event array

    slice_foreach(data->domains, translate_block_to_event);
    slice_qsort(&events, by_time);

    slice_foreach(&events, waiter);

    // TODO when all events are waited out sleep until the next event tomorrow
    // TODO start a new thread with scheduler and kill this one
    // TODO struct thrd_data has a pointer to the pthread_t

    pthread_exit(0);
}

void clear_firewall() {
    for (slice_index si = 0; slice_size(&events); ++si) {
        struct event_unit *eu = slice_get_ptr(&events, si);
        if (eu->type == START)
            hashy_foreach(&eu->addresses, del_addr);
    }
}

int main() {
    is_not_root = geteuid() != 0;
    struct sigaction sa = { 0 };
    pthread_t thrd_id;          // TODO needs mutex
    events = slice_new(struct event_unit);
    struct result wait_result = { 0 };
    SliceResult config = parse_config();
    handle_errors((struct result *) &config, OK_SLICE);
    struct thrd_data data = {
        .thrd_id = &thrd_id,
        .domains = &config.sliceresult.slice,
    };

    if (is_not_root)
        printf(NOT_ROOT_WARNING);

    // sa.sa_handler = exit_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, 0);
    sigaction(SIGTERM, &sa, 0);

    // debug
    for (slice_index si = 0; si != slice_size(&config.sliceresult.slice);
         ++si) {
        struct block_unit *bu =
            slice_get_ptr(&config.sliceresult.slice, si);

        printf("Read domains:\n");

        for (slice_index sai = 0; sai != sarray_size(&bu->domains); ++sai) {
            printf("%s\n", sarray_get(&bu->domains, sai));
        }

        putchar('\n');
    }

    for (;;) {
        pthread_create(&thrd_id, 0, scheduler, &data);
        wait_result = wait_for_wakeup();        // WARNING race condition (very unlikely)
        clear_firewall();       // WARNING scheduler might not have populated Slice events
        handle_errors(&wait_result, OK_GENERIC);
        pthread_cancel(thrd_id);
    }

    // unreachable

    // TODO free on exit
    // slice_foreach(block_units, destroy_block_unit);
    // slice_destroy(block_units);

    return 0;
}
