#pragma once
#include "libmemhandle/libmemhandle.h"

enum status {
    OK_GENERIC,
    OK_MAP,
    ERROR_TEMPORARY,
    ERROR_ADDRINFO,
    ERROR_DBUS_CONNECTION,
    ERROR_DBUS_MATCH,
    ERROR_DBUS_START,
    ERROR_DBUS_WAIT,
    ERROR_DBUS_PARSE,
};

struct result {
    enum status status;
    const char *comment;
};

struct mapresult {
    enum status status;
    Map map;
};

typedef union {
    enum status status;
    struct result result;
    struct mapresult mapresult;
} MapResult;

typedef struct {
    Sarray domains;
    int start;
    int stop;
    int days;
} BlockUnit;

MapResult fetch_addresses(Sarray *domains);
struct result wait_for_wakeup();
Slice parse_config();
void handle_errors(const struct result *result, enum status expect);
