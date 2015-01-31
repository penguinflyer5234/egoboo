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

/// @file game/module/Module.cpp
/// @details Code handling a game module
/// @author Johan Jansen

#include "game/module/Module.hpp"
#include "egolib/math/Random.hpp"
#include "game/module/Passage.hpp"
#include "game/game.h"
#include "game/network.h"
#include "game/player.h"
#include "game/mesh.h"
#include "game/char.h"
#include "game/module/ObjectHandler.hpp"

GameModule::GameModule(const mod_file_t * pdata, const std::string& name, const uint32_t seed) :
    _name(name),
    _importAmount(pdata->importamount),
    _exportValid(pdata->allowexport),
    _exportReset(pdata->allowexport),
    _playerAmount(pdata->maxplayers),
    _canRespawnAnyTime(RESPAWN_ANYTIME == pdata->respawnvalid),
    _isRespawnValid(false != pdata->respawnvalid),
    _isBeaten(false),
    _seed(seed),
    _passages()
{
    srand( _seed );
    Random::setSeed(_seed);
    randindex = rand() % RANDIE_COUNT;

    // very important or the input will not work
    egonet_set_hostactive( true );  
}

GameModule::~GameModule()
{
    // network stuff
    egonet_set_hostactive( false );
}

void GameModule::loadAllPassages()
{
    // Reset all of the old passages
    _passages.clear();

    // Load the file
    vfs_FILE *fileread = vfs_openRead( "mp_data/passage.txt" );
    if ( NULL == fileread ) return;

    //Load all passages in file
    while ( goto_colon_vfs( NULL, fileread, true ) )
    {
        //read passage area
        irect_t area;
        area._left   = vfs_get_int( fileread );
        area._top    = vfs_get_int( fileread );
        area._right  = vfs_get_int( fileread );
        area._bottom = vfs_get_int( fileread );

        //constrain passage area within the level
        area._left    = CLIP( area._left,   0, PMesh->info.tiles_x - 1 );
        area._top     = CLIP( area._top,    0, PMesh->info.tiles_y - 1 );
        area._right   = CLIP( area._right,  0, PMesh->info.tiles_x - 1 );
        area._bottom  = CLIP( area._bottom, 0, PMesh->info.tiles_y - 1 );

        //Read if open by default
        bool open = vfs_get_bool( fileread );

        //Read mask (optional)
        uint8_t mask = MAPFX_IMPASS | MAPFX_WALL;
        if ( vfs_get_bool( fileread ) ) mask = MAPFX_IMPASS;
        if ( vfs_get_bool( fileread ) ) mask = MAPFX_SLIPPY;

        std::shared_ptr<Passage> passage = std::make_shared<Passage>(area, mask);

        //check if we need to close the passage
        if(!open) {
            passage->close();
        }

        //finished loading this one!
        _passages.push_back(passage);
    }

    //all done!
    vfs_close( fileread );
}

void GameModule::checkPassageMusic()
{
    // Look at each player
    for ( PLA_REF ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        CHR_REF character = PlaStack.lst[ipla].index;
        if ( !_gameObjects.exists( character ) ) continue;

        //dont do items in hands or inventory
        if ( IS_ATTACHED_CHR( character ) ) continue;

        chr_t * pchr = _gameObjects.get( character );
        if ( !pchr->alive || !VALID_PLA( pchr->is_which_player ) ) continue;

        //Loop through every passage
        for(const std::shared_ptr<Passage> &passage : _passages)
        {
            if(passage->checkPassageMusic(pchr)) {
                return;
            }
        }   
    }
}

CHR_REF GameModule::getShopOwner(const float x, const float y)
{
    //Loop through every passage
    for(const std::shared_ptr<Passage> &passage : _passages)
    {
        //Only check actual shops
        if(!passage->isShop()) {
            continue;
        }

        //Is item inside this shop?
        if(passage->isPointInside(x, y)) {
            return passage->getShopOwner();
        }
    }

    return Passage::SHOP_NOOWNER;       
}

void GameModule::removeShopOwner(CHR_REF owner)
{
    //Loop through every passage
    for(const std::shared_ptr<Passage> &passage : _passages)
    {
        //Only check actual shops
        if(!passage->isShop()) {
            continue;
        }

        if(passage->getShopOwner() == owner) {
            passage->removeShop();
        }

        //TODO: mark all items in shop as normal items again
    }
}

int GameModule::getPassageCount()
{
    return _passages.size();
}

std::shared_ptr<Passage> GameModule::getPassageByID(int id)
{
    if(id < 0 || id >= _passages.size()) {
        return nullptr;
    }

    return _passages[id];
}