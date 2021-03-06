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

/// @file game/script_implementation.c
/// @brief implementation of the internal egoscript functions
/// @details These are the c-functions that are called to implement the egoscript commsnds
/// At some pont, the non-trivial commands in this file will be broken out to discrete functions
/// and wrapped by some external language (like Lua) using something like SWIG, and a cross compiler written.
/// The current code is about 3/4 of the way toward this goal.
/// The functions below will then be replaced with stub calls to the "real" functions.

#include "game/script_implementation.h"
#include "game/game.h"
#include "egolib/AI/AStar.h"
#include "egolib/Graphics/ModelDescriptor.hpp"
#include "game/renderer_2d.h"
#include "game/Entities/_Include.hpp"
#include "game/mesh.h"
#include "game/char.h"
#include "game/Module/Module.hpp"
#include "game/Module/Passage.hpp"

//--------------------------------------------------------------------------------------------
// wrap generic bitwise conversion macros
//--------------------------------------------------------------------------------------------
BIT_FIELD bool_to_bit( bool val )
{
    return BOOL_TO_BIT( val );
}

//--------------------------------------------------------------------------------------------
bool bit_to_bool( BIT_FIELD val )
{
    return BIT_TO_BOOL( val );
}

//--------------------------------------------------------------------------------------------
Uint8  BIT_FIELD_clip_to_08_bits( BIT_FIELD val )  { return val & 0xFF;      }

//--------------------------------------------------------------------------------------------
Uint16 BIT_FIELD_clip_to_16_bits( BIT_FIELD val )  { return val & 0xFFFF;     }

//--------------------------------------------------------------------------------------------
Uint32 BIT_FIELD_clip_to_24_bits( BIT_FIELD val )  { return val & 0xFFFFFF;   }

//--------------------------------------------------------------------------------------------
Uint32 BIT_FIELD_clip_to_32_bits( BIT_FIELD val )  { return val & 0xFFFFFFFF; }

//--------------------------------------------------------------------------------------------
// wrap the BIT_FIELD macros, since lua doesn't recognize bitwise functions
//--------------------------------------------------------------------------------------------

