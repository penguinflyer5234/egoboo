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

#pragma once

#include "egolib/FileSystem/FileSystem.hpp"

namespace Ego {
namespace Linux {

struct FileSystem : Ego::FileSystem {
private:
    std::string m_userDirectoryPath;
    std::string m_dataDirectoryPath;
    std::string m_binaryDirectoryPath;
    std::string m_configurationDirectoryPath;

public:
    /// @brief Construct this Linux file system.
    /// @param argument0 the first argument of the command line
    FileSystem(const std::string& argument);

public:
    /** @copydoc Ego::FileSystem::getUserDirectoryPath() */
    std::string getUserDirectoryPath() const override;

    /** @copydoc Ego::FileSystem::getDataDirectoryPath() */
    std::string getDataDirectoryPath() const override;

    /** @copydoc Ego::FileSystem::getBinaryDirectoryPath() */
    std::string getBinaryDirectoryPath() const override;

    /** @copydoc Ego::FileSystem::getConfigurationDirectoryPath() */
    std::string getConfigurationDirectoryPath() const override;

public:
    /** @copydoc Ego::FileSystem::exists */
    bool exists(const std::string& path) override;

    /** @copydoc Ego::FileSystem::directoryExists */
    bool directoryExists(const std::string& path) override;

    /** @copydoc Ego::FileSystem::fileExists */
    bool fileExists(const std::string& path) override;

public:
    /** @copydoc Ego::FileSystem::createDirectory */
    bool createDirectory(const std::string& path) override;

    /** @copydoc Ego::FileSystem::removeDirectory */
    bool removeDirectory(const std::string& path) override;

    /** @copydoc Ego::FileSystem::removeFile */
    bool removeFile(const std::string& path) override;

public:
    /** @copydoc Ego::FileSystem::copyFile */
    bool copyFile(const std::string& sourcePath, const std::string& targetPath) override;
};

} // namespace Linux
} // namespace Ego
