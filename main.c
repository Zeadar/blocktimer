#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include "libmemhandle/libmemhandle.h"

void handle_addrs(char *addr) {
    printf("addr: %s\n", addr);
}

const char *domain = "www.youtube.com";

int main() {
    struct hostent *he;
    struct in_addr **addr_list;

    if ((he = gethostbyname(domain)) == 0) {
        return 1;
    }

    addr_list = (struct in_addr **) he->h_addr_list;
    printf("h_name: %s\nh_length: %d\n", he->h_name, he->h_length);

    StrSet addresses = strset_create();

    for (int i = 0; addr_list[i]; ++i) {
        char *str = inet_ntoa(*addr_list[i]);
        printf("adding; %s\n", str);
        strset_set(&addresses, str);
    }

    sarray_foreach(&addresses.strings, handle_addrs);
    strset_destroy(&addresses);
}
