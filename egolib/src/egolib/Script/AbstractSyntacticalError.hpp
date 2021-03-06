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

#include "egolib/Core/Exception.hpp"
#include "egolib/Script/Location.hpp"

namespace Ego
{
namespace Script
{
using namespace std;

/**
 * @brief
 *  An abstract exception for syntactical errors.
 * @author
 *  Michael Heilmann
 */
class AbstractSyntacticalError : public Core::Exception
{

private:

    /**
     * @brief
     *  The location associated with this error.
     */
    Location _location;

protected:

    ostringstream& writeLocation(ostringstream& o) const
    {
        o << _location.getLoadName() << ": " << _location.getLineNumber()
          << " (raised in file " << getFile() << ", line " << getLine() << ")";
        return o;
    }

    /**
     * @brief
     *  Construct this exception.
     * @param file
     *  the C++ source file name associated with this error
     * @param line
     *  the line within the C++ source file associated with this error
     * @param location
     *  the location associated with this error
     *  the load name of the file associated with this error
     * @param message
     *  an additional error message to be appended to the standard error information
     */
    AbstractSyntacticalError(const char *file, int line, const Location& location) :
        Core::Exception(file, line), _location(location)
    {}

public:

    /**
     * @brief
     *  Get the location associated with the error.
     * @return
     *  the location associated with the error
     */
    const Location& getLocation() const
    {
        return _location;
    }

};

} // namespace Script
} // namespace Ego
