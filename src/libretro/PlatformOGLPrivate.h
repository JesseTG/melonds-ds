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

#ifndef GL_SHADER_STORAGE_BUFFER
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#endif

#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER 0x91B9
#endif

#ifndef GL_DISPATCH_INDIRECT_BUFFER
#define GL_DISPATCH_INDIRECT_BUFFER 0x90EE
#endif

#ifndef GL_SHADER_STORAGE_BARRIER_BIT
#define GL_SHADER_STORAGE_BARRIER_BIT 0x2000
#endif

#ifndef GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x00000020
#endif

#ifndef GL_COMMAND_BARRIER_BIT
#define GL_COMMAND_BARRIER_BIT 0x00000040
#endif

#if defined(HAVE_OPENGL) && !defined(HAVE_OPENGLES)
// glsym.h doesn't have wrappers for OpenGL 1.x functions,
// so we define our own equivalents
// to ensure the core uses the correct ones.
// If melonDS or the core starts using another OpenGL 1.0/1.1 function,
// add it here and to melondsds_base_gl_symbol_map in glsym_private.cpp.
// nx_glsym.h defines typedefs for those function pointers,
// but rglgen's pointer array only includes entries for them
// if HAVE_LIBNX is defined, and I don't want to enable it project-wide.

typedef void (APIENTRYP MDSGLSYMGLBINDTEXTUREPROC) (GLenum target, GLuint texture);
typedef void (APIENTRYP MDSGLSYMGLBLENDFUNCPROC) (GLenum sfactor, GLenum dfactor);
typedef void (APIENTRYP MDSGLSYMGLCLEARPROC) (GLbitfield mask);
typedef void (APIENTRYP MDSGLSYMGLCLEARCOLORPROC) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (APIENTRYP MDSGLSYMGLCLEARDEPTHPROC) (GLdouble depth);
typedef void (APIENTRYP MDSGLSYMGLCOLORMASKPROC) (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
typedef void (APIENTRYP MDSGLSYMGLCULLFACEPROC) (GLenum mode);
typedef void (APIENTRYP MDSGLSYMGLDELETETEXTURESPROC) (GLsizei n, const GLuint *textures);
typedef void (APIENTRYP MDSGLSYMGLDEPTHFUNCPROC) (GLenum func);
typedef void (APIENTRYP MDSGLSYMGLDEPTHMASKPROC) (GLboolean flag);
typedef void (APIENTRYP MDSGLSYMGLDEPTHRANGEPROC) (GLdouble n, GLdouble f);
typedef void (APIENTRYP MDSGLSYMGLDISABLEPROC) (GLenum cap);
typedef void (APIENTRYP MDSGLSYMGLDRAWARRAYSPROC) (GLenum mode, GLint first, GLsizei count);
typedef void (APIENTRYP MDSGLSYMGLDRAWBUFFERPROC) (GLenum buf);
typedef void (APIENTRYP MDSGLSYMGLDRAWELEMENTSPROC) (GLenum mode, GLsizei count, GLenum type, const void *indices);
typedef void (APIENTRYP MDSGLSYMGLENABLEPROC) (GLenum cap);
typedef void (APIENTRYP MDSGLSYMGLFLUSHPROC) (void);
typedef void (APIENTRYP MDSGLSYMGLFRONTFACEPROC) (GLenum mode);
typedef void (APIENTRYP MDSGLSYMGLGENTEXTURESPROC) (GLsizei n, GLuint *textures);
typedef GLenum (APIENTRYP MDSGLSYMGLGETERRORPROC) (void);
typedef void (APIENTRYP MDSGLSYMGLGETINTEGERVPROC) (GLenum pname, GLint *data);
typedef const GLubyte *(APIENTRYP MDSGLSYMGLGETSTRINGPROC) (GLenum name);
typedef void (APIENTRYP MDSGLSYMGLGETTEXIMAGEPROC) (GLenum target, GLint level, GLenum format, GLenum type, void *pixels);
typedef void (APIENTRYP MDSGLSYMGLLINEWIDTHPROC) (GLfloat width);
typedef void (APIENTRYP MDSGLSYMGLPIXELSTOREIPROC) (GLenum pname, GLint param);
typedef void (APIENTRYP MDSGLSYMGLPOLYGONMODEPROC) (GLenum face, GLenum mode);
typedef void (APIENTRYP MDSGLSYMGLPOLYGONOFFSETPROC) (GLfloat factor, GLfloat units);
typedef void (APIENTRYP MDSGLSYMGLREADBUFFERPROC) (GLenum src);
typedef void (APIENTRYP MDSGLSYMGLREADPIXELSPROC) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels);
typedef void (APIENTRYP MDSGLSYMGLSCISSORPROC) (GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (APIENTRYP MDSGLSYMGLSTENCILFUNCPROC) (GLenum func, GLint ref, GLuint mask);
typedef void (APIENTRYP MDSGLSYMGLSTENCILMASKPROC) (GLuint mask);
typedef void (APIENTRYP MDSGLSYMGLSTENCILOPPROC) (GLenum sfail, GLenum dpfail, GLenum dppass);
typedef void (APIENTRYP MDSGLSYMGLTEXIMAGE2DPROC) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
typedef void (APIENTRYP MDSGLSYMGLTEXPARAMETERIPROC) (GLenum target, GLenum pname, GLint param);
typedef void (APIENTRYP MDSGLSYMGLTEXSUBIMAGE2DPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
typedef void (APIENTRYP MDSGLSYMGLVIEWPORTPROC) (GLint x, GLint y, GLsizei width, GLsizei height);

RETRO_BEGIN_DECLS

extern MDSGLSYMGLBINDTEXTUREPROC melondsds_glBindTexture;
extern MDSGLSYMGLBLENDFUNCPROC melondsds_glBlendFunc;
extern MDSGLSYMGLCLEARPROC melondsds_glClear;
extern MDSGLSYMGLCLEARCOLORPROC melondsds_glClearColor;
extern MDSGLSYMGLCLEARDEPTHPROC melondsds_glClearDepth;
extern MDSGLSYMGLCOLORMASKPROC melondsds_glColorMask;
extern MDSGLSYMGLCULLFACEPROC melondsds_glCullFace;
extern MDSGLSYMGLDELETETEXTURESPROC melondsds_glDeleteTextures;
extern MDSGLSYMGLDEPTHFUNCPROC melondsds_glDepthFunc;
extern MDSGLSYMGLDEPTHMASKPROC melondsds_glDepthMask;
extern MDSGLSYMGLDEPTHRANGEPROC melondsds_glDepthRange;
extern MDSGLSYMGLDISABLEPROC melondsds_glDisable;
extern MDSGLSYMGLDRAWARRAYSPROC melondsds_glDrawArrays;
extern MDSGLSYMGLDRAWBUFFERPROC melondsds_glDrawBuffer;
extern MDSGLSYMGLDRAWELEMENTSPROC melondsds_glDrawElements;
extern MDSGLSYMGLENABLEPROC melondsds_glEnable;
extern MDSGLSYMGLFLUSHPROC melondsds_glFlush;
extern MDSGLSYMGLFRONTFACEPROC melondsds_glFrontFace;
extern MDSGLSYMGLGENTEXTURESPROC melondsds_glGenTextures;
extern MDSGLSYMGLGETERRORPROC melondsds_glGetError;
extern MDSGLSYMGLGETINTEGERVPROC melondsds_glGetIntegerv;
extern MDSGLSYMGLGETSTRINGPROC melondsds_glGetString;
extern MDSGLSYMGLGETTEXIMAGEPROC melondsds_glGetTexImage;
extern MDSGLSYMGLLINEWIDTHPROC melondsds_glLineWidth;
extern MDSGLSYMGLPIXELSTOREIPROC melondsds_glPixelStorei;
extern MDSGLSYMGLPOLYGONMODEPROC melondsds_glPolygonMode;
extern MDSGLSYMGLPOLYGONOFFSETPROC melondsds_glPolygonOffset;
extern MDSGLSYMGLREADBUFFERPROC melondsds_glReadBuffer;
extern MDSGLSYMGLREADPIXELSPROC melondsds_glReadPixels;
extern MDSGLSYMGLSCISSORPROC melondsds_glScissor;
extern MDSGLSYMGLSTENCILFUNCPROC melondsds_glStencilFunc;
extern MDSGLSYMGLSTENCILMASKPROC melondsds_glStencilMask;
extern MDSGLSYMGLSTENCILOPPROC melondsds_glStencilOp;
extern MDSGLSYMGLTEXIMAGE2DPROC melondsds_glTexImage2D;
extern MDSGLSYMGLTEXPARAMETERIPROC melondsds_glTexParameteri;
extern MDSGLSYMGLTEXSUBIMAGE2DPROC melondsds_glTexSubImage2D;
extern MDSGLSYMGLVIEWPORTPROC melondsds_glViewport;

extern const struct rglgen_sym_map melondsds_base_gl_symbol_map[];

RETRO_END_DECLS

#define glBindTexture melondsds_glBindTexture
#define glBlendFunc melondsds_glBlendFunc
#define glClear melondsds_glClear
#define glClearColor melondsds_glClearColor
#define glClearDepth melondsds_glClearDepth
#define glColorMask melondsds_glColorMask
#define glCullFace melondsds_glCullFace
#define glDeleteTextures melondsds_glDeleteTextures
#define glDepthFunc melondsds_glDepthFunc
#define glDepthMask melondsds_glDepthMask
#define glDepthRange melondsds_glDepthRange
#define glDisable melondsds_glDisable
#define glDrawArrays melondsds_glDrawArrays
#define glDrawBuffer melondsds_glDrawBuffer
#define glDrawElements melondsds_glDrawElements
#define glEnable melondsds_glEnable
#define glFlush melondsds_glFlush
#define glFrontFace melondsds_glFrontFace
#define glGenTextures melondsds_glGenTextures
#define glGetError melondsds_glGetError
#define glGetIntegerv melondsds_glGetIntegerv
#define glGetString melondsds_glGetString
#define glGetTexImage melondsds_glGetTexImage
#define glLineWidth melondsds_glLineWidth
#define glPixelStorei melondsds_glPixelStorei
#define glPolygonMode melondsds_glPolygonMode
#define glPolygonOffset melondsds_glPolygonOffset
#define glReadBuffer melondsds_glReadBuffer
#define glReadPixels melondsds_glReadPixels
#define glScissor melondsds_glScissor
#define glStencilFunc melondsds_glStencilFunc
#define glStencilMask melondsds_glStencilMask
#define glStencilOp melondsds_glStencilOp
#define glTexImage2D melondsds_glTexImage2D
#define glTexParameteri melondsds_glTexParameteri
#define glTexSubImage2D melondsds_glTexSubImage2D
#define glViewport melondsds_glViewport

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
#endif

RETRO_BEGIN_DECLS
void glDrawBuffer(GLenum buf);
RETRO_END_DECLS

#endif

#endif //MELONDS_DS_PLATFORMOGLPRIVATE_H
