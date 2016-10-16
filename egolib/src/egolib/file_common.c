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

/// @file egolib/file_common.c
/// @brief Base implementation of the Egoboo filesystem
/// @details File operations that are shared between various operating systems.
/// OS-specific code goes in *-file.c (such as win-file.c)

#include "egolib/file_common.h"

#include "egolib/Log/_Include.hpp"

#include "egolib/strutil.h"
#include "egolib/vfs.h"
#include "egolib/platform.h"
#include "egolib/FileSystem/FileSystem.hpp"

#if !defined(MAX_PATH)
#define MAX_PATH 260  // Same value that Windows uses...
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool _fs_initialized = false;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// @brief Initialize the platform file system.
/// @param argument0 the first argument of the command-line
/// @return @a 0 on success, a non-zero value on failure
int sys_fs_init(const std::string& argument0);

/// @brief Initialize the platform file system.
/// @param argument0 the first argument of the command-line
/// @param rootPath the root path
/// @return @a 0 on success, a non-zero value on failure
int sys_fs_init(const std::string& argument0, const std::string& rootPath);

/// @brief Uninitialize the platform file system.
void sys_fs_uninit();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

int fs_init(const std::string& argument0) {
    if (_fs_initialized) {
        return 0;
    }
    if (sys_fs_init(argument0)) {
        return 1;
    }
    _fs_initialized = true;
    return 0;
}

int fs_init(const std::string& argument0, const std::string& rootPath) {
    if (_fs_initialized) {
        return 0;
    }
    if (sys_fs_init(argument0, rootPath)) {
        return 1;
    }
    _fs_initialized = true;
    return 0;
}

void fs_uninit() {
    if (!_fs_initialized) {
        return;
    }
    sys_fs_uninit();
    _fs_initialized = false;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void fs_removeDirectoryAndContents(const char *dirname, int recursive)
{
    /// @author ZZ
    /// @details This function deletes all files in a directory,
    ///    and the directory itself

    char filePath[MAX_PATH] = EMPTY_CSTR;
    const char *fileName;
    fs_find_context_t fs_search;

    // List all the files in the directory
    fileName = fs_findFirstFile(dirname, NULL, &fs_search);
    while (NULL != fileName)
    {
        // Ignore files that start with a ., like .svn for example.
        if ('.' != fileName[0])
        {
            snprintf(filePath, MAX_PATH, "%s" SLASH_STR "%s", dirname, fileName);
            if (Ego::FileSystem::get().directoryExists(filePath))
            {
                if (recursive)
                {
                    fs_removeDirectoryAndContents(filePath, recursive);
                }
                else
                {
                    Ego::FileSystem::get().removeDirectory(filePath);
                }
            }
            else
            {
                Ego::FileSystem::get().removeFile(filePath);
            }
        }
        fileName = fs_findNextFile(&fs_search);
    }
    fs_findClose(&fs_search);

    Ego::FileSystem::get().removeDirectory(dirname);
}

//--------------------------------------------------------------------------------------------
void fs_copyDirectory(const char *sourceDir, const char *targetDir)
{
    fs_find_context_t fs_search;

    // List all the files in the directory
    const char *filename = fs_findFirstFile(sourceDir, NULL, &fs_search);
    if (filename)
    {
        // Make sure the destination directory exists.
        Ego::FileSystem::get().createDirectory(targetDir); /// @todo Error handling here - if the directory does not exist, we can stop.

        while (filename)
        {
            // Ignore files that begin with a `'.'`.
            if ('.' != filename[0])
            {
                char sourcePath[MAX_PATH] = EMPTY_CSTR, targetPath[MAX_PATH] = EMPTY_CSTR;
                snprintf(sourcePath, MAX_PATH, "%s" SLASH_STR "%s", sourceDir, filename);
                snprintf(targetPath, MAX_PATH, "%s" SLASH_STR "%s", targetDir, filename);
                Ego::FileSystem::get().copyFile(sourcePath, targetPath);
            }

            filename = fs_findNextFile(&fs_search);
        }
    }

    fs_findClose(&fs_search);
}

//--------------------------------------------------------------------------------------------
bool fs_ensureUserFile( const char * relative_filename, bool required )
{
    /// @author BB
    /// @details if the file does not exist in the user data directory, it is copied from the
    /// data directory. Pass this function a system-dependent pathneme, relative the the root of the
    /// data directory.
    ///
    /// @note we can't use the vfs to do this in win32 because of the dir structure and
    /// the fact that PHYSFS will not add the same directory to 2 different mount points...
    /// seems pretty stupid to me, but there you have it.

    std::string path_str = Ego::FileSystem::get().getUserDirectoryPath() + std::string(SLASH_STR) + relative_filename;
    path_str = str_convert_slash_sys( path_str );

    bool found = Ego::FileSystem::get().fileExists( path_str );
    if (!found)
    {
        // copy the file from the Data Directory to the User Directory
        std::string src_path_str = Ego::FileSystem::get().getConfigurationDirectoryPath() + std::string(SLASH_STR) + relative_filename;
        Ego::FileSystem::get().copyFile( src_path_str, path_str );
        found = Ego::FileSystem::get().fileExists( path_str );
    }

    // if it still doesn't exist, we have problems
    if ( !found && required )
    {
		std::ostringstream os;
		os << "cannot find the file `" << relative_filename << "`" << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }

    return found;
}
