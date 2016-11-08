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

/// @file egolib/FileSystem/FileSystem.cpp
/// @brief Common interface of all file systems
/// @author Michael Heilmann

#include "egolib/FileSystem/FileSystem.hpp"

#if defined(ID_WINDOWS)
#include "egolib/FileSystem/Windows/FileSystem.hpp"
#elif defined (ID_LINUX)
#include "egolib/FileSystem/Linux/FileSystem.hpp"
#elif defined (ID_OSX) || defined (ID_IOS) || defined (ID_IOSSIMULATOR)
#include "egolib/FileSystem/OSX/FileSystem.hpp"
#endif


namespace Ego {
namespace Core {
Ego::FileSystem *CreateFunctor<Ego::FileSystem>::operator()(const std::string& argument0, const std::string& rootPath) const {
#if defined(ID_WINDOWS)
    return new Ego::Windows::FileSystem(argument0, rootPath);
#elif defined (ID_LINUX)
    return new Ego::Linux::FileSystem(argument0, rootPath);
#elif defined (ID_OSX) || defined (ID_IOS) || defined (ID_IOSSIMULATOR)
    return new Ego::OSX::FileSystem(argument0, rootPath);
#else
#error No platform filesystem found!
#endif
}
} // namespace Core

FileSystem::FileSystem() {
    /* Intentionally empty. */
}

FileSystem::~FileSystem() {
    /* Intentionally empty. */
}

bool FileSystem::copyDirectory(const std::string &sourcePath, const std::string &targetPath) {
    // Make sure the destination directory exists.
    if (!createDirectory(targetPath))
        return false;
    
    // List all the files in the directory
    auto files = findFiles(sourcePath, "");
    bool success = true;
    
    for (auto &file : files) {
        // TODO: is there a good reason for this?
        // Ignore files that begin with a `'.'`.
        if ('.' != file[0])
        {
            auto newSourcePath = sourcePath + SLASH_STR + file;
            auto newTargetPath = targetPath + SLASH_STR + file;
            if (!copyFile(newSourcePath, newTargetPath))
                success = false;
        }
    }
    
    return success;
}

bool FileSystem::removeDirectoryAndContents(const std::string &path, bool recursive) {
    auto files = findFiles(path, "");
    for (auto &file : files) {
        // TODO: is there a good reason for this? it won't delete .DS_Store and thus fail to delete the non-empty directory
        // Ignore files that start with a ., like .svn for example.
        if ('.' != file[0]) {
            std::string filePath = path + SLASH_STR + file;
            if (directoryExists(filePath)) {
                if (recursive)
                    removeDirectoryAndContents(filePath, recursive);
                else
                    removeDirectory(filePath);
            } else {
                removeFile(filePath);
            }
        }
    }
    
    return removeDirectory(path);
}

std::vector<std::string> FileSystem::findFiles(const std::string &path, const std::string &extension) {
    std::vector<std::string> files;
    findFiles(path, extension, [&](const std::string &path) {
        files.push_back(path);
        return true;
    });
    return files;
}

} // namespace Ego