BIT_FIELD BIT_FIELD_fill( BIT_FIELD val )
{
    return FULL_BIT_FIELD;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD BIT_FIELD_empty( BIT_FIELD val )
{
    return EMPTY_BIT_FIELD;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD BIT_FIELD_set_bits( BIT_FIELD val, BIT_FIELD bits )
{
    BIT_FIELD new_val = val;

    SET_BIT( new_val, bits );

    return new_val;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD BIT_FIELD_unset_bits( BIT_FIELD val, BIT_FIELD bits )
{
    BIT_FIELD new_val = val;

    UNSET_BIT( new_val, bits );

    return new_val;
}

//--------------------------------------------------------------------------------------------
bool BIT_FIELD_has_some_bits( BIT_FIELD val, BIT_FIELD test )
{
    return HAS_SOME_BITS( val, test );
}

//--------------------------------------------------------------------------------------------
bool BIT_FIELD_has_all_bits( BIT_FIELD val, BIT_FIELD test )
{
    return HAS_ALL_BITS( val, test );
}

//--------------------------------------------------------------------------------------------
bool BIT_FIELD_has_no_bits( BIT_FIELD val, BIT_FIELD test )
{
    return HAS_NO_BITS( val, test );
}

//--------------------------------------------------------------------------------------------
bool BIT_FIELD_missing_bits( BIT_FIELD val, BIT_FIELD test )
{
    return HAS_SOME_BITS( val, test ) && !HAS_ALL_BITS( val, test );
}

//--------------------------------------------------------------------------------------------
BIT_FIELD BIT_FIELD_set_all_bits( BIT_FIELD val, BIT_FIELD bits )
{
    BIT_FIELD new_val = val;

    SET_BIT( new_val, bits );

    return new_val;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD BIT_FIELD_clear_all_bits( BIT_FIELD val, BIT_FIELD bits )
{
    BIT_FIELD new_val = val;

    UNSET_BIT( new_val, bits );

    return new_val;
}

//--------------------------------------------------------------------------------------------
bool BIT_FIELD_test_all_bits( BIT_FIELD val, BIT_FIELD bits )
{
    return HAS_ALL_BITS( val, bits );
}

//--------------------------------------------------------------------------------------------
// line_of_sight_info_t
//--------------------------------------------------------------------------------------------

bool line_of_sight_blocked( line_of_sight_info_t * plos )
{
    bool mesh_hit = line_of_sight_with_mesh( plos );

    //if ( mesh_hit )
    //{
    //    plos->x1 = (plos->collide_x + 0.5f) * GRID_FSIZE;
    //    plos->y1 = (plos->collide_y + 0.5f) * GRID_FSIZE;
    //}

    //bool chr_hit = line_of_sight_with_characters( plos );

    return mesh_hit /*|| chr_hit*/;
}

//--------------------------------------------------------------------------------------------
bool line_of_sight_with_mesh( line_of_sight_info_t * plos )
{
    int Dx, Dy;
    int ix, ix_stt, ix_end;
    int iy, iy_stt, iy_end;

    int Dbig, Dsmall;
    int ibig, ibig_stt, ibig_end;
    int ismall, ismall_stt, ismall_end;
    int dbig, dsmall;
    int TwoDsmall, TwoDsmallMinusTwoDbig, TwoDsmallMinusDbig;

    bool steep;

    if ( NULL == plos ) return false;

    //is there any point of these calculations?
    if ( EMPTY_BIT_FIELD == plos->stopped_by ) return false;

    ix_stt = std::floor( plos->x0 / GRID_FSIZE ); /// @todo We have a projection function for that.
    ix_end = std::floor( plos->x1 / GRID_FSIZE );

    iy_stt = std::floor( plos->y0 / GRID_FSIZE ); /// @todo We have a projection function for that.
    iy_end = std::floor( plos->y1 / GRID_FSIZE );

    Dx = plos->x1 - plos->x0;
    Dy = plos->y1 - plos->y0;

    steep = (std::abs(Dy) >= std::abs(Dx));

    // determine which are the big and small values
    if ( steep )
    {
        ibig_stt = iy_stt;
        ibig_end = iy_end;

        ismall_stt = ix_stt;
        ismall_end = ix_end;
    }
    else
    {
        ibig_stt = ix_stt;
        ibig_end = ix_end;

        ismall_stt = iy_stt;
        ismall_end = iy_end;
    }

    // set up the big loop variables
    dbig = 1;
    Dbig = ibig_end - ibig_stt;
    if ( Dbig < 0 )
    {
        dbig = -1;
        Dbig = -Dbig;
        ibig_end--;
    }
    else
    {
        ibig_end++;
    }

    // set up the small loop variables
    dsmall = 1;
    Dsmall = ismall_end - ismall_stt;
    if ( Dsmall < 0 )
    {
        dsmall = -1;
        Dsmall = -Dsmall;
    }

    // pre-compute some common values
    TwoDsmall             = 2 * Dsmall;
    TwoDsmallMinusTwoDbig = TwoDsmall - 2 * Dbig;
    TwoDsmallMinusDbig    = TwoDsmall - Dbig;

    TileIndex fan_last = TileIndex::Invalid;
    for ( ibig = ibig_stt, ismall = ismall_stt;  ibig != ibig_end;  ibig += dbig )
    {
        if ( steep )
        {
            ix = ismall;
            iy = ibig;
        }
        else
        {
            ix = ibig;
            iy = ismall;
        }

        // check to see if the "ray" collides with the mesh
        TileIndex fan = ego_mesh_t::get_tile_int(_currentModule->getMeshPointer(), PointGrid(ix, iy));
        if (TileIndex::Invalid != fan && fan != fan_last )
        {
            Uint32 collide_fx = ego_mesh_t::test_fx( _currentModule->getMeshPointer(), fan, plos->stopped_by );
            // collide the ray with the mesh

            if ( EMPTY_BIT_FIELD != collide_fx )
            {
                plos->collide_x  = ix;
                plos->collide_y  = iy;
                plos->collide_fx = collide_fx;

                return true;
            }

            fan_last = fan;
        }

        // go to the next step
        if ( TwoDsmallMinusDbig > 0 )
        {
            TwoDsmallMinusDbig += TwoDsmallMinusTwoDbig;
            ismall             += dsmall;
        }
        else
        {
            TwoDsmallMinusDbig += TwoDsmall;
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------
bool line_of_sight_with_characters( line_of_sight_info_t * plos )
{

    if ( NULL == plos ) return false;

    //TODO: do line/character intersection

    return false;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool AddWaypoint( waypoint_list_t * plst, CHR_REF ichr, float pos_x, float pos_y )
{
    // AddWaypoint( tmpx = "x position", tmpy = "y position" )
    /// @author ZZ
    /// @details This function tells the character where to move next

    bool returncode;

    if ( NULL == plst ) return false;

#if defined(_DEBUG) && defined(DEBUG_WAYPOINTS)
    fvec2_t loc_pos;
    fvec3_t nrm;
    float   pressure;

    // init the vector with the desired position
    loc_pos.x = pos_x;
    loc_pos.y = pos_y;

    // is this a safe position?
    returncode = false;

    ObjectProfile * profile = chr_get_ppro( ichr );
    if ( nullptr != profile )
    {
        if ( CAP_INFINITE_WEIGHT == profile->getWeight() || !ego_mesh_hit_wall( _currentModule->getMeshPointer(), loc_pos.v, pchr->bump.size, pchr->stoppedby, nrm.v, &pressure, NULL ) )
        {
            // yes it is safe. add it.
            returncode = waypoint_list_push( plst, pos_x, pos_y );
        }
        else
        {
            // no it is not safe. what to do? nothing, or add the current position?
            //returncode = waypoint_list_push( plst, pchr->loc_pos.x, pchr->loc_pos.y );

            log_warning( "%s - failed to add a waypoint because object was \"inside\" a wall.\n"
                         "\tcharacter %d (\"%s\", \"%s\")\n"
                         "\tWaypoint index %d\n"
                         "\tWaypoint location (in tiles) <%f,%f>\n"
                         "\tWall normal <%1.4f,%1.4f>\n"
                         "\tPressure %f\n",
                         __FUNCTION__,
                         ichr, pchr->Name, profile->getClassName().c_str(),
                         plst->head,
                         loc_pos.x / GRID_FSIZE, loc_pos.y / GRID_FSIZE,
                         nrm.x, nrm.y,
                         SQRT( pressure ) / GRID_FSIZE );
        }
    }
#else
    returncode = waypoint_list_push( plst, pos_x, pos_y );
#endif

    return returncode;
}

//--------------------------------------------------------------------------------------------
bool FindPath( waypoint_list_t * plst, Object * pchr, float dst_x, float dst_y, bool * used_astar_ptr )
{
    // FindPath
    /// @author ZF
    /// @details Ported the A* path finding algorithm by birdsey and heavily modified it
    /// This function adds enough waypoints to get from one point to another

    int src_ix, src_iy;
    int dst_ix, dst_iy;
    line_of_sight_info_t los_info;
    bool straight_line;
    bool returncode = false;

    if ( NULL != used_astar_ptr )
    {
        *used_astar_ptr = false;
    }

    //Our current position
    src_ix = ( int )pchr->getPosX() / GRID_ISIZE;
    src_iy = ( int )pchr->getPosY() / GRID_ISIZE;

    //Destination position
    dst_ix = dst_x / GRID_ISIZE;
    dst_iy = dst_y / GRID_ISIZE;

    //Always clear any old waypoints
    waypoint_list_clear( plst );

    //Don't do need to do anything if there is no need to move
    if ( src_ix == dst_ix && src_iy == dst_iy ) return false;

    returncode = false;

    //setup line of sight data for source
    los_info.stopped_by = pchr->stoppedby;
    los_info.x0 = pchr->getPosX();
    los_info.y0 = pchr->getPosY();
    los_info.z0 = 0;

    //setup line of sight to target
    los_info.x1 = dst_x;
    los_info.y1 = dst_y;
    los_info.z1 = 0;

    // test for the simple case... a straight line
    straight_line = !line_of_sight_blocked( &los_info );

    if ( !straight_line )
    {
#ifdef DEBUG_ASTAR
        printf( "Finding a path from %d,%d to %d,%d: \n", src_ix, src_iy, dst_ix, dst_iy );
#endif
        //Try to find a path with the AStar algorithm
        if ( AStar_find_path( _currentModule->getMeshPointer(), pchr->stoppedby, src_ix, src_iy, dst_ix, dst_iy ) )
        {
            returncode = AStar_get_path( dst_x, dst_y, plst );
        }

        if ( NULL != used_astar_ptr )
        {
            *used_astar_ptr = true;
        }
    }

    //failed to find a path
    if ( !returncode )
    {
        // just use a straight line path
        waypoint_list_push( plst, dst_x, dst_y );
    }

    return returncode;
}

//--------------------------------------------------------------------------------------------
bool Compass( fvec2_t& pos, int facing, float distance )
{
    // Compass( tmpturn = "rotation", tmpdistance = "radius" )

    /// @author ZZ
    /// @details This function modifies tmpx and tmpy, depending on the setting of
    /// tmpdistance and tmpturn.  It acts like one of those Compass thing
    /// with the two little needle legs

    TURN_T turn;

    turn = TO_TURN( facing );

    pos[kX] -= turntocos[ turn ] * distance;
    pos[kY] -= turntosin[ turn ] * distance;

    return true;
}

//--------------------------------------------------------------------------------------------
int GetArmorPrice( Object * pchr, const int skin )
{
    // tmpx = GetTargetArmorPrice( tmpargument = "skin" )
    /// @author ZZ
    /// @details This function returns the cost of the desired skin upgrade, setting
    /// tmpx to the price

    return ProfileSystem::get().getProfile(pchr->profile_ref)->getSkinInfo(skin).cost;
}

//--------------------------------------------------------------------------------------------
Uint32 UpdateTime( Uint32 time_val, int delay )
{
    // UpdateTime( tmpargument = "time" )
    /// @author ZZ
    /// @details This function sets the character's ai timer.  50 clicks per second.
    /// Used in conjunction with IfTimeOut

    Uint32 new_time_val;

    if ( delay <= 0 )
    {
        new_time_val = time_val;
    }
    else
    {
        new_time_val = update_wld + delay;
    }

    return new_time_val;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint8 BreakPassage( int mesh_fx_or, const Uint16 become, const int frames, const int starttile, const int passageID, int *ptilex, int *ptiley )
{
    /// @author ZZ
    /// @details This function breaks the tiles of a passage if there is a character standing
    ///               on 'em.  Turns the tiles into damage terrain if it reaches last frame.

    Uint32 endtile;
    bool       useful;
    ego_tile_info_t * ptile = NULL;
    int loc_starttile;

    const std::shared_ptr<Passage> &passage = _currentModule->getPassageByID(passageID);

    if ( !passage ) return false;

    // limit the start tile the the 256 tile images that we have
    loc_starttile = CLIP_TO_08BITS( starttile );

    // same with the end tile
    endtile   =  loc_starttile + frames - 1;
    endtile = CLIP( endtile, (Uint32)0, (Uint32)255 );

    useful = false;
    for(const std::shared_ptr<Object> &pchr : _currentModule->getObjectHandler().iterator())
    {
        float lerp_z;

        // nothing in packs
        if ( IS_ATTACHED_CHR( pchr->ai.index ) ) continue;

        // nothing flying
        if ( 0 != pchr->flyheight ) continue;

        lerp_z = ( pchr->getPosZ() - pchr->enviro.floor_level ) / DAMAGERAISE;
        lerp_z = 1.0f - CLIP( lerp_z, 0.0f, 1.0f );

        if ( pchr->phys.weight * lerp_z <= 20 ) continue;

        TileIndex fan = ego_mesh_t::get_grid(_currentModule->getMeshPointer(), PointWorld(pchr->getPosX(), pchr->getPosY()));

        ptile = _currentModule->getMeshPointer()->get_ptile(fan);
        if ( NULL != ptile )
        {
            Uint16 img      = ptile->img & TILE_LOWER_MASK;
            Uint16 highbits = ptile->img & TILE_UPPER_MASK;

            if ( img >= loc_starttile && img < endtile )
            {
                if ( passage->objectIsInPassage( pchr->getPosX(), pchr->getPosY(), pchr->bump_1.size ) )
                {
                    // Remember where the hit occured.
                    *ptilex = pchr->getPosX();
                    *ptiley = pchr->getPosY();

                    useful = true;

                    // Change the tile image
                    img++;
                }
            }

            if ( img == endtile )
            {
                useful = ego_mesh_add_fx( _currentModule->getMeshPointer(), fan, mesh_fx_or );

                if ( become != 0 )
                {
                    img = become;
                }
            }

            if ( ptile->img != ( img | highbits ) )
            {
                ego_mesh_set_texture( _currentModule->getMeshPointer(), fan, img | highbits );
            }
        }
    }

    return useful;
}

//--------------------------------------------------------------------------------------------
Uint8 AddEndMessage( Object * pchr, const int message_index, script_state_t * pstate )
{
    /// @author ZZ
    /// @details This function appends a message to the end-module text

    size_t length;
    CHR_REF ichr;
    char * dst, * dst_end;

    Uint8 returncode = true;

    if ( nullptr == ( pchr ) ) return false;

    const std::shared_ptr<ObjectProfile> &ppro = ProfileSystem::get().getProfile(pchr->profile_ref);
    if ( !ppro->isValidMessageID( message_index ) ) return false;

    ichr           = GET_INDEX_PCHR( pchr );
    length = ppro->getMessage(message_index).length();

    dst     = endtext + endtext_carat;
    dst_end = endtext + MAXENDTEXT - 1;

    char buffer[256];
    strncpy(buffer, ppro->getMessage(message_index).c_str(), 256);

    expand_escape_codes( ichr, pstate, buffer, buffer + length, dst, dst_end );
    endtext_carat = strlen( endtext );

    str_add_linebreaks( endtext, strlen( endtext ), 30 );

    return returncode;
}

//--------------------------------------------------------------------------------------------
Uint8 FindTileInPassage( const int x0, const int y0, const int tiletype, const int passageID, int *px1, int *py1 )
{
    /// @author ZZ
    /// @details This function finds the next tile in the passage, x0 and y0
    ///    must be set first, and are set on a find.  Returns true or false
    ///    depending on if it finds one or not

    int x, y;
    
    ego_tile_info_t * ptile = NULL;

    const std::shared_ptr<Passage> &passage = _currentModule->getPassageByID(passageID);
    if ( !passage ) return false;

    // Do the first row
    x = x0 / GRID_ISIZE;
    y = y0 / GRID_ISIZE;

    if ( x < passage->getLeft() )  x = passage->getLeft();
    if ( y < passage->getTop() )  y = passage->getTop();

    if ( y < passage->getBottom() )
    {
        for ( /*nothing*/; x <= passage->getRight(); x++ )
        {
            TileIndex fan = ego_mesh_t::get_tile_int(_currentModule->getMeshPointer(), PointGrid(x, y));

            ptile = _currentModule->getMeshPointer()->get_ptile(fan);
            if ( NULL != ptile && tiletype == ( ptile->img & TILE_LOWER_MASK ) )
            {
                *px1 = ( x * GRID_ISIZE ) + 64;
                *py1 = ( y * GRID_ISIZE ) + 64;
                return true;
            }
        }
        y++;
    }

    // Do all remaining rows
    for ( /* nothing */; y <= passage->getBottom(); y++ )
    {
        for ( x = passage->getLeft(); x <= passage->getRight(); x++ )
        {
            TileIndex fan = ego_mesh_t::get_tile_int(_currentModule->getMeshPointer(), PointGrid(x, y));

            ptile = _currentModule->getMeshPointer()->get_ptile(fan);
            if ( NULL != ptile && tiletype == ( ptile->img & TILE_LOWER_MASK ) )
            {
                *px1 = x * GRID_ISIZE + 64;
                *py1 = y * GRID_ISIZE + 64;
                return true;
            }
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------
Uint8 _display_message( const CHR_REF ichr, const PRO_REF iprofile, const int message, script_state_t * pstate )
{
    /// @author ZZ
    /// @details This function sticks a message_offset in the display queue and sets its timer

    int slot;
    char * dst, * dst_end;
    size_t length;

    const std::shared_ptr<ObjectProfile> &ppro = ProfileSystem::get().getProfile(iprofile);
    if ( !ppro->isValidMessageID( message ) ) return false;

    slot = DisplayMsg_get_free();
    DisplayMsg.ary[slot].time = egoboo_config_t::get().hud_messageDuration.getValue();

    length = ppro->getMessage(message).length();

    dst     = DisplayMsg.ary[slot].textdisplay;
    dst_end = DisplayMsg.ary[slot].textdisplay + EGO_MESSAGE_SIZE - 1;

    char buffer[256];
    strncpy(buffer, ppro->getMessage(message).c_str(), 256);

    expand_escape_codes( ichr, pstate, buffer, buffer+length, dst, dst_end );

    *dst_end = CSTR_END;

    return true;
}

//--------------------------------------------------------------------------------------------
CHR_REF FindWeapon( Object * pchr, float max_distance, IDSZ weap_idsz, bool find_ranged, bool use_line_of_sight )
{
    /// @author ZF
    /// @details This function searches the nearby vincinity for a melee weapon the character can use

    CHR_REF ichr;
    CHR_REF retval = INVALID_CHR_REF;

    CHR_REF best_target = INVALID_CHR_REF;
    float   best_dist   = WIDE * WIDE;

    line_of_sight_info_t los;

    if ( nullptr == ( pchr ) ) return false;
    ichr = GET_INDEX_PCHR( pchr );

    // set up the target
    best_target = INVALID_CHR_REF;
    best_dist   = SQR( max_distance );

    //setup line of sight data
    los.x0 = pchr->getPosX();
    los.y0 = pchr->getPosY();
    los.z0 = pchr->getPosZ();
    los.stopped_by = pchr->stoppedby;

    for(const std::shared_ptr<Object> &pweapon : _currentModule->getObjectHandler().iterator())
    {
        float dist;
        fvec3_t diff;

        //only do items on the ground
        if ( _currentModule->getObjectHandler().exists( pweapon->attachedto ) || !pweapon->isitem ) continue;
        const std::shared_ptr<ObjectProfile> &weaponProfile = pweapon->getProfile();

        // only target those with a the given IDSZ
        if ( !chr_has_idsz( pweapon->getCharacterID(), weap_idsz ) ) continue;

        // ignore ranged weapons
        if ( !find_ranged && weaponProfile->isRangedWeapon() ) continue;

        // see if the character can use this weapon (we assume everyone has a left grip here)
        if ( ACTION_COUNT == pchr->getProfile()->getModel()->randomizeAction(weaponProfile->getWeaponAction(), SLOT_LEFT)) continue;

        // then check if a skill is needed
        if ( weaponProfile->requiresSkillIDToUse() )
        {
            if ( !chr_get_skill( pchr, chr_get_idsz( pweapon->getCharacterID(), IDSZ_SKILL ) ) ) continue;
        }

        //check distance
		diff = pchr->getPosition() - pweapon->getPosition();
        dist = diff.length_2();
        if ( dist < best_dist )
        {
            //finally, check line of sight. we only care for weapons we can see
            los.x1 = pweapon->getPosX();
            los.y1 = pweapon->getPosY();
            los.z1 = pweapon->getPosZ();

            if ( !use_line_of_sight || !line_of_sight_blocked( &los ) )
            {
                //found a valid weapon!
                best_target = pweapon->getCharacterID();
                best_dist = dist;
            }
        }
    }

    //Did we find anything?
    retval = INVALID_CHR_REF;
    if ( _currentModule->getObjectHandler().exists( best_target ) )
    {
        retval = best_target;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool FlashObject( Object * pchr, Uint8 value )
{
    /// @author ZZ
    /// @details This function sets a character's lighting
	if (!pchr) {
		return false;
	}
	chr_instance_flash(pchr->inst, value);
	return true;
}

