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
#include <codecvt>
#include "egolib/platform.h"
#include "egolib/strutil.h"

namespace Ego {
namespace Windows {

static std::string windowsStringToUTF8(const std::wstring &string) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
    return convert.to_bytes(string);
}

static std::wstring utf8StringToWindows(const std::string &string) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
    return convert.from_bytes(string);
}

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
    wchar_t temporary[MAX_PATH] = EMPTY_CSTR;
    // The save path goes into the user's ApplicationData directory,
    // according to Microsoft's standards.  Will people like this, or
    // should I stick saves someplace easier to find, like My Documents?
    SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, temporary);
    return windowsStringToUTF8(temporary) + "\\Egoboo";
}

/// @brief Compute the binary directory path.
/// @return the binary directory path
/// @todo Error handling.
static std::wstring computeBinaryDirectoryPath() {
    wchar_t temporary[MAX_PATH] = EMPTY_CSTR;
    GetModuleFileNameW(NULL, temporary, MAX_PATH);
    PathRemoveFileSpecW(temporary);
    return temporary;
}

/// @brief Compute the working directory path.
/// @return the working directory path
/// @todo Error handling.
static std::wstring computeWorkingDirectoryPath() {
    wchar_t temporary[MAX_PATH] = EMPTY_CSTR;
    GetCurrentDirectoryW(MAX_PATH, temporary);
    return temporary;
}

/// @brief Compute the data directory path.
/// @return the data directory path
/// @throw Id::RuntimeErrorException computation of the data directory path failed
static std::string computeDataDirectoryPath() {
    std::wstring path;
    DWORD attributes;
    
    // (1) Check for data in the working directory
    path = computeWorkingDirectoryPath();
    attributes = GetFileAttributesW(path.c_str());
    if (hasAttribs(attributes, FILE_ATTRIBUTE_DIRECTORY)) {
        return windowsStringToUTF8(path);
    }
    // if (1) failed
    // (2) Check for data in the binary directory
    path = computeBinaryDirectoryPath();
    path = path + L"\\basicdat";
    attributes = GetFileAttributesW(path.c_str());
    if (hasAttribs(attributes, FILE_ATTRIBUTE_DIRECTORY)) {
        return windowsStringToUTF8(path);
    }
    throw Id::RuntimeErrorException(__FILE__, __LINE__, "unable to compute data directory path");
}

FileSystem::FileSystem(const std::string& argument0, const std::string& rootPath) :
    m_userDirectoryPath(computeUserDirectoryPath()),
    m_dataDirectoryPath(computeDataDirectoryPath()),
    m_configurationDirectoryPath(m_dataDirectoryPath){
    // No logging at this point as logging relies on the file system.
    std::cout << "game directories:" << std::endl;
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

std::string FileSystem::getConfigurationDirectoryPath() const {
    return m_configurationDirectoryPath;
}

bool FileSystem::exists(const std::string& path) {
    if (path.empty()) {
        throw std::runtime_error("empty path");
    }
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    std::wstring wpath = utf8StringToWindows(path);
    if (GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &fileInfo)) {
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
    std::wstring wpath = utf8StringToWindows(path);
    if (GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &fileInfo)) {
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
    std::wstring wpath = utf8StringToWindows(path);
    if (GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &fileInfo)) {
        return 0 == (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    } else {
        return false;
    }
}

bool FileSystem::createDirectory(const std::string& path) {
    if (path.empty()) {
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    }
    std::wstring wpath = utf8StringToWindows(path);
    if (CreateDirectoryW(wpath.c_str(), NULL))
        return true;
    return directoryExists(path);
}

bool FileSystem::removeDirectory(const std::string& path) {
    if (path.empty()) {
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    }
    std::wstring wpath = utf8StringToWindows(path);
    if (RemoveDirectoryW(wpath.c_str()))
        return true;
    return !directoryExists(path);
}

bool FileSystem::removeFile(const std::string& path) {
    if (path.empty()) {
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    }
    std::wstring wpath = utf8StringToWindows(path);
    if (DeleteFileW(wpath.c_str()))
        return true;
    return !fileExists(path);
}

bool FileSystem::copyFile(const std::string& sourcePath, const std::string& targetPath) {
    if (sourcePath.empty() || targetPath.empty()) {
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    }
    std::wstring wSourcePath = utf8StringToWindows(sourcePath);
    std::wstring wTargetPath = utf8StringToWindows(targetPath);
    return CopyFileW(wSourcePath.c_str(), wTargetPath.c_str(), false);
}

void FileSystem::findFiles(const std::string& path, const std::string& extension, const std::function<bool(const std::string&)>& onPathFound) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");

    std::string search = path + "\\*";
    if (!extension.empty())
        search += "." + extension;
    std::wstring wsearch = utf8StringToWindows(search);

    WIN32_FIND_DATAW searchData;
    HANDLE searchHandle = FindFirstFileW(wsearch.c_str(), &searchData);
    if (searchHandle == INVALID_HANDLE_VALUE)
        return;

    try {
        do {
            std::string path = windowsStringToUTF8(searchData.cFileName);
            if (path == "." || path == "..")
                continue;
            bool shouldContinue = onPathFound(path);
            if (!shouldContinue)
                break;
        } while (FindNextFileW(searchHandle, &searchData));
    } catch (...) {
        FindClose(searchHandle);
        std::rethrow_exception(std::current_exception());
    }
    FindClose(searchHandle);
}

static bool doShellExecute(const std::string path) {
    auto wpath = utf8StringToWindows(path);
    auto result = reinterpret_cast<int>(ShellExecuteW(nullptr, L"open", wpath.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
    return result > 32;
}

bool FileSystem::openFileWithDefaultApplication(const std::string& path) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    return doShellExecute(path);
}

bool FileSystem::showDirectoryInFileBrowser(const std::string& path) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    return doShellExecute(path);
}

bool FileSystem::openURLWithDefaultApplication(const std::string& url) {
    if (url.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "url is empty");
    return doShellExecute(url);
}

} // namespace Windows
} // namespace Ego
