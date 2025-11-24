#include <time.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "libmemhandle/libmemhandle.h"
#include "blocktimer.h"

#define CONF_NAME "block.conf"

ptrdiff_t strip_fluff(char *line) {
    char *read = line, *write = line;

    while (*read != '\0') {
        if (*read == ' ' || *read == '\t' || *read == '\n') {
            ++read;
            continue;
        }

        if (*read == '#') {
            break;
        }

        *write = *read;

        ++write;
        ++read;
    }

    *write = '\0';
    return write - line;
}

struct tm extract_time(FILE *config, char *buf, size_t *buf_size) {
    ssize_t n;
    struct tm extracted_time = { 0 };
    // scrolling through comments and blank lines
    while ((n = getline(&buf, buf_size, config)) != EOF) {
        if ((n = strip_fluff(buf)) != 0) {
            break;
        }
    }

    if (n != 5) {
        fprintf(stderr,
                "\"%s\" expected 24 hour format hh:mm (%td)\n", buf, n);
        exit(2);
    }

    if (buf[2] != ':') {
        fprintf(stderr,
                "\"%s\" expected 24 hour format hh:mm (%td)\n", buf, n);
        exit(2);
    }

    buf[2] = '\0';
    char *minute_buf = buf + 3;

    for (char *c = buf; *c != '\0'; ++c) {
        if (!isalnum(*c)) {
            fprintf(stderr, "Could not parse \"%s\" as a numbers\n", buf);
            exit(2);
        }
    }

    for (char *c = minute_buf; *c != '\0'; ++c) {
        if (!isalnum(*c)) {
            fprintf(stderr, "Could not parse \"%s\" as a number\n",
                    minute_buf);
            exit(2);
        }
    }

    extracted_time.tm_hour = atoi(buf);
    extracted_time.tm_min = atoi(minute_buf);

    return extracted_time;      // returns 56 bytes
}

Slice parse_config() {
    FILE *config = fopen(CONF_NAME, "r");
    ptrdiff_t n;
    size_t buf_size = 4096;
    char *buf = malloc(buf_size);
    Slice block_units = slice_new(BlockUnit);
    BlockUnit *blocklist = slice_allocate(&block_units);

    if (config == 0) {
        fprintf(stderr, "could not find %s\n", CONF_NAME);
        exit(1);
    }

    blocklist->domains = sarray_create();
    blocklist->start = extract_time(config, buf, &buf_size);
    blocklist->stop = extract_time(config, buf, &buf_size);

    while ((n = getline(&buf, &buf_size, config)) != EOF) {
        n = strip_fluff(buf);

        if (n == 0)
            continue;

        sarray_push(&blocklist->domains, buf);
    }

    fclose(config);
    free(buf);

    return block_units;
}
