/*
    Copyright 2023 Jesse Talavera-Greenberg

    melonDS DS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS DS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS DS. If not, see http://www.gnu.org/licenses/.
*/

#ifndef MELONDS_DS_LIBRETRO_HPP
#define MELONDS_DS_LIBRETRO_HPP

#include <libretro.h>
#ifdef HAVE_OPENGL
#include <gl/gl.h>
#include <glsym/glsym.h>
#endif

/**!
 * Contains global state that's accessible to the entire core.
 */

#define PUBLIC_SYMBOL [[maybe_unused]]

#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif

#endif //MELONDS_DS_LIBRETRO_HPP
