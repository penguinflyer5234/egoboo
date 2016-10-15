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

/// @file egolib/file_common.h
/// @brief file system functionality
/**
 * @remark
 *  Some terminology; a pathname is composed of filenames separated by filename separators.
 *  A filename consists of a filename base and optionally a filename extension.
 *  A pathname can start with a root filename that and is then an absolute pathname; otherwise it is a relative pathname.
 *  @remark
 *  Usually a filename separator is a slash or a backlash.
 *  A filename extension is usually the string up to but not including the first period in the filename
 *  when searching from right to left (!). If no period is found, then the filename has no filename
 *  extension.
 */

#pragma once

#include "egolib/typedef.h"

//--------------------------------------------------------------------------------------------
// TYPEDEFS
//--------------------------------------------------------------------------------------------

struct s_win32_find_context;
struct s_linux_find_context;
struct s_mac_find_context;

//--------------------------------------------------------------------------------------------
// struct s_fs_find_context
//--------------------------------------------------------------------------------------------

/// enum to label 3 typed of find data
typedef enum fs_find_type
{
    unknown_find = 0,
    win32_find,
    linux_find,
    mac_find
} fs_find_type_t;

//--------------------------------------------------------------------------------------------

/// struct to alias the 3 types of find data
typedef union fs_find_ptr_t
{
    void *v;
    struct s_win32_find_context *w;
    struct s_linux_find_context *l;
    struct s_mac_find_context *m;
} fs_find_ptr_t;

//--------------------------------------------------------------------------------------------
typedef struct fs_find_context_t
{
    fs_find_type_t type;
    fs_find_ptr_t  ptr;
} fs_find_context_t;

//--------------------------------------------------------------------------------------------
// GLOBAL FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

/// @brief Initialize the file system.
/// @param argument0 the first argument of the command-line
/// @return @a 0 on success, a non-zero value on failure
int fs_init(const std::string& argument0);

/// @brief Initialize the file system.
/// @param argument0 the first argument of the command-line
/// @param rootPath the root path
/// @return @a 0 on success, a non-zero value on failure
int fs_init(const std::string& argument0, const std::string& rootPath);

/// @brief Ensure the file system is uninitialized.
void fs_uninit();

/**@{*/

/**@}*/

void fs_removeDirectoryAndContents(const char *pathname, int recursive);

/// @brief Copy all files in a directory into another directory.
/// @param sourcePath the source path
/// @param targetPath the target path
/// @remark If the target directory does not exist, it is created.
void fs_copyDirectory(const char *source, const char *target);

/**
 * @brief
 *  Begin a search.
 * @param directory
 *  the pathname of the directory to search in. Must not be a null pointer.
 * @param extension
 *  the filename extension of the files to search for.
 *  Only files with filenames ending with a period followed by the specified file extension are seached.
 * @param fs_search
 *  the context
 * @return
 *  a pointer to a C string with the filename of the first file found.
 *  A null pointer is returned if no file was found or an error occurred.
 * @remark
 *  If a pointer to a C string is returned,
 *  then this string remains valid as long as the search context is not modified.
 */
const char *fs_findFirstFile(const char *directory, const char *extension, fs_find_context_t *fs_search);
/**
 * @brief
 *  Continue a search.
 * @param fs_search
 *  the search context
 * @return
 *  a pointer to a C string with the filename of the next file found.
 *  A null pointer is returned if no file was found or an error occurred.
 * @remark
 *  If a pointer to a C string is returned,
 *  then this string remains valid as long as the search context is not modified.
 */
const char *fs_findNextFile(fs_find_context_t *fs_search);
/**
 * @brief
 *  End a search.
 * @param fs_search
 *  the context
 */
void fs_findClose(fs_find_context_t *fs_search);

bool fs_ensureUserFile(const char * relative_filename, bool required);
