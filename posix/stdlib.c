/*
 *  ntloader  --  Microsoft Windows NT6+ loader
 *  Copyright (C) 2025  A1ive.
 *
 *  ntloader is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, either version 3 of the License,
 *  or (at your option) any later version.
 *
 *  ntloader is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ntloader.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "ntloader.h"
#include "pmapi.h"

void *calloc (size_t nmemb, size_t size)
{
    void *ret;
    size_t sz = 0;

    if (safe_mul (nmemb, size, &sz))
        die ("overflow is detected");

    ret = pm->_malloc (sz);
    if (!ret)
        return NULL;

    memset (ret, 0, sz);
    return ret;
}

void *malloc (size_t size)
{
    return pm->_malloc (size);
}

void *zalloc (size_t size)
{
    void *ret;

    ret = pm->_malloc (size);
    if (ret)
        memset (ret, 0, size);

    return ret;
}

void free (void *ptr)
{
    pm->_free (ptr);
}
