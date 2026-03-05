#include "blocktimer.h"
#include "libmemhandle/libmemhandle.h"
#include <stdio.h>
#include <sys/socket.h>

static const char ipv4[] = "iptables";
static const char ipv6[] = "ip6tables";

// static const char f_check_rule[] =
//     "%s -C OUTPUT -d %s -j REJECT 2>/dev/null";
// iptables -A blocktimer -d IP -j REJECT
static const char f_add_rule[] = "%s -A blocktimer -d %s -j REJECT";
static const char f_remove_rule[] = "%s -D blocktimer -d %s -j REJECT";

extern pthread_mutex_t addr_lock;

static void add_static_addresses(Map *map, const Sarray *addresses, int family) {
  for (slice_index si = 0; si != sarray_size(addresses); ++si) {
    char *ip = sarray_get(addresses, si);
    hashy_set(map, ip, &family);
  }
}

static void add_addr(char *k, void *v) {
  // char check[1024];
  char add[1024];
  int ip_type = *(int *)v;

  if (ip_type == AF_INET) {
    // sprintf(check, f_check_rule, ipv4, k);
    sprintf(add, f_add_rule, ipv4, k);
  } else {
    // sprintf(check, f_check_rule, ipv6, k);
    sprintf(add, f_add_rule, ipv6, k);
  }

  command_log(add);
}

static void del_addr(char *k, void *v) {
  // char check[1024];
  char remove[1024];
  int ip_type = *(int *)v;

  if (ip_type == AF_INET) {
    // sprintf(check, f_check_rule, ipv4, k);
    sprintf(remove, f_remove_rule, ipv4, k);
  } else {
    // sprintf(check, f_check_rule, ipv6, k);
    sprintf(remove, f_remove_rule, ipv6, k);
  }

  command_log(remove);
}

struct result add(struct event_unit *eu) {
  MapResult mr = {0};

  if (eu->addresses.map != 0) { // addresses is initialized
    mr.status = OK_GENERIC;
    return mr.result;
  }

  pthread_mutex_lock(&addr_lock);
  mr = fetch_addresses(&eu->block_unit->domains);
  pthread_mutex_unlock(&addr_lock);

  if (mr.status != OK_MAP)
    return mr.result;

  pthread_mutex_lock(&addr_lock);
  eu->addresses = mr.mapresult.map;
  add_static_addresses(&eu->addresses, &eu->block_unit->ipv4, AF_INET);
  add_static_addresses(&eu->addresses, &eu->block_unit->ipv6, AF_INET6);
  hashy_foreach(&eu->addresses, add_addr);
  pthread_mutex_unlock(&addr_lock);

  mr.status = OK_GENERIC;
  return mr.result;
}

void del(struct event_unit *eu) {
  if (eu->addresses.map == 0)
    return;

  pthread_mutex_lock(&addr_lock);
  hashy_foreach(&eu->addresses, del_addr);
  hashy_destroy(&eu->addresses);
  pthread_mutex_unlock(&addr_lock);
}
