#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "blocktimer.h"
#include "libmemhandle/libmemhandle.h"

#define IPV4 "iptables"
#define IPV6 "ip6tables"

static const char f_check_rule[] = "%s -C OUTPUT -d %s -j REJECT 2>/dev/null";
static const char f_add_rule[] = "%s -A OUTPUT -d %s -j REJECT";
static const char f_remove_rule[] = "%s -D OUTPUT -d %s -j REJECT";

extern int is_not_root;

static void add_addr(char *k, void *v) {
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
        printf("%s\n", add);
        return;
    }

    ret = system(check);
    if (ret != 0)
        ret = system(add);
    if (ret != 0)
        exit(ret);
}

static void del_addr(char *k, void *v) {
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
        printf("%s\n", remove);
        return;
    }

    ret = system(check);
    if (ret == 0)
        ret = system(remove);
    if (ret != 0)
        exit(ret);
}

struct result add(struct event_unit *eu) {
    MapResult mr = { 0 };

    if (eu->addresses.map != 0) {       // addresses is initialized
        mr.status = OK_GENERIC;
        return mr.result;
    }

    mr = fetch_addresses(&eu->block_unit->domains);
    if (mr.status != OK_MAP)
        return mr.result;

    eu->addresses = mr.mapresult.map;

    hashy_foreach(&eu->addresses, add_addr);

    mr.status = OK_GENERIC;
    return mr.result;
}

void del(struct event_unit *eu) {
    if (eu->addresses.map == 0)
        return;

    hashy_foreach(&eu->addresses, del_addr);
    hashy_destroy(&eu->addresses);
}
