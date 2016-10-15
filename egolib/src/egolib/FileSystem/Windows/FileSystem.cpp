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

#include "egolib/FileSystem/Windows/FileSystem.hpp"
#include <Windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include "egolib/platform.h"
#include "egolib/strutil.h"

namespace Ego {
namespace Windows {

static bool hasAttribs(DWORD variable, DWORD mask) {
    if (INVALID_FILE_ATTRIBUTES != variable) {
        return (mask == (variable & mask));
    }
    return false;
}

/// @brief Get the user directory path.
/// @return the user directory path
/// @todo Error handling.
static std::string computeUserDirectoryPath() {
    char temporary[MAX_PATH] = EMPTY_CSTR;
    // The save path goes into the user's ApplicationData directory,
    // according to Microsoft's standards.  Will people like this, or
    // should I stick saves someplace easier to find, like My Documents?
    SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, temporary);
    strncat(temporary, SLASH_STR "Egoboo", MAX_PATH);
    return temporary;
}

/// @brief Compute the binary directory path.
/// @return the binary directory path
/// @todo Error handling.
static std::string computeBinaryDirectoryPath() {
    char temporary[MAX_PATH] = EMPTY_CSTR;
    GetModuleFileName(NULL, temporary, MAX_PATH);
    PathRemoveFileSpec(temporary);
    return temporary;
}

/// @brief Compute the working directory path.
/// @return the working directory path
/// @todo Error handling.
static std::string computeWorkingDirectoryPath() {
    char temporary[MAX_PATH] = EMPTY_CSTR;
    GetCurrentDirectory(MAX_PATH, temporary);
    return temporary;
}

/// @brief Compute the data directory path.
/// @return the data directory path
/// @throw Id::RuntimeErrorException computation of the data directory path failed
static std::string computeDataDirectoryPath() {
    std::string path;
    DWORD attributes;

    // (1) Check for data in the working directory
    path = computeWorkingDirectoryPath();
    path = path + SLASH_STR + "data";
    attributes = GetFileAttributes(path.c_str());
    if (hasAttribs(attributes, FILE_ATTRIBUTE_DIRECTORY)) {
        return path;
    }
    // if (1) failed
    // (2) Check for data in the binary directory
    path = computeBinaryDirectoryPath();
    path = path + SLASH_STR + "basicdat";
    attributes = GetFileAttributes(path.c_str());
    if (hasAttribs(attributes, FILE_ATTRIBUTE_DIRECTORY)) {
        return path;
    }
    throw Id::RuntimeErrorException(__FILE__, __LINE__, "unable to compute data directory path");
}

FileSystem::FileSystem(const std::string& argument0) :
    m_userDirectoryPath(computeUserDirectoryPath()),
    m_binaryDirectoryPath(computeBinaryDirectoryPath()),
    m_dataDirectoryPath(computeDataDirectoryPath()),
    m_configurationDirectoryPath(m_dataDirectoryPath){
    // No logging at this point as logging relies on the file system.
    std::cout << "game directories:" << std::endl;
    std::cout << "  binary directory:        " << m_binaryDirectoryPath << std::endl;
    std::cout << "  data directory:          " << m_dataDirectoryPath << std::endl;
    std::cout << "  configuration directory: " << m_configurationDirectoryPath << std::endl;
    std::cout << "  user directory:          " << m_userDirectoryPath << std::endl;
}

std::string FileSystem::getUserDirectoryPath() const {
    return m_userDirectoryPath;
}

std::string FileSystem::getDataDirectoryPath() const {
    return m_dataDirectoryPath;
}

std::string FileSystem::getBinaryDirectoryPath() const {
    return m_binaryDirectoryPath;
}

std::string FileSystem::getConfigurationDirectoryPath() const {
    return m_configurationDirectoryPath;
}

bool FileSystem::exists(const std::string& path) {
    if (path.empty()) {
        throw std::runtime_error("empty path");
    }
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesEx(path.c_str(), GetFileExInfoStandard, &fileInfo)) {
        return true;
    } else {
        return false;
    }
}

bool FileSystem::directoryExists(const std::string& path) {
    if (path.empty()) {
        throw std::runtime_error("empty path");
    }
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesEx(path.c_str(), GetFileExInfoStandard, &fileInfo)) {
        return FILE_ATTRIBUTE_DIRECTORY == (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    } else {
        return false;
    }
}

bool FileSystem::fileExists(const std::string& path) {
    if (path.empty()) {
        throw std::runtime_error("empty path");
    }
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesEx(path.c_str(), GetFileExInfoStandard, &fileInfo)) {
        return 0 == (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    } else {
        return false;
    }
}

bool FileSystem::createDirectory(const std::string& path) {
    if (path.empty()) {
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    }
    return (0 != CreateDirectory(path.c_str(), NULL)) ? 0 : 1;
}

bool FileSystem::removeDirectory(const std::string& path) {
    if (path.empty()) {
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    }
    return (0 != RemoveDirectory(path.c_str())) ? 0 : 1;
}

bool FileSystem::removeFile(const std::string& path) {
    if (path.empty()) {
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    }
    return (0 != DeleteFile(path.c_str()));
}

bool FileSystem::copyFile(const std::string& sourcePath, const std::string& targetPath) {
    if (sourcePath.empty() || targetPath.empty()) {
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    }
    return (TRUE == CopyFile(sourcePath.c_str(), targetPath.c_str(), false));
}

} // namespace Windows
} // namespace Ego
