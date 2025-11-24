#pragma once
#include "libmemhandle/libmemhandle.h"
#include <time.h>

typedef struct {
    Sarray domains;
    struct tm start;
    struct tm stop;
} BlockUnit;

Map fetch_addresses(Sarray *domains, int *temp_error);
int wait_for_wakeup();
Slice parse_config();
