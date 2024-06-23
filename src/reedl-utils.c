#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <assert.h>
#include <time.h>
#include <errno.h>

#include <sys/timerfd.h>
#include <sys/syslog.h>

#define DBG_LOG_EN 1
#define DBG_LVL LOG_DEBUG
#include "reedl-generic.h"

#include "reedl-utils.h"

int g_alloc_cnt = 0;
int g_alloc_cnt_presc = 0;
int g_free_cnt_presc = 0;

void reedl_free(void *mem)
{
    g_alloc_cnt--;
    if (mem) free(mem);
}

void *reedl_malloc(size_t s)
{
    void *mem;
    g_alloc_cnt++;
    g_alloc_cnt_presc++;
    if (0 == (g_alloc_cnt_presc & 0x3FF)) {
        DBG_INFO("Alloc counter + %d", g_alloc_cnt);
    }

    mem = malloc(s);
    if (!mem) {
        DBG_ERR("Out of memory!");
        assert(0);
        return NULL;
    }
    return mem;
     
}

int timer_create_set(int *fd_timer, int period_msec, char *dbg_name)
{
    int rc;
    struct itimerspec itimer;

    itimer.it_value.tv_sec = 0 / 1000;
    itimer.it_value.tv_nsec = 100 * (1000 * 1000);
    itimer.it_interval.tv_sec = period_msec / 1000;
    itimer.it_interval.tv_nsec = (period_msec % 1000) * (1000 * 1000);

    *fd_timer = timerfd_create(CLOCK_MONOTONIC, 0);
    rc = timerfd_settime(*fd_timer, 0, &itimer, NULL);
    if (rc) {
        DBG_ERR("Can't init %s timer (%d)", dbg_name, errno);
        return -1;
    }
    return 0;
}

void timer_force_trigger(int fd_timer, char *dbg_name)
{
    int rc;
    struct itimerspec itimer;

    rc = timerfd_gettime(fd_timer, &itimer);
    if (rc) {
        DBG_ERR("Can't get %s timer (%d)", dbg_name, errno);
    }

    itimer.it_value.tv_sec = 0;
    itimer.it_value.tv_nsec = 1;

    rc = timerfd_settime(fd_timer, 0, &itimer, NULL);
    if (rc) {
        DBG_ERR("Can't force %s timer (%d)", dbg_name, errno);
    }
}
