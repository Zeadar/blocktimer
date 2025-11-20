#include "libmemhandle/libmemhandle.h"
#include "blocktimer.h"
#include <stdio.h>
#include <threads.h>

void handle_addrs(char *key, void *value) {
    int *addr_type = (int *) value;
    printf("Type: %d, Addr: %s\n", *addr_type, key);
}

int main() {
    Sarray domains = sarray_create();
    sarray_push(&domains, "youtube.com");
    sarray_push(&domains, "www.youtube.com");
    sarray_push(&domains, "kalleponken");
    sarray_push(&domains, "google.com");
    sarray_push(&domains, "www.google.com");

    thrd_t wait_for_wakeup_thrd = { 0 };

    for (;;) {
        Map addresses = fetch_addresses(&domains);
        hashy_foreach(&addresses, handle_addrs);
        hashy_destroy(&addresses);

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

    sarray_destroy(&domains);
}
