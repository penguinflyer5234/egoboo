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

/// @file egolib/FileFormats/quest_file.c
/// @brief Handles functions that modify quest.txt files and the players quest log

#include "egolib/FileFormats/quest_file.h"

#include "egolib/IDSZ_map.h"
#include "egolib/log.h"

#include "egolib/fileutil.h"
#include "egolib/vfs.h"
#include "egolib/strutil.h"

#include "egolib/_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
std::shared_ptr<ConfigFile> quest_file_open(const char *player_directory)
{
    if (!player_directory || !strlen(player_directory))
    {
        return nullptr;
    }
    // Figure out the file path
    std::string newLoadName = std::string(player_directory) + "/quest.txt";
    std::shared_ptr<ConfigFile> configFile = ConfigFileParser().parse(newLoadName);
    if (!configFile)
    {
        configFile = std::make_shared<ConfigFile>(newLoadName);
    }
    return configFile;
}

//--------------------------------------------------------------------------------------------
egolib_rv quest_file_export(std::shared_ptr<ConfigFile> file)
{
    if (!file)
    {
        return rv_error;
    }
    if (!ConfigFileUnParser().unparse(file))
    {
        log_warning("%s:%d: unable to export quest file `%s`\n", __FILE__, __LINE__, file->getFileName().c_str());
        return rv_fail;
    }
    return rv_success;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egolib_rv quest_log_download_vfs(IDSZ_node_t *quest_log, size_t quest_log_len, const char* player_directory)
{
    /// @author ZF
    /// @details Reads a quest.txt for a player and turns it into a data structure
    ///               we can use. If the file isn't found, the quest log will be initialized as empty.

    egolib_rv retval = rv_success;

    if ( NULL == quest_log ) return rv_error;

    // blank out the existing map
    idsz_map_init( quest_log, quest_log_len );

    // Figure out the file path
    std::string newLoadName = std::string(player_directory) + "/quest.txt";

    // Try to open a context
    ReadContext ctxt(newLoadName);
    if (!ctxt.ensureOpen()) return rv_error;
    // Load each IDSZ
    retval = rv_success;
    while (ctxt.skipToColon(true))
    {
        egolib_rv rv;

        IDSZ idsz = ctxt.readIDSZ();
        int  level = ctxt.readInt();

        // Try to add a single quest to the map
        rv = idsz_map_add( quest_log, quest_log_len, idsz, level );

        // Stop here if it failed
        if ( rv_error == rv )
        {
            log_warning("quest_log_download_vfs() - Encountered an error while trying to add a quest. (%s)\n", newLoadName.c_str());
            retval = rv;
            break;
        }
        else if ( rv_fail == rv )
        {
            log_warning( "quest_log_download_vfs() - Unable to load all quests. (%s)\n", newLoadName.c_str());
            retval = rv;
            break;
        }
    }
    return retval;
}

//--------------------------------------------------------------------------------------------
egolib_rv quest_log_upload_vfs( IDSZ_node_t * quest_log, size_t quest_log_len, const char *player_directory )
{
    /// @author ZF
    /// @details This exports quest_log data into a quest.txt file
    vfs_FILE *filewrite;
    int iterator;
    IDSZ_node_t *pquest;

    if ( NULL == quest_log ) return rv_error;

    // Write a new quest file with all the quests
    filewrite = vfs_openWrite( player_directory );
    if ( NULL == filewrite )
    {
        log_warning( "Cannot create quest file! (%s)\n", player_directory );
        return rv_fail;
    }

    vfs_printf( filewrite, "// This file keeps order of all the quests for this player\n" );
    vfs_printf( filewrite, "// The number after the IDSZ shows the quest level. %i means it is completed.", QUEST_BEATEN );

    // Iterate through every element in the IDSZ map
    iterator = 0;
    pquest = idsz_map_iterate( quest_log, quest_log_len, &iterator );
    while ( pquest != NULL )
    {
        // Write every single quest to the quest log
        vfs_printf( filewrite, "\n:[%4s] %i", undo_idsz( pquest->id ), pquest->level );

        // Get the next element
        pquest = idsz_map_iterate( quest_log, quest_log_len, &iterator );
    }

    // Clean up and return
    vfs_close( filewrite );
    return rv_success;
}

//--------------------------------------------------------------------------------------------
int quest_log_set_level( IDSZ_node_t * quest_log, size_t quest_log_len, IDSZ idsz, int level )
{
    /// @author ZF
    /// @details This function will set the quest level for the specified quest
    ///          and return the new quest_level. It will return QUEST_NONE if the quest was
    ///          not found.

    IDSZ_node_t *pquest = NULL;

    // find the quest
    pquest = idsz_map_get( quest_log, quest_log_len, idsz );
    if ( NULL == pquest ) return QUEST_NONE;

    // make a copy of the quest's level
    pquest->level = level;

    return level;
}

//--------------------------------------------------------------------------------------------
int quest_log_adjust_level( IDSZ_node_t * quest_log, size_t quest_log_len, IDSZ idsz, int adjustment )
{
    /// @author ZF
    /// @details This function will modify the quest level for the specified quest with adjustment
    ///          and return the new quest_level total. It will return QUEST_NONE if the quest was
    ///          not found or if it was already beaten.

    int          src_level = QUEST_NONE;
    int          dst_level = QUEST_NONE;
    IDSZ_node_t *pquest    = NULL;

    // find the quest
    pquest = idsz_map_get( quest_log, quest_log_len, idsz );
    if ( NULL == pquest ) return QUEST_NONE;

    // make a copy of the quest's level
    src_level = pquest->level;

    // figure out what the dst_level is
    if ( QUEST_BEATEN == src_level )
    {
        // Don't modify quests that are already beaten
        dst_level = src_level;
    }
    else
    {
        // if the quest "doesn't exist" make the src_level 0
        if ( QUEST_NONE   == src_level ) src_level = 0;

        // Modify the quest level for that specific quest
        if ( adjustment == QUEST_MAXVAL ) dst_level = QUEST_BEATEN;
        else                             dst_level = std::max( 0, src_level + adjustment );

        // set the quest level
        pquest->level = dst_level;
    }

    return dst_level;
}

//--------------------------------------------------------------------------------------------
int quest_log_get_level( IDSZ_node_t * quest_log, size_t quest_log_len, IDSZ idsz )
{
    /// @author ZF
    /// @details Returns the quest level for the specified quest IDSZ.
    ///          It will return QUEST_NONE if the quest was not found or if the quest was beaten.

    IDSZ_node_t *pquest;

    pquest = idsz_map_get( quest_log, quest_log_len, idsz );
    if ( NULL == pquest ) return QUEST_NONE;

    return pquest->level;
}

//--------------------------------------------------------------------------------------------
egolib_rv quest_log_add( IDSZ_node_t * quest_log, size_t quest_log_len, IDSZ idsz, int level )
{
    /// @author ZF
    /// @details This adds a new quest to the quest log. If the quest is already in there, the higher quest
    ///          level of either the old and new one will be kept.

    return idsz_map_add( quest_log, quest_log_len, idsz, level );
}
