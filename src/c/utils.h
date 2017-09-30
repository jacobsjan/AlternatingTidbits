#pragma once

bool is_asleep();
bool should_keep_quiet();
#if defined(PBL_PLATFORM_DIORITE) || defined(PBL_PLATFORM_EMERY) 
bool is_heartrate_available();
#endif
void check_write_status(enum StatusCode status, int key);
void utils_init();
void utils_deinit();