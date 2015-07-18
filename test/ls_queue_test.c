/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <string.h>
#include <stdlib.h>

#include "../src/ls_queue.h"
#include "test_utils.h"

CTEST(ls_queue, create)
{
    ls_queue *q;
    ls_err err;
    ASSERT_TRUE(ls_queue_create(&q, NULL, &err));
    ls_queue_destroy(q);
}

CTEST(ls_queue, enq)
{
    ls_queue *q;
    ls_err err;
    char *val = "VALUE";

    ASSERT_TRUE(ls_queue_create(&q, NULL, &err));
    ASSERT_TRUE(ls_queue_enq(q, val, &err));
    ASSERT_STR(ls_queue_deq(q), "VALUE");
    ls_queue_destroy(q);
}

static void cleaner(ls_queue *q, void *data)
{
    UNUSED_PARAM(q);
    ls_data_free(data);
}

CTEST(ls_queue, cleanup)
{
    ls_queue *q;
    ls_err err;
    int i;

    ASSERT_TRUE(ls_queue_create(&q, cleaner, &err));
    for (i=0; i<10; i++) {
        ASSERT_TRUE(ls_queue_enq(q, ls_data_malloc(1), &err));
    }
    ls_queue_destroy(q);
}