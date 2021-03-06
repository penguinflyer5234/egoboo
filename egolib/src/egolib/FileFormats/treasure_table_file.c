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

/// @file egolib/FileFormats/treasure_table_file.c
/// @brief Implementation of code for handling random treasure generation
/// @details

#include "egolib/FileFormats/treasure_table_file.h"

#include "egolib/fileutil.h"
#include "egolib/strutil.h"

#include "egolib/_math.h"      //For randomization

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
treasure_table_t treasureTableList[MAX_TABLES];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//Private functions
static void load_one_treasure_table_vfs(ReadContext& ctxt, treasure_table_t* new_table );
static void add_object_to_table( treasure_table_t table[], const char *name );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void add_object_to_table( treasure_table_t table[], const char *name )
{
    //ZF> Adds a new treasure object to the specified treasure table

    //Avoid null pointers
    if ( NULL == table ) return;

    //Make sure there is enough size to add one more
    if ( table->size + 1 >= TREASURE_TABLE_SIZE )
    {
        log_warning( "No more room to add object (%s) to table, consider increasing TREASURE_TABLE_SIZE (currently %i)\n", name, TREASURE_TABLE_SIZE );
        return;
    }

    //Add the element to the list
    strncpy( table->object_list[ table->size ], name, SDL_arraysize( table->object_list[ table->size ] ) );
    table->size++;
}

//--------------------------------------------------------------------------------------------
void load_one_treasure_table_vfs(ReadContext& ctxt, treasure_table_t* new_table )
{
    //ZF> Creates and loads a treasure table from the specified file until a :END is encountered
    new_table->size = 0;

    //Keep adding objects into the table until we encounter a :END
    while (ctxt.skipToColon(false))
    {
        STRING temporary;
        // We need to distinguish between regular names and references starting with '%'.
        ctxt.skipWhiteSpaces();
        if (ctxt.is('%'))
        {
            ctxt.next();
            temporary[0] = '%';
            vfs_read_name(ctxt, temporary + 1, SDL_arraysize(temporary) - 1);
        }
        else
        {
            vfs_read_name(ctxt, temporary, SDL_arraysize(temporary));
        }


        //Check if we reached the end of this table
        if ( 0 == strcmp( temporary, "END" ) ) break;

        //Nope, add one more to the table
        add_object_to_table( new_table, temporary);
    }
}

//--------------------------------------------------------------------------------------------
egolib_rv init_random_treasure_tables_vfs( const char* filepath )
{
    //ZF> This loads all the treasure tables from randomtreasure.txt
    int num_table;

    // Try to open a context.
    ReadContext ctxt(filepath);
    if (!ctxt.ensureOpen())
    {
        log_warning("unable to load random treasure tables file `%s`\n", filepath);
        return rv_error;
    }

    //Load each treasure table
    num_table = 0;
    while (ctxt.skipToColon(true))
    {
        //Load the name of this table
        STRING temporary;
        vfs_read_name(ctxt, temporary, SDL_arraysize(temporary));

        //Stop here if we are already full
        if (num_table >= MAX_TABLES)
        {
            log_warning("Cannot load random treasure table: %s (We only support up to %i tables, consider increasing MAX_TABLES) \n", temporary, MAX_TABLES);
            break;
        }

        snprintf(treasureTableList[num_table].table_name, SDL_arraysize(treasureTableList[num_table].table_name), "%%%s", temporary);

        //Load all objects in this treasure table
        treasureTableList[num_table].size = 0;
        load_one_treasure_table_vfs( ctxt, &treasureTableList[num_table] );
        num_table++;
    }
    return rv_success;
}

//--------------------------------------------------------------------------------------------
egolib_rv get_random_treasure( char * buffer, size_t buffer_length )
{
    //ZF> Gets the name for a treasure randomly selected from the specified treasure table
    //    This function effectively "converts" a table name into a random element from that table

    IPair loc_rand;
    size_t i;
    int treasure_index;

    bool found = false;
    STRING tmp_buffer;

    // Trap invalid strings
    if ( 0 == buffer_length || INVALID_CSTR( buffer ) ) return rv_error;

    // make a local copy of the string
    strncpy( tmp_buffer, buffer, SDL_arraysize( tmp_buffer ) );

    // Iterate through every treasure table until we find the one we want
    found = false;
    for ( i = 0; i < MAX_TABLES; i++ )
    {
        //Continue looking until we find the correct table
        if ( 0 != strcmp( treasureTableList[i].table_name, tmp_buffer ) ) continue;

        //Pick a random number between 0 and the length of the table to get a random element out of the array
        loc_rand.base = 0;
        loc_rand.rand = treasureTableList[i].size;
        treasure_index = generate_irand_pair( loc_rand );
        strncpy( tmp_buffer, treasureTableList[i].object_list[treasure_index], buffer_length );

        //See if it is an actual random object or a reference to a different random table
        if ( '%' != tmp_buffer[0] )
        {
            found = true;
        }
        else
        {
            if ( rv_success == get_random_treasure( tmp_buffer, buffer_length ) )
            {
                found = true;
            }
        }
    }

    //Could not find anything
    if ( found )
    {
        // copy the local string to the output
        strncpy( buffer, tmp_buffer, buffer_length );
    }
    else
    {
        // give a warning
        tmp_buffer[0] = CSTR_END;
        log_warning( "Could not find treasure table: %s!\n", buffer );
    }

    return found ? rv_success : rv_fail;
}
