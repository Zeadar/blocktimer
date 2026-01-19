#pragma once
#include "libmemhandle/libmemhandle.h"

enum status {
    OK_GENERIC,
    OK_SLICE,
    OK_MAP,
    OK_INT,
    OK_TYPE_START,
    OK_TYPE_STOP,
    OK_TYPE_DOMAIN,
    ERROR_ADDRINFO_TEMPORARY,
    ERROR_ADDRINFO,
    ERROR_DBUS_CONNECTION,
    ERROR_DBUS_MATCH,
    ERROR_DBUS_START,
    ERROR_DBUS_WAIT,
    ERROR_DBUS_PARSE,
    ERROR_CONF_FILE_NOT_FOUND,
    ERROR_CONF_PARSE,
    ERROR_CONF_PARSE_TIME,
};

struct result {
    enum status status;
    const char *comment;
};

struct intresult {
    enum status status;
    int num;
};

struct mapresult {
    enum status status;
    Map map;
};

struct sliceresult {
    enum status status;
    Slice slice;
};

typedef union{
    enum status status;
    struct result result;
    struct intresult intresult;
} IntResult;

typedef union {
    enum status status;
    struct result result;
    struct sliceresult sliceresult;
} SliceResult;

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
SliceResult parse_config();
void handle_errors(const struct result *result, enum status expect);
