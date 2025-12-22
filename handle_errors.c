#include "blocktimer.h"
#include <stdio.h>
#include <stdlib.h>

#define GENERIC_ERROR_CODE_FOR_THE_MEANTIME 100

void handle_errors(const struct result *result, enum status expect) {
    if (result->status == expect)
        return;

    fprintf(stderr, "Expected status %d but got %d\n", expect,
            result->status);

    switch (result->status) {
    case ERROR_TEMPORARY:
        fprintf(stderr, "%s\n", result->comment);
        // No exit, handled elsewhere 
        break;
    case ERROR_ADDRINFO:
        fprintf(stderr, "Error fetching addresses:\n%s\n",
                result->comment);
        exit(GENERIC_ERROR_CODE_FOR_THE_MEANTIME);
    case ERROR_DBUS_CONNECTION:
        fprintf(stderr, "DBUS connection failed:\n%s\n", result->comment);
        exit(GENERIC_ERROR_CODE_FOR_THE_MEANTIME);
    case ERROR_DBUS_MATCH:
        fprintf(stderr, "DBUS failed to add match:\n%s\n",
                result->comment);
        exit(GENERIC_ERROR_CODE_FOR_THE_MEANTIME);
    case ERROR_DBUS_START:
        fprintf(stderr, "DBUS failed to start:\n%s\n", result->comment);
        exit(GENERIC_ERROR_CODE_FOR_THE_MEANTIME);
    case ERROR_DBUS_WAIT:
        fprintf(stderr, "DBUS wait for signal failed:\n%s\n",
                result->comment);
        exit(GENERIC_ERROR_CODE_FOR_THE_MEANTIME);
    case ERROR_DBUS_PARSE:
        fprintf(stderr, "DBUS signal parsing failed:\n%s\n",
                result->comment);
        exit(GENERIC_ERROR_CODE_FOR_THE_MEANTIME);
    default:
        fprintf(stderr, "Error status not covered %d\n", result->status);
        exit(GENERIC_ERROR_CODE_FOR_THE_MEANTIME);
        break;
    }
}
