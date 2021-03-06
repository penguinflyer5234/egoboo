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
#include "egolib/Script/Location.hpp"

namespace Ego
{
    namespace Script
    {

        Location::Location(const std::string& loadName, const size_t lineNumber) :
            _loadName(loadName), _lineNumber(lineNumber)
        {}

        Location::Location(const Location& other) :
            _loadName(other._loadName), _lineNumber(other._lineNumber)
        {
        }

        bool Location::operator==(const Location& other)
        {
            return _loadName == other._loadName
                && _lineNumber == other._lineNumber;
        }

        bool Location::operator!=(const Location& other)
        {
            return _loadName != other._loadName
                || _lineNumber != other._lineNumber;
        }

        const std::string& Location::getLoadName() const
        {
            return _loadName;
        }

        size_t Location::getLineNumber() const
        {
            return _lineNumber;
        }

    }
}