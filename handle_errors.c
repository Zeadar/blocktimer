#include "blocktimer.h"
#include <stdio.h>
#include <stdlib.h>

#define ERROR_CODE_UNCOVERED 1
#define ERROR_CODE_ADDRINFO 2
#define ERROR_CODE_DBUS 3
#define ERROR_CODE_CONF 4

void handle_errors(const struct result *result, enum status expect) {
    if (result->status == expect)
        return;

    fprintf(stderr, "Expected status %d but got %d\n", expect,
            result->status);

    switch (result->status) {
    case ERROR_ADDRINFO_TEMPORARY:
        fprintf(stderr, "%s\n", result->comment);
        // No exit. Error is not fatal. Handled elsewhere.
        break;
    case ERROR_ADDRINFO:
        fprintf(stderr, "Error fetching addresses:\n%s\n",
                result->comment);
        exit(ERROR_CODE_ADDRINFO);
    case ERROR_DBUS_CONNECTION:
        fprintf(stderr, "DBUS connection failed:\n%s\n", result->comment);
        exit(ERROR_CODE_DBUS);
    case ERROR_DBUS_MATCH:
        fprintf(stderr, "DBUS failed to add match:\n%s\n",
                result->comment);
        exit(ERROR_CODE_DBUS);
    case ERROR_DBUS_START:
        fprintf(stderr, "DBUS failed to start:\n%s\n", result->comment);
        exit(ERROR_CODE_DBUS);
    case ERROR_DBUS_WAIT:
        fprintf(stderr, "DBUS wait for signal failed:\n%s\n",
                result->comment);
        exit(ERROR_CODE_DBUS);
    case ERROR_DBUS_PARSE:
        fprintf(stderr, "DBUS signal parsing failed:\n%s\n",
                result->comment);
        exit(ERROR_CODE_DBUS);
    case ERROR_CONF_FILE_NOT_FOUND:
        fprintf(stderr, "Could not fine config file:\n%s\n",
                result->comment);
        exit(ERROR_CODE_CONF);
    case ERROR_CONF_PARSE:
        fprintf(stderr, "File format wrong:\n%s\n", result->comment);
        exit(ERROR_CODE_CONF);
    case ERROR_CONF_PARSE_TIME:
        fprintf(stderr, "Time format wrong:\n%s\n", result->comment);
        exit(ERROR_CODE_CONF);
    default:
        fprintf(stderr, "Status not covered %d\n", result->status);
        exit(ERROR_CODE_UNCOVERED);
    }
}
