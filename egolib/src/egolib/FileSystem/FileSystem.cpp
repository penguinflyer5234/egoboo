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
#elif defined (ID_OSX)
#include "egolib/FileSystem/OSX/FileSystem.hpp"
#elif defined (ID_IOS)
#include "egolib/FileSystem/iOS/FileSystem.hpp"
#elif defined (ID_IOSSIMULATOR)
#include "egolib/FileSystem/iOSSimulator/FileSystem.hpp"
#endif


namespace Ego {
namespace Core {
Ego::FileSystem *CreateFunctor<Ego::FileSystem>::operator()(const std::string& argument0) const {
#if defined(ID_WINDOWS)
    return new Ego::Windows::FileSystem(argument0);
#elif defined (ID_LINUX)
    return new Ego::Linux::FileSystem(argument0);
#elif defined (ID_OSX)
    return new Ego::OSX::FileSystem(argument0);
#elif defined (ID_IOS)
    return new Ego::iOS::FileSystem(argument0);
#elif defined (ID_IOSSIMULATOR)
    return new Ego::iOSSimulator::FileSystem(argument0);
#endif
}
} // namespace Core

FileSystem::FileSystem() {
    /* Intentionally empty. */
}

FileSystem::~FileSystem() {
    /* Intentionally empty. */
}

} // namespace Ego
