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

#ifndef MELONDS_DS_PLATFORMOGLPRIVATE_H
#define MELONDS_DS_PLATFORMOGLPRIVATE_H

#if defined(__APPLE__) && !defined(GL_SILENCE_DEPRECATION)
#define GL_SILENCE_DEPRECATION
#endif

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
#include <glsym/glsym.h>
#endif

#ifndef GL_BUFFER
#define GL_BUFFER 0x82E0
#endif

#ifndef GL_PROGRAM
#define GL_PROGRAM 0x82E2
#endif

#ifndef GL_VERTEX_ARRAY
#define GL_VERTEX_ARRAY 0x8074
#endif

#ifndef GL_SHADER
#define GL_SHADER 0x82E1
#endif

#ifdef HAVE_OPENGLES
#define GL_UNSIGNED_SHORT_1_5_5_5_REV GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT
#define GL_WRITE_ONLY GL_WRITE_ONLY_OES
#define GL_BGRA GL_BGRA_EXT
#define glBindFragDataLocation glBindFragDataLocationEXT
#define glClearDepth glClearDepthf
#define glColorMaski glColorMaskiEXT
#define glDepthRange glDepthRangef
#define glFramebufferTexture glFramebufferTextureEXT
#define glMapBuffer glMapBufferOES

#ifndef GL_READ_ONLY
#define GL_READ_ONLY 0x88B8

RETRO_BEGIN_DECLS
void glDrawBuffer(GLenum buf);
RETRO_END_DECLS

#endif
#endif

#endif //MELONDS_DS_PLATFORMOGLPRIVATE_H
