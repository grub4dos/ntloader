#ifndef _STDLIB_H
#define _STDLIB_H

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
 * Standard library
 *
 */

extern unsigned long strtoul (const char *nptr, char **endptr, int base);

extern void *memalign (size_t align, size_t size);

extern void *calloc (size_t nmemb, size_t size);
extern void *malloc (size_t size);
extern void *zalloc (size_t size);
extern void free (void *ptr);
extern void *realloc (void *ptr, size_t size);

#endif /* _STDLIB_H */
