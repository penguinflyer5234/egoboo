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

/// @file egolib/process.h

#pragma once

#include "egolib/typedef.h"
#include "egolib/state_machine.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct s_process_instance;
    typedef struct s_process_instance process_t;

//--------------------------------------------------------------------------------------------
// MACROS AND ENUMS
//--------------------------------------------------------------------------------------------

/// grab a pointer to the process_t of any object that "inherits" this type
#define PROC_PBASE(PTR) (&( (PTR)->base ))

// The various states that a process can occupy
    enum e_process_states
    {
        proc_invalid  = ego_state_invalid,
        proc_begin    = ego_state_begin,
        proc_entering = ego_state_entering,
        proc_running  = ego_state_running,
        proc_leaving  = ego_state_leaving,
        proc_finish   = ego_state_finish
    };

// this typedef must be after the enum definition or gcc has a fit
    typedef enum e_process_states process_state_t;

//--------------------------------------------------------------------------------------------
// CLASS DEFINITIONS
//--------------------------------------------------------------------------------------------

/// A rudimantary implementation of "non-preemptive multitasking" in Egoboo.
/// @details All other process types "inherit" from this one

    struct s_process_instance
    {
        bool          valid;
        bool          paused;
        bool          killme;
        bool          terminated;
        process_state_t state;
        double          frameDuration;
    };

    process_t * process_init( process_t * proc );
    bool      process_start( process_t * proc );
    bool      process_kill( process_t * proc );
    bool      process_validate( process_t * proc );
    bool      process_terminate( process_t * proc );
    bool      process_pause( process_t * proc );
    bool      process_resume( process_t * proc );
    bool      process_running( process_t * proc );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}

#endif
