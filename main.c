#include "libmemhandle/libmemhandle.h"
#include "blocktimer.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define NOT_ROOT_WARNING "Not running as root. Commands that would be run will be printed instead\n"
#define WAIT_ON_FAIL_TIME_SEC 10
#define DAY_SEC (24 * 60 * 60)
#define F_TEMP_FAIL "Temporary failure detected. Will try again in %d seconds...\n"

Slice block_units;
Slice event_units;
int is_not_root;

void *scheduler(void *eu) {
    struct event_unit *event_unit = eu;
    time_t now_epoch;
    int sleep_time;
    int now;
    struct tm today;
    struct result r;

    for (;;) {
        now_epoch = time(0);
        localtime_r(&now_epoch, &today);
        now = today.tm_hour * 3600 + today.tm_min * 60 + today.tm_sec;
        if (event_unit->block_unit->start < event_unit->block_unit->stop) {     // withing the day
            if (event_unit->block_unit->start <= now && event_unit->block_unit->stop > now) {   // on
                r = add(event_unit);
                if (r.status == ERROR_ADDRINFO_TEMPORARY) {
                    fprintf(stderr, F_TEMP_FAIL, WAIT_ON_FAIL_TIME_SEC);
                    sleep(WAIT_ON_FAIL_TIME_SEC);
                    continue;
                }
                handle_errors(&r, OK_GENERIC);
                sleep_time = event_unit->block_unit->stop - now;
            } else if (event_unit->block_unit->stop <= now) {   // off
                del(event_unit);
                sleep_time =
                    (DAY_SEC - now) + event_unit->block_unit->start;
            } else {            // off
                del(event_unit);
                sleep_time = event_unit->block_unit->start - now;
            }
        } else {                // Spanning across days
            if (now < event_unit->block_unit->stop) {
                r = add(event_unit);
                if (r.status == ERROR_ADDRINFO_TEMPORARY) {
                    fprintf(stderr, F_TEMP_FAIL, WAIT_ON_FAIL_TIME_SEC);
                    sleep(WAIT_ON_FAIL_TIME_SEC);
                    continue;
                }
                handle_errors(&r, OK_GENERIC);
                sleep_time = event_unit->block_unit->stop - now;
            } else if (now >= event_unit->block_unit->start) {
                r = add(event_unit);
                if (r.status == ERROR_ADDRINFO_TEMPORARY) {
                    fprintf(stderr, F_TEMP_FAIL, WAIT_ON_FAIL_TIME_SEC);
                    sleep(WAIT_ON_FAIL_TIME_SEC);
                    continue;
                }
                handle_errors(&r, OK_GENERIC);
                sleep_time =
                    (DAY_SEC - now) + event_unit->block_unit->stop;
            } else {
                del(event_unit);
                sleep_time = event_unit->block_unit->start - now;
            }
        }
        // printf("Sleeping for %.2d:%.2d:%.2d\n",
        //        sleep_time / 60 / 60,
        //        (sleep_time / 60) % 60, (sleep_time % 60) % 60);
        sleep(sleep_time);
    }
}

void init_and_link_events_to_blocks(void *bu) {
    struct event_unit *e = slice_allocate(&event_units);
    *e = (const struct event_unit) { 0 };
    e->block_unit = bu;
}

void destroy_domains_in_block_units(void *bu) {
    struct block_unit *block_unit = bu;
    sarray_destroy(&block_unit->domains);
}

void destroy_addresses_in_block_events(void *eu) {
    struct event_unit *event_unit = eu;
    hashy_destroy(&event_unit->addresses);
}

void cancel_threads(void *eu) {
    struct event_unit *event_unit = eu;
    pthread_cancel(event_unit->thread);
}

void create_threads(void *eu) {
    struct event_unit *event_unit = eu;
    pthread_create(&event_unit->thread, 0, scheduler, eu);
}

void exit_handler(int sig) {
    printf("Recieved signal %d, exiting...\n", sig);
    // printf("%ld\n", slice_size(&event_units));
    slice_foreach(&event_units, cancel_threads);
    // clear firewall
    for (slice_index si = 0; si != slice_size(&event_units); ++si) {
        struct event_unit *eu = slice_get_ptr(&event_units, si);
        del(eu);
    }
    slice_foreach(&event_units, destroy_addresses_in_block_events);
    slice_destroy(&event_units);
    slice_foreach(&block_units, destroy_domains_in_block_units);
    slice_destroy(&block_units);
    exit(0);
}

int main() {
    is_not_root = geteuid() != 0;
    struct sigaction sa = { 0 };
    event_units = slice_new(struct event_unit);
    struct result wait_result = { 0 };
    SliceResult config = parse_config();

    if (is_not_root)
        printf(NOT_ROOT_WARNING);

    handle_errors((struct result *) &config, OK_SLICE);
    block_units = config.sliceresult.slice;

    sa.sa_handler = exit_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, 0);
    sigaction(SIGTERM, &sa, 0);

    slice_foreach(&block_units, init_and_link_events_to_blocks);

    // debug
    // for (slice_index si = 0; si != slice_size(&config.sliceresult.slice);
    //      ++si) {
    //     struct block_unit *bu =
    //         slice_get_ptr(&config.sliceresult.slice, si);

    //     printf("Read domains:\n");

    //     for (slice_index sai = 0; sai != sarray_size(&bu->domains); ++sai) {
    //         printf("%s\n", sarray_get(&bu->domains, sai));
    //     }

    //     putchar('\n');
    // }

    for (;;) {
        slice_foreach(&event_units, create_threads);
        wait_result = wait_for_wakeup();
        handle_errors(&wait_result, OK_GENERIC);
        slice_foreach(&event_units, cancel_threads);
    }
}
