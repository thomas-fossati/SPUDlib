/**
 * \file
 *
 * Copyrights
 *
 * Portions created or assigned to Cisco Systems, Inc. are
 * Copyright (c) 2010 Cisco Systems, Inc.  All Rights Reserved.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "basics.h"
#include "error.h"
#include "str.h"
#include "mem.h"

#include "pool_types.h"

ls_data_malloc_func _malloc_func = malloc;
ls_data_realloc_func _realloc_func = realloc;
ls_data_free_func _free_func = free;

/*
 * malloc and free wrappers, _malloc_fnc checks errors and add cleaners as needed.
 */
static bool _malloc_fnc(ls_pool pool, size_t size, void **ptr, ls_pool_cleaner cleaner, ls_err *err)
{
    void *ret = ls_data_malloc(size);
    if (!ret)
    {
        LS_ERROR(err, LS_ERR_NO_MEMORY);
        return false;
    }
    if (pool && cleaner && !ls_pool_add_cleaner(pool, cleaner, ret, err))
    {
        ls_data_free(ret);
        return false;
    }

    *ptr = ret;
    return true;
}

/*
 * Allocate block from given page. return false if request is too large
 */
bool _page_malloc(_pool_page page, size_t size, void **ptr)
{
    size_t will_use = page->used;

    /* may need word-boundary alignment here... */
    /* ... but keeping it simple until we do */

    /* if request will not fit in page, failure */
    if (size > (page->size - will_use))
    {
        return false;
    }

    *ptr = (uint8_t *)page->block + will_use;
    page->used = will_use + size;
    return true;
}

/* page cleaner*/
void _free_page(void *arg)
{
    _pool_page page = (struct pool_page*)arg;

    ls_data_free(page->block);
    ls_data_free(page);
}
/* Allocate and add a new page to the given pool
   may result in a LS_ERR_NO_MEMORY err,
   incs pool->size as side-effect*/
bool _add_page(ls_pool pool, ls_err* err)
{
    _pool_page page;

    if (!_malloc_fnc(pool, sizeof(struct pool_page), (void *) &page, NULL, err))
    {
        return false;
    }
    if (!_malloc_fnc(pool, pool->page_size, &(page->block), NULL, err))
    {
        ls_data_free(page);
        return false;
    }
    if (!ls_pool_add_cleaner(pool, _free_page, page, err))
    {
        ls_data_free(page->block);
        ls_data_free(page);
        return false;
    }

    page->size  = pool->page_size;
    page->used  = 0;

    page->next = pool->pages;
    pool->pages = page;

    pool->size += pool->page_size;
    return true;
}

/* exported functions */
LS_API void ls_data_set_memory_funcs(ls_data_malloc_func malloc_func,
                                             ls_data_realloc_func realloc_func,
                                             ls_data_free_func free_func)
{
    _malloc_func = (malloc_func) ? malloc_func : malloc;
    _realloc_func = (realloc_func) ? realloc_func : realloc;
    _free_func = (free_func) ? free_func : free;

    // set identical functions in libevent2
    //event_set_mem_functions(_malloc_func, _realloc_func, _free_func);
}

LS_API void ls_data_free(void *ptr)
{
    if (ptr)
    {
        _free_func(ptr);
    }
}

LS_API void *ls_data_malloc(size_t size)
{
    return _malloc_func(size);
}

LS_API void *ls_data_realloc(void *ptr, size_t size)
{
    return _realloc_func(ptr, size);
}

LS_API char *ls_data_strdup(const char  *src)
{
    char   *ret = NULL;
    if (src)
    {
        size_t len = ls_strlen(src);
        ret = ls_data_malloc(len + 1);
        if (!ret)
        {
            return NULL;
        }
        memcpy(ret, src, len + 1);
    }
    return ret;
}

LS_API char *ls_data_strndup(const char  *src,
                                    size_t len)
{
    char   *ret = NULL;
    if (src)
    {
        /* Trim len down to the actual size of the string */
        len = ls_strnlen(src, len);
        ret = ls_data_malloc(len + 1);
        if (!ret)
        {
            return NULL;
        }
        memcpy(ret, src, len);
        ret[len] = '\0';
    }
    return ret;
}

LS_API bool ls_pool_create(size_t size, ls_pool *pool, ls_err *err)
{
    ls_pool ret;

    assert(pool);

    if (!_malloc_fnc(NULL, sizeof(struct _ls_pool_int), (void *) &ret, NULL, err))
    {
        return false;
    }

    ret->cleaners = NULL;
    ret->pages = NULL;
    ret->size = 0;
    ret->page_size = size;

    if (size && !_add_page(ret, err))
    {
        ls_data_free(ret);
        return false;
    }
    *pool = ret;
    return true;
}
LS_API void ls_pool_destroy(ls_pool pool)
{
    struct pool_cleaner_ctx *cur, *next;

    assert(pool);

    cur = pool->cleaners;
    while (cur != NULL)
    {
        (*cur->cleaner)(cur->arg);
        next = cur->next;
        ls_data_free(cur);
        cur = next;
    }

    ls_data_free(pool);
}

LS_API bool ls_pool_add_cleaner(ls_pool pool,
                                        ls_pool_cleaner callback,
                                        void   *arg,
                                        ls_err *err)
{
    _pool_cleaner_ctx ctx;

    assert(pool);
    assert(callback);

    if (!_malloc_fnc(pool, sizeof(struct pool_cleaner_ctx), (void *) &ctx, NULL, err))
    {
        return false;
    }

    ctx->cleaner = callback;
    ctx->arg = arg;

    if (!pool->cleaners)
    {
        pool->tail = ctx;
    }
    ctx->next = pool->cleaners;
    pool->cleaners = ctx;
    return true;
}

LS_API bool ls_pool_malloc(ls_pool pool,
                                   size_t  size,
                                   void    **ptr,
                                   ls_err  *err)
{
    void *ret = NULL;

    assert(pool);
    assert(ptr);

    /* return NULL if size == 0 */
    if (size)
    {
        /* if request is too big for page, just malloc*/
        if (size > pool->page_size)
        {
            if (!_malloc_fnc(pool, size, &ret, ls_data_free, err))
            {
                return false;
            }
            /* "manually" inc pool's size */
            pool->size += size;
        }
        /* try to allocate from current page */
        else if (!_page_malloc(pool->pages, size, &ret))
        {
            if (!_add_page(pool, err))
            {
                return false;
            }
            /* size will fit on an empty page */
            _page_malloc(pool->pages, size, &ret);
        }
    }
    *ptr = ret;
    return true;
}
LS_API bool ls_pool_calloc(ls_pool pool,
                                   size_t num,
                                   size_t size,
                                   void   **ptr,
                                   ls_err *err)
{
    return ls_pool_malloc(pool, num * size, ptr, err);
}
LS_API bool ls_pool_strdup(ls_pool pool,
                                   const char  *src,
                                   char  **cpy,
                                   ls_err *err)
{
    assert(cpy);

    char   *ret = NULL;
    if (src)
    {
        size_t len = ls_strlen(src);
        if (!ls_pool_malloc(pool, len + 1, (void *) &ret, err))
        {
            return false;
        }
        memcpy(ret, src, len + 1);
    }
    *cpy = ret;
    return true;
}
