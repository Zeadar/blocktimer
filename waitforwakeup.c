#include <stdio.h>
#include <string.h>
#include <systemd/sd-bus.h>

int wait_for_wakeup() {
    sd_bus *bus;
    sd_bus_message *message;

    int result;

    result = sd_bus_default_system(&bus);

    if (result < 0) {
        fprintf(stderr, "DBus connection failed: %s\n", strerror(-result));
        return 1;
    }

    result = sd_bus_add_match(bus,
                              NULL,
                              "type='signal',"
                              "sender='org.freedesktop.login1',"
                              "interface='org.freedesktop.login1.Manager',"
                              "member='PrepareForSleep'", 0, 0);

    if (result < 0) {
        fprintf(stderr, "Failed to add match: %s\n", strerror(-result));
        sd_bus_close_unref(bus);
        return 2;
    }

    for (;;) {
        result = sd_bus_process(bus, &message);

        if (result < 0) {
            fprintf(stderr, "Failed to start bus process: %s\n",
                    strerror(-result));
            return 3;
        }


        if (result == 0) {
            //No message processed, will block until message recieved
            result = sd_bus_wait(bus, ~0);

            if (result < 0) {
                fprintf(stderr, "Waiting failed: %s\n", strerror(-result));
                return 5;
            }
            continue;
        }

        if (!message)
            continue;

        if (sd_bus_message_is_signal
            (message, "org.freedesktop.login1.Manager",
             "PrepareForSleep")) {

            int sleeping = -1;
            result = sd_bus_message_read(message, "b", &sleeping);
            sd_bus_message_unref(message);

            if (result < 0) {
                fprintf(stderr, "Could not parse message: %s\n",
                        strerror(-result));
                return 4;
            }

            if (sleeping == 0) {        // PrepareForSleep returns "False" upon waking up
                break;
            }

        } else {
            sd_bus_message_unref(message);      // Still unref even if message is not signal
        }
    }

    sd_bus_flush_close_unref(bus);

    return 0;
}
