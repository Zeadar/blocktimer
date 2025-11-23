#pragma once
#include "libmemhandle/libmemhandle.h"

Map fetch_addresses(Sarray *domains, int *temp_error);
int wait_for_wakeup();
