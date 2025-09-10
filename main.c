#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>

const char *domain = "youtube.com";

int main() {
    struct hostent *he;
    struct in_addr **addr_list;

    if ((he = gethostbyname(domain)) == 0) {
        return 1;
    }

    addr_list = (struct in_addr **) he->h_addr_list;
    printf("%lu\n", sizeof addr_list);
    printf("%s\n", he->h_name);

    for (int i = 0; i != he->h_length; ++i) {
        printf("%s\n", inet_ntoa(*addr_list[i]));
    }
}
