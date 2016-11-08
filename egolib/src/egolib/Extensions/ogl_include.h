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

/// @defgroup _ogl_extensions_ Extensions to OpenGL

/// @file egolib/Extensions/ogl_include.h
/// @ingroup _ogl_extensions_
/// @brief Base definitions of the extensions to OpenGL
/// @details

#pragma once

#include "egolib/typedef.h"

enum { XX = 0, YY, ZZ, WW };         ///< indices for x, y, z, and w coordinates in a 4-vector
enum { RR = 0, GG, BB, AA };         ///< indices for r, g, b, and alpha coordinates in a 4-color vector
enum { SS = 0, TT };                 ///< indices for s and t, 4-vector texture coordinate

typedef GLfloat GLXvector4f[4];      ///< generic 4-vector
typedef GLfloat GLXvector3f[3];      ///< generic 3-vector
typedef GLfloat GLXvector2f[2];      ///< generic 2-vector
