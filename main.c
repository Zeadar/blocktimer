#include "libmemhandle/libmemhandle.h"
#include "blocktimer.h"
#include <stdio.h>
#include <threads.h>

#define WAIT_ON_FAIL_TIME_SEC 10

void handle_addrs(char *key, void *value) {
    int *addr_type = (int *) value;
    printf("Type: %d, Addr: %s\n", *addr_type, key);
}

void print_domains(char *line) {
    printf("%s\n", line);
}

int main() {
    thrd_t wait_for_wakeup_thrd = 0;    // Appearently this is an unsigned long
    int temp_error = 0;
    Slice block_units = parse_config();
    BlockUnit *current_config = slice_get_ptr(&block_units, 0);
    sarray_foreach(&current_config->domains, print_domains);

    for (;;) {
        Map addresses =
            fetch_addresses(&current_config->domains, &temp_error);

        hashy_foreach(&addresses, handle_addrs);
        hashy_destroy(&addresses);

        if (temp_error) {
            fprintf(stderr,
                    "Temporary failure detected. Will try again in %d seconds...\n",
                    WAIT_ON_FAIL_TIME_SEC);
            temp_error = 0;     // Happy I didn't forget this
            struct timespec wait_duration = { 0 };
            wait_duration.tv_sec = WAIT_ON_FAIL_TIME_SEC;
            thrd_sleep(&wait_duration, 0);
            continue;
        }

        thrd_create(&wait_for_wakeup_thrd, wait_for_wakeup, 0);
        int wait_result;
        thrd_join(wait_for_wakeup_thrd, &wait_result);
        printf("Detected system wake up. Resetting program state.\n");

        if (wait_result != 0) {
            fprintf(stderr, "Thread wating for DBus failed, exiting: %d\n",
                    wait_result);
            break;
        }
    }

    // sarray_destroy(domains);
    slice_destroy(&block_units);

    return 0;
}
