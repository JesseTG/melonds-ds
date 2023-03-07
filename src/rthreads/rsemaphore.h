/*
  Copyright 2005 Allen B. Downey

    This file contains an example program from The Little Book of
    Semaphores, available from Green Tea Press, greenteapress.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, see http://www.gnu.org/licenses/gpl.html
    or write to the Free Software Foundation, Inc., 51 Franklin St,
    Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __LIBRETRO_SDK_SEMAPHORE_H
#define __LIBRETRO_SDK_SEMAPHORE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ssem ssem_t;

/**
 * ssem_create:
 * @value                   : initial value for the semaphore
 *
 * Create a new semaphore.
 *
 * Returns: pointer to new semaphore if successful, otherwise NULL.
 */
ssem_t *ssem_new(int value);

void ssem_free(ssem_t *semaphore);

int ssem_get(ssem_t *semaphore);

void ssem_wait(ssem_t *semaphore);

bool ssem_trywait(ssem_t *semaphore);

void ssem_signal(ssem_t *semaphore);

#ifdef __cplusplus
}
#endif

#endif /* __LIBRETRO_SDK_SEMAPHORE_H */
