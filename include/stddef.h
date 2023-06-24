#ifndef _STDDEF_H
#define _STDDEF_H

/*
 * Copyright (C) 2012 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/**
 * @file
 *
 * Standard definitions
 *
 */

#include <stdint.h>

#define NULL ( ( void * ) 0 )

#define offsetof( __type, __member ) ( ( size_t ) &( ( __type * ) NULL )->__member )

#define container_of( __ptr, __type, __member ) \
    ( { \
        const typeof ( ( ( __type * ) NULL )->__member ) *__mptr = (__ptr); \
        ( __type * ) ( ( void * ) __mptr - offsetof ( __type, __member ) ); \
    } )

#endif /* _STDDEF_H */
