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

#include "glsym_private.h"

#ifdef HAVE_OPENGLES

void glDrawBuffer(GLenum buf)
{
    glDrawBuffers(1, &buf);
}
#endif

#if defined(HAVE_OPENGL) && !defined(HAVE_OPENGLES)

MDSGLSYMGLBINDTEXTUREPROC melondsds_glBindTexture;
MDSGLSYMGLBLENDFUNCPROC melondsds_glBlendFunc;
MDSGLSYMGLCLEARPROC melondsds_glClear;
MDSGLSYMGLCLEARCOLORPROC melondsds_glClearColor;
MDSGLSYMGLCLEARDEPTHPROC melondsds_glClearDepth;
MDSGLSYMGLCOLORMASKPROC melondsds_glColorMask;
MDSGLSYMGLCULLFACEPROC melondsds_glCullFace;
MDSGLSYMGLDELETETEXTURESPROC melondsds_glDeleteTextures;
MDSGLSYMGLDEPTHFUNCPROC melondsds_glDepthFunc;
MDSGLSYMGLDEPTHMASKPROC melondsds_glDepthMask;
MDSGLSYMGLDEPTHRANGEPROC melondsds_glDepthRange;
MDSGLSYMGLDISABLEPROC melondsds_glDisable;
MDSGLSYMGLDRAWARRAYSPROC melondsds_glDrawArrays;
MDSGLSYMGLDRAWBUFFERPROC melondsds_glDrawBuffer;
MDSGLSYMGLDRAWELEMENTSPROC melondsds_glDrawElements;
MDSGLSYMGLENABLEPROC melondsds_glEnable;
MDSGLSYMGLFLUSHPROC melondsds_glFlush;
MDSGLSYMGLFRONTFACEPROC melondsds_glFrontFace;
MDSGLSYMGLGENTEXTURESPROC melondsds_glGenTextures;
MDSGLSYMGLGETERRORPROC melondsds_glGetError;
MDSGLSYMGLGETINTEGERVPROC melondsds_glGetIntegerv;
MDSGLSYMGLGETSTRINGPROC melondsds_glGetString;
MDSGLSYMGLGETTEXIMAGEPROC melondsds_glGetTexImage;
MDSGLSYMGLLINEWIDTHPROC melondsds_glLineWidth;
MDSGLSYMGLPIXELSTOREIPROC melondsds_glPixelStorei;
MDSGLSYMGLPOLYGONMODEPROC melondsds_glPolygonMode;
MDSGLSYMGLPOLYGONOFFSETPROC melondsds_glPolygonOffset;
MDSGLSYMGLREADBUFFERPROC melondsds_glReadBuffer;
MDSGLSYMGLREADPIXELSPROC melondsds_glReadPixels;
MDSGLSYMGLSCISSORPROC melondsds_glScissor;
MDSGLSYMGLSTENCILFUNCPROC melondsds_glStencilFunc;
MDSGLSYMGLSTENCILMASKPROC melondsds_glStencilMask;
MDSGLSYMGLSTENCILOPPROC melondsds_glStencilOp;
MDSGLSYMGLTEXIMAGE2DPROC melondsds_glTexImage2D;
MDSGLSYMGLTEXPARAMETERIPROC melondsds_glTexParameteri;
MDSGLSYMGLTEXSUBIMAGE2DPROC melondsds_glTexSubImage2D;
MDSGLSYMGLVIEWPORTPROC melondsds_glViewport;

#define MDS_SYM(x) { "gl" #x, (void*)&(melondsds_gl##x) }

const struct rglgen_sym_map melondsds_base_gl_symbol_map[] = {
    MDS_SYM(BindTexture),
    MDS_SYM(BlendFunc),
    MDS_SYM(Clear),
    MDS_SYM(ClearColor),
    MDS_SYM(ClearDepth),
    MDS_SYM(ColorMask),
    MDS_SYM(CullFace),
    MDS_SYM(DeleteTextures),
    MDS_SYM(DepthFunc),
    MDS_SYM(DepthMask),
    MDS_SYM(DepthRange),
    MDS_SYM(Disable),
    MDS_SYM(DrawArrays),
    MDS_SYM(DrawBuffer),
    MDS_SYM(DrawElements),
    MDS_SYM(Enable),
    MDS_SYM(Flush),
    MDS_SYM(FrontFace),
    MDS_SYM(GenTextures),
    MDS_SYM(GetError),
    MDS_SYM(GetIntegerv),
    MDS_SYM(GetString),
    MDS_SYM(GetTexImage),
    MDS_SYM(LineWidth),
    MDS_SYM(PixelStorei),
    MDS_SYM(PolygonMode),
    MDS_SYM(PolygonOffset),
    MDS_SYM(ReadBuffer),
    MDS_SYM(ReadPixels),
    MDS_SYM(Scissor),
    MDS_SYM(StencilFunc),
    MDS_SYM(StencilMask),
    MDS_SYM(StencilOp),
    MDS_SYM(TexImage2D),
    MDS_SYM(TexParameteri),
    MDS_SYM(TexSubImage2D),
    MDS_SYM(Viewport),
    { nullptr, nullptr },
};
#endif
