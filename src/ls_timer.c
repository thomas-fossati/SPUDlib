/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <assert.h>
#include <sys/time.h>

#include "ls_mem.h"
#include "ls_timer.h"

struct _ls_timer
{
    ls_timer_func cb;
    const void *context;
    struct timeval tv;
};

const static struct timeval epoch = {0, 0};

LS_API bool ls_timer_create(const struct timeval *actual,
                            ls_timer_func         cb,
                            const void           *context,
                            ls_timer            **tim,
                            ls_err               *err)
{
    ls_timer *ret = NULL;
    assert(actual);
    assert(cb);
    assert(tim);

    ret = ls_data_calloc(1, sizeof(ls_timer));
    if (ret == NULL) {
        LS_ERROR(err, LS_ERR_NO_MEMORY);
        return false;
    }

    ret->cb      = cb;
    ret->context = context;
    ret->tv      = *actual;
    *tim         = ret;
    return true;
}

LS_API bool ls_timer_create_ms(const struct timeval *now,
                               unsigned long         ms,
                               ls_timer_func         cb,
                               const void           *context,
                               ls_timer            **tim,
                               ls_err               *err)
{
    struct timeval delta = {ms / 1000, ms % 1000};
    struct timeval then;
    assert(now);
    timeradd(now, &delta, &then);
    return ls_timer_create(&then, cb, context, tim, err);
}

LS_API void ls_timer_destroy(ls_timer *tim)
{
    assert(tim);
    ls_data_free(tim);
}

LS_API void ls_timer_cancel(ls_timer *tim)
{
    assert(tim);
    tim->tv = epoch;
}

LS_API bool ls_timer_is_cancelled(ls_timer *tim)
{
    assert(tim);
    return timercmp(&tim->tv, &epoch, ==);
}

LS_API bool ls_timer_less(ls_timer *a, ls_timer *b)
{
    assert(a);
    assert(b);
    return timercmp(&a->tv, &b->tv, <);
}
