//********************************************************************************************
//*
//*    This file is part of the opengl extensions library. This library is
//*    distributed with Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file egolib/Extensions/ogl_extensions.c
/// @ingroup _ogl_extensions_
/// @brief Implementation of extended functions and variables for OpenGL
/// @details

#include "egolib/Extensions/ogl_extensions.h"
#include "egolib/Extensions/ogl_debug.h"
#include "egolib/log.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
oglx_caps_t g_ogl_caps;

void oglx_caps_t::report(oglx_caps_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    log_message("\nOpenGL state parameters\n");
    log_message("\tgl_version    == %s\n", self->gl_version);
    log_message("\tgl_vendor     == %s\n", self->gl_vendor);
    log_message("\tgl_renderer   == %s\n", self->gl_renderer);
    log_message("\tgl_extensions == %s\n", self->gl_extensions);

    log_message("\tglu_version    == %s\n", self->glu_version);
    log_message("\tglu_extensions == %s\n\n", self->glu_extensions);

    log_message("\tGL_MAX_MODELVIEW_STACK_DEPTH     == %d\n", self->max_modelview_stack_depth);
    log_message("\tGL_MAX_PROJECTION_STACK_DEPTH    == %d\n", self->max_projection_stack_depth);
    log_message("\tGL_MAX_TEXTURE_STACK_DEPTH       == %d\n", self->max_texture_stack_depth);
    log_message("\tGL_MAX_NAME_STACK_DEPTH          == %d\n", self->max_name_stack_depth);
    log_message("\tGL_MAX_ATTRIB_STACK_DEPTH        == %d\n", self->max_attrib_stack_depth);
    log_message("\tGL_MAX_CLIENT_ATTRIB_STACK_DEPTH == %d\n\n", self->max_client_attrib_stack_depth);

    log_message("\tGL_SUBPIXEL_BITS          == %d\n", self->subpixel_bits);
    log_message("\tGL_POINT_SIZE_RANGE       == %f - %f\n", self->point_size_range[0], self->point_size_range[1]);
    log_message("\tGL_POINT_SIZE_GRANULARITY == %f\n", self->point_size_granularity);
    log_message("\tGL_LINE_WIDTH_RANGE       == %f - %f\n", self->line_width_range[0], self->line_width_range[1]);
    log_message("\tGL_LINE_WIDTH_GRANULARITY == %f\n\n", self->line_width_granularity);

    log_message("\tGL_MAX_VIEWPORT_DIMS == %d, %d\n", self->max_viewport_dims[0], self->max_viewport_dims[1]);
    log_message("\tGL_AUX_BUFFERS       == %d\n", self->aux_buffers);
    log_message("\tGL_RGBA_MODE         == %s\n", self->rgba_mode ? "TRUE" : "FALSE");
    log_message("\tGL_INDEX_MODE        == %s\n", self->index_mode ? "TRUE" : "FALSE");
    log_message("\tGL_DOUBLEBUFFER      == %s\n", self->doublebuffer ? "TRUE" : "FALSE");
    log_message("\tGL_STEREO            == %s\n", self->stereo ? "TRUE" : "FALSE");
    log_message("\tGL_RED_BITS          == %d\n", self->red_bits);
    log_message("\tGL_GREEN_BITS        == %d\n", self->green_bits);
    log_message("\tGL_BLUE_BITS         == %d\n", self->blue_bits);
    log_message("\tGL_ALPHA_BITS        == %d\n", self->alpha_bits);
    log_message("\tGL_INDEX_BITS        == %d\n", self->index_bits);
    log_message("\tGL_DEPTH_BITS        == %d\n", self->depth_bits);
    log_message("\tGL_STENCIL_BITS      == %d\n", self->stencil_bits);
    log_message("\tGL_ACCUM_RED_BITS    == %d\n", self->accum_red_bits);
    log_message("\tGL_ACCUM_GREEN_BITS  == %d\n", self->accum_green_bits);
    log_message("\tGL_ACCUM_BLUE_BITS   == %d\n", self->accum_blue_bits);
    log_message("\tGL_ACCUM_ALPHA_BITS  == %d\n\n", self->accum_alpha_bits);

    log_message("\tGL_MAX_LIGHTS        == %d\n", self->max_lights);
    log_message("\tGL_MAX_CLIP_PLANES   == %d\n", self->max_clip_planes);
    log_message("\tGL_MAX_TEXTURE_SIZE  == %d\n\n", self->max_texture_size);

    log_message("\tGL_MAX_PIXEL_MAP_TABLE == %d\n", self->max_pixel_map_table);
    log_message("\tGL_MAX_LIST_NESTING    == %d\n", self->max_list_nesting);
    log_message("\tGL_MAX_EVAL_ORDER      == %d\n\n", self->max_eval_order);

    if (self->anisotropic_supported)
    {
        log_message("\tGL_MAX_TEXTURE_MAX_ANISOTROPY_EXT == %f\n", self->maxAnisotropy);
    }

    log_message("==============================================================\n");
}

