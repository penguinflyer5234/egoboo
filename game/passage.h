#pragma once

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

/// @file passage.h
/// @Passages and doors and whatnot.  Things that impede your progress!

#include "egoboo_typedef.h"

#include "passage_file.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_script_state;

DECLARE_REF( PASS_REF );
DECLARE_REF( SHOP_REF );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define MAX_PASS             256                     ///< Maximum number of passages ( mul 32 )
#define MAX_SHOP             MAX_PASS
#define CLOSETOLERANCE       2                       ///< For closing doors

#define SHOP_NOOWNER 0xFFFF        ///< Shop has no owner
#define SHOP_STOLEN  0xFFFF        ///< Someone stole a item

/// The pre-defined orders for communicating with shopkeepers
enum e_shop_orders
{
    SHOP_BUY       = 0,
    SHOP_SELL,
    SHOP_NOAFFORD,
    SHOP_THEFT
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Passages

DEFINE_STACK_EXTERN( passage_t, PassageStack, MAX_PASS );

#define VALID_PASSAGE_RANGE( IPASS ) ( ((IPASS) >= 0) && ((IPASS) <   MAX_PASS) )
#define VALID_PASSAGE( IPASS )       ( VALID_PASSAGE_RANGE( IPASS ) && ((IPASS) <  PassageStack.count) )
#define INVALID_PASSAGE( IPASS )     ( !VALID_PASSAGE( IPASS ) )

/// The data defining a shop
struct s_shop
{
    REF_T   passage;  ///< The passage number
    REF_T   owner;    ///< Who gets the gold?
};
typedef struct s_shop shop_t;

DEFINE_STACK_EXTERN( shop_t, ShopStack, MAX_SHOP );

#define VALID_SHOP_RANGE( ISHOP ) ( ((ISHOP) >= 0) && ((ISHOP) <   MAX_SHOP) )
#define VALID_SHOP( ISHOP )       ( VALID_SHOP_RANGE( ISHOP ) && ((ISHOP) <  ShopStack.count) )
#define INVALID_SHOP( ISHOP )     ( !VALID_SHOP( ISHOP ) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// prototypes

bool_t open_passage( PASS_REF ipassage );
bool_t close_passage( PASS_REF ipassage );
void   check_passage_music();
void   flash_passage( PASS_REF ipassage, Uint8 color );
REF_T  who_is_blocking_passage( PASS_REF ipassage, bool_t targetitems, bool_t targetdead, bool_t targetquest,
                                bool_t requireitem, IDSZ findidsz );
void   clear_all_passages();
void   add_shop_passage( REF_T owner, PASS_REF ipassage );
void   add_passage( passage_t * pdata );
void   activate_passages_file();
bool_t point_is_in_passage( PASS_REF ipassage, float xpos, float ypos );
bool_t object_is_in_passage( PASS_REF ipassage, float xpos, float ypos, float radius );

REF_T  shop_get_owner( int ix, int iy );
