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

/// @file  egolib/timer.h
/// @brief Definitions of a timer "class" using SDL_GetTicks().

#pragma once

#include "egolib/typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct egolib_timer_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// SDL_GetTicks() always returns milli-seconds
#define TICKS_PER_SEC 1000.0f

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// a method for throttling processes using SDL_GetTicks()
struct egolib_timer_t
{
    bool    free_running;

    int     ticks_lst;
    int     ticks_now;
    int     ticks_next;
    int     ticks_diff;
};

egolib_timer_t * egolib_timer__init(egolib_timer_t *);
bool egolib_timer__throttle(egolib_timer_t * timer, float rate);
bool egolib_timer__reset(egolib_timer_t * timer, int ticks, float rate);