void oglx_report_caps()
{
    oglx_Get_Screen_Info(&g_ogl_caps);
    oglx_caps_t::report(&g_ogl_caps);
}

void oglx_Get_Screen_Info(oglx_caps_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }

    BLANK_STRUCT_PTR(self);

    // Get any pure OpenGL device caps.

    self->gl_version = GL_DEBUG(glGetString)(GL_VERSION);
    self->gl_vendor = GL_DEBUG(glGetString)(GL_VENDOR);
    self->gl_renderer = GL_DEBUG(glGetString)(GL_RENDERER);
    self->gl_extensions = GL_DEBUG(glGetString)(GL_EXTENSIONS);

    self->glu_version = GL_DEBUG(gluGetString)(GLU_VERSION);
    self->glu_extensions = GL_DEBUG(gluGetString)(GLU_EXTENSIONS);

    GL_DEBUG(glGetIntegerv)(GL_MAX_MODELVIEW_STACK_DEPTH, &self->max_modelview_stack_depth);
    GL_DEBUG(glGetIntegerv)(GL_MAX_PROJECTION_STACK_DEPTH, &self->max_projection_stack_depth);
    GL_DEBUG(glGetIntegerv)(GL_MAX_TEXTURE_STACK_DEPTH, &self->max_texture_stack_depth);
    GL_DEBUG(glGetIntegerv)(GL_MAX_NAME_STACK_DEPTH, &self->max_name_stack_depth);
    GL_DEBUG(glGetIntegerv)(GL_MAX_ATTRIB_STACK_DEPTH, &self->max_attrib_stack_depth);
    GL_DEBUG(glGetIntegerv)(GL_MAX_CLIENT_ATTRIB_STACK_DEPTH, &self->max_client_attrib_stack_depth);

    GL_DEBUG(glGetIntegerv)(GL_SUBPIXEL_BITS, &self->subpixel_bits);
    GL_DEBUG(glGetFloatv)(GL_POINT_SIZE_RANGE, self->point_size_range);
    GL_DEBUG(glGetFloatv)(GL_POINT_SIZE_GRANULARITY, &self->point_size_granularity);
    GL_DEBUG(glGetFloatv)(GL_LINE_WIDTH_RANGE, self->line_width_range);
    GL_DEBUG(glGetFloatv)(GL_LINE_WIDTH_GRANULARITY, &self->line_width_granularity);

    GL_DEBUG(glGetIntegerv)(GL_MAX_VIEWPORT_DIMS, self->max_viewport_dims);
    GL_DEBUG(glGetBooleanv)(GL_AUX_BUFFERS, &self->aux_buffers);
    GL_DEBUG(glGetBooleanv)(GL_RGBA_MODE, &self->rgba_mode);
    GL_DEBUG(glGetBooleanv)(GL_INDEX_MODE, &self->index_mode);
    GL_DEBUG(glGetBooleanv)(GL_DOUBLEBUFFER, &self->doublebuffer);
    GL_DEBUG(glGetBooleanv)(GL_STEREO, &self->stereo);
    GL_DEBUG(glGetIntegerv)(GL_RED_BITS, &self->red_bits);
    GL_DEBUG(glGetIntegerv)(GL_GREEN_BITS, &self->green_bits);
    GL_DEBUG(glGetIntegerv)(GL_BLUE_BITS, &self->blue_bits);
    GL_DEBUG(glGetIntegerv)(GL_ALPHA_BITS, &self->alpha_bits);
    GL_DEBUG(glGetIntegerv)(GL_INDEX_BITS, &self->index_bits);
    GL_DEBUG(glGetIntegerv)(GL_DEPTH_BITS, &self->depth_bits);
    GL_DEBUG(glGetIntegerv)(GL_STENCIL_BITS, &self->stencil_bits);
    GL_DEBUG(glGetIntegerv)(GL_ACCUM_RED_BITS, &self->accum_red_bits);
    GL_DEBUG(glGetIntegerv)(GL_ACCUM_GREEN_BITS, &self->accum_green_bits);
    GL_DEBUG(glGetIntegerv)(GL_ACCUM_BLUE_BITS, &self->accum_blue_bits);
    GL_DEBUG(glGetIntegerv)(GL_ACCUM_ALPHA_BITS, &self->accum_alpha_bits);

    GL_DEBUG(glGetIntegerv)(GL_MAX_LIGHTS, &self->max_lights);
    GL_DEBUG(glGetIntegerv)(GL_MAX_CLIP_PLANES, &self->max_clip_planes);
    GL_DEBUG(glGetIntegerv)(GL_MAX_TEXTURE_SIZE, &self->max_texture_size);

    GL_DEBUG(glGetIntegerv)(GL_MAX_PIXEL_MAP_TABLE, &self->max_pixel_map_table);
    GL_DEBUG(glGetIntegerv)(GL_MAX_LIST_NESTING, &self->max_list_nesting);
    GL_DEBUG(glGetIntegerv)(GL_MAX_EVAL_ORDER, &self->max_eval_order);

    self->maxAnisotropy = 0;
    self->log2Anisotropy = 0;

    /// Get the supported values for anisotropic filtering.
    self->anisotropic_supported = GL_FALSE;
    self->maxAnisotropy = 1.0f;
    self->log2Anisotropy = 0.0f;
    if (NULL != self->gl_extensions && NULL != strstr((char*)self->gl_extensions, "GL_EXT_texture_filter_anisotropic"))
    {
        self->anisotropic_supported = GL_TRUE;
        GL_DEBUG(glGetFloatv)(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &(self->maxAnisotropy));
        self->log2Anisotropy = (0 == self->maxAnisotropy) ? 0.0f : floor(log(self->maxAnisotropy + 1e-6) / log(2.0f));
    }
}

