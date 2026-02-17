#include "blocktimer.h"
#include <stdio.h>
#include <string.h>
#include <systemd/sd-bus.h>

struct result wait_for_wakeup() {
    sd_bus *bus;
    sd_bus_message *message;
    int ret_val;

    struct result result = {
        .status = OK_GENERIC,
        .comment = 0,
    };

    ret_val = sd_bus_default_system(&bus);

    if (ret_val < 0) {
        fprintf(stderr, "DBus connection failed: %s\n",
                strerror(-ret_val));
        result.status = ERROR_DBUS_CONNECTION;
        result.comment = strerror(-ret_val);
        return result;
    }

    ret_val = sd_bus_add_match(bus,
                               NULL,
                               "type='signal',"
                               "sender='org.freedesktop.login1',"
                               "interface='org.freedesktop.login1.Manager',"
                               "member='PrepareForSleep'", 0, 0);

    if (ret_val < 0) {
        sd_bus_close_unref(bus);
        result.status = ERROR_DBUS_MATCH;
        result.comment = strerror(-ret_val);
        return result;
    }

    for (;;) {
        ret_val = sd_bus_process(bus, &message);

        if (ret_val < 0) {
            result.status = ERROR_DBUS_START;
            result.comment = strerror(-ret_val);
            return result;
        }

        if (ret_val == 0) {
            //No message processed, will block until message recieved
            ret_val = sd_bus_wait(bus, ~0);

            if (ret_val < 0) {
                result.status = ERROR_DBUS_WAIT;
                result.comment = strerror(-ret_val);
                return result;
            }
            continue;
        }

        if (!message)
            continue;

        if (sd_bus_message_is_signal
            (message, "org.freedesktop.login1.Manager",
             "PrepareForSleep")) {

            int sleeping = -1;
            ret_val = sd_bus_message_read(message, "b", &sleeping);
            sd_bus_message_unref(message);

            if (ret_val < 0) {
                result.status = ERROR_DBUS_PARSE;
                result.comment = strerror(-ret_val);
                return result;
            }

            if (sleeping == 0) {        // PrepareForSleep returns "False" upon waking up
                break;
            }

        } else {
            sd_bus_message_unref(message);      // Still unref even if message is not signal
        }
    }

    sd_bus_flush_close_unref(bus);

    return result;
}
