#include "libmemhandle/libmemhandle.h"
#include "blocktimer.h"
#include <stdio.h>
#include <unistd.h>

#define WAIT_ON_FAIL_TIME_SEC 10

void handle_addrs(char *key, void *value) {
    int *addr_type = (int *) value;
    printf("Type: %d, Addr: %s\n", *addr_type, key);
}

void destroy_block_unit(void *block_unit) {
    BlockUnit *bu = block_unit;
    sarray_destroy(&bu->domains);
}

void print_domains(char *line) {
    printf("%s\n", line);
}

// TODO: replace std thrd with pthread
int main() {
    // thrd_t wait_for_wakeup_thrd = 0;    // Appearently this is an unsigned long
    struct result wait_result = { 0 };
    SliceResult config = parse_config();
    handle_errors((struct result *) &config, OK_SLICE);
    Slice *block_units = &config.sliceresult.slice;
    BlockUnit *current_config = slice_get_ptr(block_units, 0);
    sarray_foreach(&current_config->domains, print_domains);
    printf("start: %d, stop: %d\n", current_config->start,
           current_config->stop);

    for (;;) {
        MapResult mapres = fetch_addresses(&current_config->domains);
        handle_errors((struct result *) &mapres, OK_MAP);

        if (mapres.status == ERROR_ADDRINFO_TEMPORARY) {
            fprintf(stderr,
                    "Temporary failure detected. Will try again in %d seconds...\n",
                    WAIT_ON_FAIL_TIME_SEC);
            sleep(WAIT_ON_FAIL_TIME_SEC);
            continue;
        }

        Map *addresses = &mapres.mapresult.map;
        hashy_foreach(addresses, handle_addrs);
        hashy_destroy(addresses);

        // thrd_create(&wait_for_wakeup_thrd, wait_for_wakeup, 0);
        // thrd_join(wait_for_wakeup_thrd, &wait_result);
        wait_result = wait_for_wakeup();
        // break;                  //temp
        printf("Detected system wake up. Resetting program state.\n");

        handle_errors(&wait_result, OK_GENERIC);
    }

    // unreachable
    slice_foreach(block_units, destroy_block_unit);
    slice_destroy(block_units);

    return 0;
}
