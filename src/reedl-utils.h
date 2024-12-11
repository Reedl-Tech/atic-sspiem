#pragma once
#include <stddef.h>

void *reedl_malloc(size_t s);
void reedl_free(void *mem);
int timer_create_set(int *fd_timer, int period_msec, char *dbg_name);
void timer_force_trigger(int fd_timer, char *dbg_name);

