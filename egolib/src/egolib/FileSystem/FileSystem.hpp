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

/// @file egolib/VFS/FileSystem.cpp
/// @brief Common interface of all file systems
/// @author Michael Heilmann

#pragma once

#include "egolib/Core/Singleton.hpp"

namespace Ego {

struct FileSystem : public Core::Singleton<FileSystem> {
protected:
    friend Core::Singleton<FileSystem>::CreateFunctorType;
    friend Core::Singleton<FileSystem>::DestroyFunctorType;

protected:
    /// @brief Construct this file system.
    /// @remark Intentionally protected.
    FileSystem();

    /// @brief Destruct this file system.
    /// @remark Intentionally protected
    virtual ~FileSystem();

public:
    /// @brief Get the user directory path
    /// @return the user directory path
    virtual std::string getUserDirectoryPath() const = 0;

    /// @brief Get the data directory path.
    /// @return the data directory path
    virtual std::string getDataDirectoryPath() const = 0;

    /// @brief Get the configuration directory path.
    /// @return the configuration directory path
    virtual std::string getConfigurationDirectoryPath() const = 0;

public:
    /// @brief Get if a file or directory exists.
    /// @param path the path
    /// @return @a true if the file or the directory exists, @a false otherwise
    virtual bool exists(const std::string& path) = 0;

    /// @brief Get if a directory exists.
    /// @param path the path
    /// @return @a true if the directory exists, @a false otherwise
    virtual bool directoryExists(const std::string& path) = 0;

    /// @brief Get if a file exists.
    /// @param path the path
    /// @return @a true if the file exists, @a false otherwise
    virtual bool fileExists(const std::string& path) = 0;

public:
    /// @brief Ensure a directory exists.
    /// @param path the path of the directory
    /// @return @a true if the directory exists, @a false otherwise
    /// @remark All intermediate directories must exist;
    /// this function will only create the final directory in the specified path.
    /// @throw Id::RuntimeErrorException the path is empty
    virtual bool createDirectory(const std::string& path) = 0;

    /// @brief Ensure a directory does not exist.
    /// @param path the path of the directory
    /// @return @a true if the directory does not exist, @a false otherwise
    /// @remark This function will only remove the final directory in the path.
    /// @remark This function will only remove the final directory in the path if it is empty.
    /// @throw Id::RuntimeErrorException the path is empty
    virtual bool removeDirectory(const std::string& path) = 0;

    /// @brief Ensure a file does not exist.
    /// @param pathname the pathname of the file
    /// @return @a true if the file does not exist, @a false otherwise
    /// @throw Id::RuntimeErrorException the path is empty
    virtual bool removeFile(const std::string& path) = 0;

public:
    /// @brief Copy a source file to a target file.
    /// @param sourcePath the source path
    /// @param targetPath the target path
    /// @return @a true if copying succeeded, @a false otherwise
    /// @throw Id::RuntimeErrorException the source path or the target path is empty
    /// @remark If the source file does not exist or the target file already exists, then this method fails.
    virtual bool copyFile(const std::string& sourcePath, const std::string& targetPath) = 0;
    
    /// @brief Copy all files in a directory into another directory.
    /// @param sourcePath the source path
    /// @param targetPath the target path
    /// @return @a true if copying succeeded, @a false otherwise
    /// @throw Id::RuntimeErrorException the source path or the target path is empty
    /// @remark If the target directory does not exist, it is created.
    virtual bool copyDirectory(const std::string& sourcePath, const std::string& targetPath);
    
    virtual bool removeDirectoryAndContents(const std::string& path, bool recursive);
    
    /// @brief Find files in a directory.
    /// @param path the path to search in
    /// @param extension the filename extension to search for; may be empty for all files
    /// @param onPathFound the function called when a file has matched; the return value determines if the search continues
    virtual void findFiles(const std::string& path, const std::string& extension, const std::function<bool(const std::string& path)>& onPathFound) = 0;
    
    /// @brief Find files in a directory.
    /// @param path the path to search in
    /// @param extension the filename extension to search for; may be empty for all files
    /// @return the list of files in the given directory that match
    virtual std::vector<std::string> findFiles(const std::string& path, const std::string& extension);
    
    // Move these?
public:
    /// @brief Open a file with the user's default application for the file.
    /// @param path the file to open
    /// @return @a true if the file was successfully opened, @a false otherwise
    virtual bool openFileWithDefaultApplication(const std::string& path) = 0;
    
    /// @brief Open a directory with the user's default file browser.
    /// @param path the directory to show
    /// @return @a true if the directory was successfully shown, @a false otherwise
    virtual bool showDirectoryInFileBrowser(const std::string& path) = 0;
    
    /// @brief Open a URL with the user's default application for the URL.
    /// @param url the URL to open
    /// @return @a true if the URL was successfully opened, @a false otherwise
    virtual bool openURLWithDefaultApplication(const std::string& url) = 0;

}; // struct FileSystem

namespace Core {
template <>
struct CreateFunctor<Ego::FileSystem> {
    Ego::FileSystem *operator()(const std::string& argument0, const std::string& rootPath) const;
};
}

} // namespace Ego
