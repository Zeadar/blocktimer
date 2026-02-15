#include <string.h>
#include <time.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "libmemhandle/libmemhandle.h"
#include "blocktimer.h"

#define LOCAL_CONF_NAME "./block.conf"
#define FULL_PATH_NAME "/etc/blocktimer/block.conf"

#define START_EXPR "start="
#define STOP_EXPR "stop="
#define DOMAIN_EXPR "domain="
#define NEWBLOCK_EXPR "[block]"

#define NOBLOCK_COMMENT ""

static ptrdiff_t strip_fluff(char *line) {
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

static struct result get_type(char **buf) {
    char *buffer = *buf;
    unsigned len;
    struct result r = { 0 };

    if (strncmp(buffer, START_EXPR, len = strlen(START_EXPR)) == 0) {
        r.status = OK_TYPE_START;
        *buf = buffer + len;
    } else if (strncmp(buffer, STOP_EXPR, len = strlen(STOP_EXPR)) == 0) {
        r.status = OK_TYPE_STOP;
        *buf = buffer + len;
    } else if (strncmp(buffer, DOMAIN_EXPR, len = strlen(DOMAIN_EXPR)) ==
               0) {
        r.status = OK_TYPE_DOMAIN;
        *buf = buffer + len;
    } else if (strncmp(buffer, NEWBLOCK_EXPR, len = strlen(NEWBLOCK_EXPR))
               == 0) {
        r.status = OK_TYPE_NEWBLOCK;
    } else {
        r.status = ERROR_CONF_PARSE;
        char *err = malloc(strlen(buffer) + 50);
        sprintf(err, "Unrecognized thingie [%s]\n", buffer);
        r.comment = err;
    }

    return r;
}

#define ERROR_TIME_PARSE_MSG "Expected time format HH:MM 24 hour"
#define ERROR_TIME_PARSE_NONUMBER "Contains non-numericals."
static IntResult parse_time(const char *buf) {
    char splitbuf[6];
    IntResult ir = { 0 };
    ir.status = OK_INT;

    if (strlen(buf) != 5) {
        ir.result.status = ERROR_CONF_PARSE_TIME;
        ir.result.comment = ERROR_TIME_PARSE_MSG;
        return ir;
    }

    if (buf[2] != ':') {
        ir.result.status = ERROR_CONF_PARSE_TIME;
        ir.result.comment = ERROR_TIME_PARSE_MSG;
        return ir;
    }

    splitbuf[0] = buf[0];
    splitbuf[1] = buf[1];
    splitbuf[2] = '\0';
    splitbuf[3] = buf[3];
    splitbuf[4] = buf[4];
    splitbuf[5] = '\0';

    char *hstr = splitbuf;
    char *mstr = splitbuf + 3;

    for (char *c = hstr; *c != '\0'; ++c) {
        if (*c < '0' || *c > '9') {
            ir.result.status = ERROR_CONF_PARSE_TIME;
            ir.result.comment = ERROR_TIME_PARSE_NONUMBER;
            return ir;
        }
    }

    for (char *c = mstr; *c != '\0'; ++c) {
        if (*c < '0' || *c > '9') {
            ir.result.status = ERROR_CONF_PARSE_TIME;
            ir.result.comment = ERROR_TIME_PARSE_NONUMBER;
            return ir;
        }
    }

    ir.intresult.num = 0;
    ir.intresult.num += 60 * atoi(mstr);
    ir.intresult.num += 60 * 60 * atoi(hstr);

    return ir;
}

SliceResult parse_config() {
    FILE *config = fopen(LOCAL_CONF_NAME, "r");
    if (config == 0)
        config = fopen(FULL_PATH_NAME, "r");

    ptrdiff_t n;
    size_t buf_size = 4096;
    char *buf = malloc(buf_size);
    SliceResult sr = { 0 };
    sr.status = OK_SLICE;
    sr.sliceresult.slice = slice_new(struct block_unit);

    if (config == 0) {
        sr.result.status = ERROR_CONF_FILE_NOT_FOUND;
        sprintf(buf, "could not find %s or %s\n", LOCAL_CONF_NAME,
                FULL_PATH_NAME);
        sr.result.comment = buf;
        return sr;
    }
    struct block_unit *blocklist = 0;

    while ((n = getline(&buf, &buf_size, config)) != EOF) {
        if ((n = strip_fluff(buf) == 0))
            continue;

        char *temp_buf = buf;
        IntResult ir;
        struct result r = get_type(&temp_buf);  // get_type will change buf

        if (r.status == OK_TYPE_NEWBLOCK) {
            blocklist = slice_allocate(&sr.sliceresult.slice);
            *blocklist = (const struct block_unit) { 0 };
            blocklist->domains = sarray_create();
            continue;
        }

        if (!blocklist) {
            r.status = ERROR_GENERIC;
            r.comment =
                "Need at least one [block] line before anything else in config\n";
            return (SliceResult) r;
        }

        switch (r.status) {
        case OK_TYPE_START:
            ir = parse_time(temp_buf);
            handle_errors((struct result *) &ir, OK_INT);
            blocklist->start = ir.intresult.num;
            break;
        case OK_TYPE_STOP:
            ir = parse_time(temp_buf);
            handle_errors((struct result *) &ir, OK_INT);
            blocklist->stop = ir.intresult.num;
            break;
        case OK_TYPE_DOMAIN:
            sarray_push(&blocklist->domains, temp_buf);
            break;
        default:
            sr = (SliceResult) r;
            return sr;
        }
    }

    for (slice_index si = 0; si != slice_size(&sr.sliceresult.slice); ++si) {
        struct block_unit *bu = slice_get_ptr(&sr.sliceresult.slice, si);
        if (sarray_size(&bu->domains) == 0) {
            sr.result.status = ERROR_GENERIC;
            sr.result.comment = "[block]s needs at least one domain each";
            return sr;
        }
    }

    fclose(config);
    free(buf);

    return sr;
}
