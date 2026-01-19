#include "blocktimer.h"
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

MapResult fetch_addresses(Sarray *domains) {
    struct addrinfo hints, *response, *r_ptr;
    hints = (struct addrinfo) { 0 };
    hints.ai_family = AF_UNSPEC;

    char ipstr_buf[INET6_ADDRSTRLEN];
    MapResult mapres = { 0 };
    mapres.mapresult.status = OK_MAP;
    mapres.mapresult.map = hashy_create(sizeof response->ai_family);

    for (slice_index i = 0; i != sarray_size(domains); ++i) {
        char *domain = sarray_get(domains, i);

        int status = getaddrinfo(domain, 0, &hints, &response);

        if (status != 0) {
            if (status == EAI_AGAIN) {
                mapres.result.status = ERROR_ADDRINFO_TEMPORARY;
                mapres.result.comment = gai_strerror(status);
                break;
            }

            mapres.result.status = ERROR_ADDRINFO;
            mapres.result.comment = gai_strerror(status);
            break;
        }

        for (r_ptr = response; r_ptr; r_ptr = r_ptr->ai_next) {
            if (r_ptr->ai_family == AF_INET) {
                struct sockaddr_in *ipv4 =
                    (struct sockaddr_in *) r_ptr->ai_addr;
                inet_ntop(r_ptr->ai_family, &ipv4->sin_addr, ipstr_buf,
                          INET_ADDRSTRLEN);

                hashy_set(&mapres.mapresult.map, ipstr_buf,
                          &r_ptr->ai_family);
            } else {
                struct sockaddr_in *ipv6 =
                    (struct sockaddr_in *) r_ptr->ai_addr;
                inet_ntop(r_ptr->ai_family, &ipv6->sin_addr, ipstr_buf,
                          INET6_ADDRSTRLEN);

                hashy_set(&mapres.mapresult.map, ipstr_buf,
                          &r_ptr->ai_family);
            }
        }

        freeaddrinfo(response);
    }

    return mapres;
}
