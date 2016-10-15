//********************************************************************************************
//*
//*    This file is part of Egoboo.
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

/// @file egolib/Platform/file_win.c
/// @brief Windows-specific filesystem functions.
/// @details
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#undef NOMINMAX
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>

#include "egolib/FileSystem/Windows/FileSystem.hpp"
#include "egolib/file_common.h"
#include "egolib/Log/_Include.hpp"
#include "egolib/strutil.h"
#include "egolib/platform.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define HAS_ATTRIBS(ATTRIBS,VAR) ((INVALID_FILE_ATTRIBUTES != (VAR)) && ( (ATTRIBS) == ( (VAR) & (ATTRIBS) ) ))

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_win32_find_context;
typedef struct s_win32_find_context win32_find_context_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern int sys_fs_init(const std::string& argument0);
extern int sys_fs_init(const std::string& argument0, const std::string& rootPath);
extern void sys_fs_uninit();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

int sys_fs_init(const std::string& argument0) {
    try {
        Ego::FileSystem::initialize(argument0);
    } catch (...) {
        return 1;
    }
    return 0;
}
int sys_fs_init(const std::string& argument0, const std::string& rootPath) {
    try {
        Ego::FileSystem::initialize(argument0);
    } catch (...) {
        return 1;
    }
    return 0;
}

void sys_fs_uninit() {
    Ego::FileSystem::uninitialize();
}

//--------------------------------------------------------------------------------------------
// Directory Functions
//--------------------------------------------------------------------------------------------
struct s_win32_find_context
{
    WIN32_FIND_DATA wfdData;
    HANDLE          hFind;
};

const char *fs_findFirstFile(const char *searchDir, const char *searchExtension, fs_find_context_t *fs_search)
{
    char searchSpec[MAX_PATH] = EMPTY_CSTR;

    if (INVALID_CSTR(searchDir) || !fs_search)
    {
        return nullptr;
    }

	win32_find_context_t *pcnt = new win32_find_context_t();
    fs_search->type = win32_find;
    fs_search->ptr.w = pcnt;

    size_t len = strlen(searchDir) + 1;
    if (C_SLASH_CHR != searchDir[len] || C_BACKSLASH_CHR != searchDir[len]) {
        _snprintf(searchSpec, MAX_PATH, "%s" SLASH_STR, searchDir);
    } else {
        strncpy(searchSpec, searchDir, MAX_PATH);
    }
    if (nullptr != searchExtension) {
        _snprintf(searchSpec, MAX_PATH, "%s*.%s", searchSpec, searchExtension);
    } else {
        strncat(searchSpec, "*", MAX_PATH);
    }

    pcnt->hFind = FindFirstFile( searchSpec, &pcnt->wfdData );
	if (pcnt->hFind == INVALID_HANDLE_VALUE) {
		return nullptr;
	}

    return pcnt->wfdData.cFileName;
}

const char *fs_findNextFile(fs_find_context_t *fs_search)
{
    if (!fs_search || win32_find != fs_search->type)
    {
        return NULL;
    }
    win32_find_context_t *pcnt = fs_search->ptr.w;
    if (!pcnt)
    {
        return NULL;
    }
    if (NULL == pcnt->hFind || INVALID_HANDLE_VALUE == pcnt->hFind)
    {
        return NULL;
    }
    if (!FindNextFile( pcnt->hFind, &pcnt->wfdData))
    {
        return NULL;
    }
    return pcnt->wfdData.cFileName;
}

void fs_findClose(fs_find_context_t *fs_search)
{
    if (NULL == fs_search || win32_find != fs_search->type)
    {
        return;
    }
    win32_find_context_t *pcnt = fs_search->ptr.w;
    if (NULL == pcnt)
    {
        return;
    }
    if (NULL != pcnt->hFind)
    {
        FindClose(pcnt->hFind);
        pcnt->hFind = NULL;
    }
    delete pcnt;

	fs_search->type = unknown_find;
	fs_search->ptr.v = nullptr;
}

int DirGetAttrib(const char *fromdir)
{
    return(GetFileAttributes(fromdir));
}
