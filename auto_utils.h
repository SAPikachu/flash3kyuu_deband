#pragma once

#include "include/f3kdb.h"

void params_set_defaults(f3kdb_params_t* params);

int params_set_by_string(f3kdb_params_t* params, const char* name, const char* value_string);
