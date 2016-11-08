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

#include "egolib/FileSystem/Linux/FileSystem.hpp"

#include <fstream>

#include <dirent.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

namespace Ego {
namespace Linux {

static bool executeOpen(const std::string &path) {
    pid_t childPID;
    const char *argv[] = {
#ifdef ID_OSX
        "open",
#else
        "xdg-open",
#endif
        path.c_str(),
        nullptr
    };
    
    if (int err = posix_spawnp(&childPID, argv[0], nullptr, nullptr, const_cast<char **>(argv), environ)) {
        if (err == ENOENT) {
            std::string message = std::string("The executable '") + argv[0] + "' is required to open '" + path + "'.";
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Missing executable", message.c_str(), SDL_GL_GetCurrentWindow());
        }
        return false;
    }
    
    int childStatus;
    if (!waitpid(childPID, &childStatus, 0))
        return false;
    return WIFEXITED(childStatus) && WEXITSTATUS(childStatus) == 0;
}

FileSystem::FileSystem(const std::string& argument0, const std::string& rootPath) {
    if (!rootPath.empty() && directoryExists(rootPath + "/basicdat")) {
        m_dataDirectoryPath = rootPath;
        m_configurationDirectoryPath = rootPath;
    } else {
#if defined(FS_HAS_PATHS)
        m_dataDirectoryPath = FS_DATA_PATH;
        m_configurationDirectoryPath = FS_CONFIG_PATH;
#else
        // these are read-only directories
        char* applicationPath = SDL_GetBasePath();
        if (applicationPath) {
            m_dataDirectoryPath = applicationPath;
            SDL_free(applicationPath);
            if (!directoryExists(m_dataDirectoryPath + "/basicdat"))
                m_dataDirectoryPath = "./";
        } else {
            m_dataDirectoryPath = "./";
        }
        m_configurationDirectoryPath = m_dataDirectoryPath;
#endif
    }
    
    // grab the user's home directory
    char *envVar = getenv("XDG_DATA_HOME");
    if (envVar) {
        m_userDirectoryPath = std::string(envVar) + "/egoboo/";
    } else {
        envVar = getenv("HOME");
        if (!envVar)
            throw Id::EnvironmentErrorException(__FILE__, __LINE__, "Linux FileSystem", "neither XDG_DATA_HOME nor HOME environment variable defined!");
        m_userDirectoryPath = std::string(envVar) + "/.local/share/egoboo/";
    }
    
    // the log file cannot be started until there is a user data path to dump the file into
    // so dump this debug info to stdout
    printf("Game directories are:\n"
           "\tData: %s\n"
           "\tUser: %s\n"
           "\tConfiguration: %s\n",
           m_dataDirectoryPath.c_str(), m_userDirectoryPath.c_str(), m_configurationDirectoryPath.c_str());
}

bool FileSystem::exists(const std::string &path) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    
    struct stat status;
    if (stat(path.c_str(), &status)) {
        errno = 0;
        return false;
    }
    return S_ISDIR(status.st_mode) || S_ISREG(status.st_mode);
}

bool FileSystem::directoryExists(const std::string &path) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    
    struct stat status;
    if (stat(path.c_str(), &status)) {
        errno = 0;
        return false;
    }
    return S_ISDIR(status.st_mode);
}

bool FileSystem::fileExists(const std::string &path) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    
    struct stat status;
    if (stat(path.c_str(), &status)) {
        errno = 0;
        return false;
    }
    return S_ISREG(status.st_mode);
}

bool FileSystem::createDirectory(const std::string &path) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    
    if (mkdir(path.c_str(), 0777)) {
        errno = 0;
        return directoryExists(path);
    }
    return true;
}

bool FileSystem::removeDirectory(const std::string &path) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    
    if (rmdir(path.c_str())) {
        errno = 0;
        return !directoryExists(path);
    }
    return true;
}

bool FileSystem::removeFile(const std::string &path) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    
    if (unlink(path.c_str())) {
        errno = 0;
        return !fileExists(path);
    }
    return true;
}

bool FileSystem::copyFile(const std::string &sourcePath, const std::string &targetPath) {
    if (sourcePath.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "sourcePath is empty");
    if (targetPath.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "targetPath is empty");
    
    if (!exists(sourcePath) || exists(targetPath))
        return false;
    
    std::ifstream sourceStream(sourcePath, std::ios_base::binary);
    if (!sourceStream.is_open())
        return false;
    
    std::ofstream targetStream(targetPath, std::ios_base::binary);
    if (!targetStream.is_open())
        return false;
    
    constexpr size_t bufferSize = 4096;
    char buffer[bufferSize];
    
    while (sourceStream) {
        sourceStream.read(buffer, bufferSize);
        size_t numBytesRead = sourceStream.gcount();
        targetStream.write(buffer, numBytesRead);
        if (!targetStream)
            return false;
    }
    
    return true;
}

void FileSystem::findFiles(const std::string &path, const std::string &extension, const std::function<bool (const std::string &)> &onPathFound) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    
    struct CloseDirectory {
        CloseDirectory(DIR *d) :dir(d) {}
        ~CloseDirectory() { closedir(dir); }
        DIR *dir;
    };
    
    DIR *directory = opendir(path.c_str());
    if (!directory) {
        errno = 0;
        return;
    }
    CloseDirectory closeDir(directory);
    
    while (true) {
        struct dirent *entry = readdir(directory);
        if (!entry)
            break;
        std::string entryString = entry->d_name;
        if (entryString == "." || entryString == "..")
            continue;
        
        if (!extension.empty()) {
            auto pos = entryString.rfind('.');
            if (pos == std::string::npos)
                continue;
            
            auto entryExtension = entryString.substr(pos + 1);
            if (entryExtension != extension)
                continue;
        }
        bool shouldContinue = onPathFound(entryString);
        if (!shouldContinue)
            break;
    }
}

bool FileSystem::openFileWithDefaultApplication(const std::string &path) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");

    return executeOpen(path);
}

bool FileSystem::showDirectoryInFileBrowser(const std::string &path) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    
    return executeOpen(path);
}

bool FileSystem::openURLWithDefaultApplication(const std::string &url) {
    if (url.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "url is empty");
    
    return executeOpen(url);
}

} // namespace Linux
} // namespace Ego
