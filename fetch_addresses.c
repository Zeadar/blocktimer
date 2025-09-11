#include "blocktimer.h"
#include "libmemhandle/libmemhandle.h"
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>

Map fetch_addresses(Sarray *domains) {
    struct addrinfo hints, *response, *r_ptr;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;

    char ipstr_buf[INET6_ADDRSTRLEN];
    Map addresses = hashy_create(sizeof response->ai_family);

    for (slice_index i = 0; i != sarray_size(domains); ++i) {
        char *domain = sarray_get(domains, i);

        int status = getaddrinfo(domain, 0, &hints, &response);
        if (status != 0) {
            fprintf(stderr, "ERROR (%s): %s\n",
                    domain, gai_strerror(status));
            continue;
        }

        for (r_ptr = response; r_ptr; r_ptr = r_ptr->ai_next) {
            if (r_ptr->ai_family == AF_INET) {
                struct sockaddr_in *ipv4 =
                    (struct sockaddr_in *) r_ptr->ai_addr;
                inet_ntop(r_ptr->ai_family, &ipv4->sin_addr, ipstr_buf,
                          INET_ADDRSTRLEN);

                hashy_set(&addresses, ipstr_buf, &r_ptr->ai_family);
            } else {
                struct sockaddr_in *ipv6 =
                    (struct sockaddr_in *) r_ptr->ai_addr;
                inet_ntop(r_ptr->ai_family, &ipv6->sin_addr, ipstr_buf,
                          INET6_ADDRSTRLEN);

                hashy_set(&addresses, ipstr_buf, &r_ptr->ai_family);
            }
        }

        freeaddrinfo(response);
    }

    return addresses;
}
