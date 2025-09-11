#include "libmemhandle/libmemhandle.h"
#include "blocktimer.h"
#include <stdio.h>

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

    Map addresses = fetch_addresses(&domains);

    hashy_foreach(&addresses, handle_addrs);
    hashy_destroy(&addresses);
    sarray_destroy(&domains);
}