//--------------------------------------------------------------------------------------------

namespace Ego
{
namespace OpenGL
{

GLint Utilities::toOpenGL(Ego::TextureAddressMode textureAddressMode)
{
    switch (textureAddressMode)
    {
        case Ego::TextureAddressMode::Clamp:
            return GL_CLAMP;
        case Ego::TextureAddressMode::ClampToBorder:
            return GL_CLAMP_TO_BORDER;
        case Ego::TextureAddressMode::ClampToEdge:
            return GL_CLAMP_TO_EDGE;
        case Ego::TextureAddressMode::Repeat:
            return GL_REPEAT;
        case Ego::TextureAddressMode::RepeatMirrored:
            return GL_MIRRORED_REPEAT;
        default:
            throw std::runtime_error("unreachable code reached");
    }
}

void Utilities::clearError()
{
    while (GL_NO_ERROR != glGetError())
    {
        /* Nothing to do. */
    }
}

bool Utilities::isError()
{
    GLenum error = glGetError();
    if (GL_NO_ERROR != error)
    {
        switch (error)
        {
            case GL_INVALID_ENUM:
                log_warning("%s:%d: %s\n", __FILE__, __LINE__, "GL_INVALID_ENUM");
                break;
            case GL_INVALID_VALUE:
                log_warning("%s:%d: %s\n", __FILE__, __LINE__, "GL_INVALID_VALUE");
                break;
            case GL_INVALID_OPERATION:
                log_warning("%s:%d: %s\n", __FILE__, __LINE__, "GL_INVALID_OPERATION");
                break;
        #if defined(GL_INVALID_FRAMEBUFFER_OPERATION)
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                log_warning("%s:%d: %s\n", __FILE__, __LINE__, "GL_INVALID_FRAMEBUFFER_OPERATION");
                break;
        #endif
            case GL_OUT_OF_MEMORY:
                log_warning("%s:%d: %s\n", __FILE__, __LINE__, "GL_OUT_OF_MEMORY");
                break;
            case GL_STACK_UNDERFLOW:
                log_warning("%s:%d: %s\n", __FILE__, __LINE__, "GL_STACK_UNDERFLOW");
                break;
            case GL_STACK_OVERFLOW:
                log_warning("%s:%d: %s\n", __FILE__, __LINE__, "GL_STACK_OVERFLOW");
                break;
        };
        clearError();
        return true;
    }
    return false;
}

void Utilities::upload_1d(bool useAlpha, GLsizei w, const void *data)
{
    if (useAlpha)
    {
        GL_DEBUG(glTexImage1D)(GL_TEXTURE_1D, 0, GL_RGBA, w, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
    else
    {
        GL_DEBUG(glTexImage1D)(GL_TEXTURE_1D, 0, GL_RGB, w, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, data);
    }
}

void Utilities::upload_2d(bool useAlpha, GLsizei w, GLsizei h, const void *data)
{
    if (useAlpha)
    {
        GL_DEBUG(glTexImage2D)(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
    else
    {
        GL_DEBUG(glTexImage2D)(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, data);
    }
}

void Utilities::upload_2d_mipmap(bool useAlpha, GLsizei w, GLsizei h, const void *data)
{
    if (useAlpha)
    {
        GL_DEBUG(gluBuild2DMipmaps)(GL_TEXTURE_2D, 4, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
    else
    {
        GL_DEBUG(gluBuild2DMipmaps)(GL_TEXTURE_2D, GL_RGB, w, h, GL_BGR_EXT, GL_UNSIGNED_BYTE, data);
    }
}

void Utilities::bind(GLuint id, Ego::TextureType target, Ego::TextureAddressMode textureAddressModeS, Ego::TextureAddressMode textureAddressModeT)
{
    auto textureFiltering = g_ogl_textureParameters.textureFiltering;
    auto anisotropy_enable = g_ogl_textureParameters.anisotropy_enable;
    auto anisotropy_level = g_ogl_textureParameters.anisotropy_level;
    Ego::OpenGL::Utilities::clearError();
    GLenum target_gl;
    switch (target)
    {
        case Ego::TextureType::_2D:
            glEnable(GL_TEXTURE_2D);
            glDisable(GL_TEXTURE_1D);
            target_gl = GL_TEXTURE_2D;
            break;
        case Ego::TextureType::_1D:
            glEnable(GL_TEXTURE_1D);
            glDisable(GL_TEXTURE_2D);
            target_gl = GL_TEXTURE_1D;
            break;
        default:
            throw std::runtime_error("unreachable code reached");
    }
    if (Ego::OpenGL::Utilities::isError())
    {
        return;
    }
    glBindTexture(target_gl, id);
    if (Ego::OpenGL::Utilities::isError())
    {
        return;
    }

    glTexParameteri(target_gl, GL_TEXTURE_WRAP_S, toOpenGL(textureAddressModeS));
    glTexParameteri(target_gl, GL_TEXTURE_WRAP_T, toOpenGL(textureAddressModeT));


    if (Ego::OpenGL::Utilities::isError())
    {
        return;
    }


    GLint minFilter_gl, magFilter_gl;
    switch (textureFiltering)
    {
        // Unfiltered
        case Ego::TextureFilter::UNFILTERED:
            minFilter_gl = GL_NEAREST;
            magFilter_gl = GL_LINEAR;
            break;

        // Linear filtered
        case Ego::TextureFilter::LINEAR:
            minFilter_gl = GL_LINEAR;
            magFilter_gl = GL_LINEAR;
            break;

        // Bilinear interpolation
        case Ego::TextureFilter::MIPMAP:
            minFilter_gl = GL_NEAREST_MIPMAP_NEAREST;
            magFilter_gl = GL_LINEAR;
            break;

        // Bilinear interpolation
        case Ego::TextureFilter::BILINEAR:
            minFilter_gl = GL_LINEAR_MIPMAP_NEAREST;
            magFilter_gl = GL_LINEAR;
            break;

        // Trilinear filtered (quality 1)
        case Ego::TextureFilter::TRILINEAR_1:
            minFilter_gl = GL_NEAREST_MIPMAP_LINEAR;
            magFilter_gl = GL_LINEAR;
            break;

        // Trilinear filtered (quality 2)
        case Ego::TextureFilter::TRILINEAR_2:
            minFilter_gl = GL_LINEAR_MIPMAP_LINEAR;
            magFilter_gl = GL_LINEAR;
            break;

        default:
            throw std::runtime_error("unreachable code reached");
    };

    glTexParameteri(target_gl, GL_TEXTURE_MIN_FILTER, minFilter_gl);
    glTexParameteri(target_gl, GL_TEXTURE_MAG_FILTER, magFilter_gl);

    if (Ego::OpenGL::Utilities::isError())
    {
        return;
    }


    if (GL_TEXTURE_2D == target_gl && g_ogl_caps.anisotropic_supported && anisotropy_enable && anisotropy_level >= 1.0f)
    {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy_level);
    }

    if (Ego::OpenGL::Utilities::isError())
    {
        return;
    }
}

} // namespace OpenGL
} // namespace Ego

//--------------------------------------------------------------------------------------------

void oglx_video_parameters_t::defaults(oglx_video_parameters_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }

    self->multisample = GL_FALSE;
    self->multisample_arb = GL_FALSE;
    self->perspective = GL_FASTEST;
    self->dither = GL_FALSE;
    self->shading = GL_SMOOTH;
    self->anisotropy_enable = GL_FALSE;
    self->anisotropy_levels = 1.0f;
}

void oglx_video_parameters_t::download(oglx_video_parameters_t *self, egoboo_config_t *cfg)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    if (!cfg)
    {
        throw std::invalid_argument("nullptr == cfg");
    }
    self->dither = cfg->graphic_dithering_enable.getValue() ? GL_TRUE : GL_FALSE;
    self->antialiasing = cfg->graphic_antialiasing.getValue() ? GL_TRUE : GL_FALSE;
    self->perspective = cfg->graphic_perspectiveCorrection_enable.getValue() ? GL_NICEST : GL_FASTEST;
    self->shading = cfg->graphic_gouraudShading_enable.getValue() ? GL_SMOOTH : GL_FLAT;
    self->anisotropy_enable = cfg->graphic_anisotropy_enable.getValue() ? GL_TRUE : GL_FALSE;
    self->anisotropy_levels = cfg->graphic_anisotropy_levels.getValue();
}

//--------------------------------------------------------------------------------------------

oglx_texture_parameters_t g_ogl_textureParameters = { Ego::TextureFilter::UNFILTERED, false, 1.0f };

void oglx_texture_parameters_t::defaults(oglx_texture_parameters_t* self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    self->textureFiltering = Ego::TextureFilter::UNFILTERED;
    self->anisotropy_enable = false;
    self->anisotropy_level = 1.0f;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void oglx_begin_culling( GLenum face, GLenum mode )
{
    /* Backface culling */
    // The glEnable() seems implied - DDOI

    // cull backward facing polygons
    glEnable(GL_CULL_FACE);  // GL_ENABLE_BIT
    glCullFace(face);        // GL_POLYGON_BIT
    glFrontFace(mode);       // GL_POLYGON_BIT
    Ego::OpenGL::Utilities::isError();
}

//--------------------------------------------------------------------------------------------
void oglx_end_culling( void )
{
    GL_DEBUG(glDisable)(GL_CULL_FACE);  // GL_ENABLE_BIT
}

//--------------------------------------------------------------------------------------------
