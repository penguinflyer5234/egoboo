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

#include "bsp.h"

#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_chr;
struct s_prt;

struct s_cap;
struct s_mad;
struct s_eve;
struct s_pip;
struct Mix_Chunk;

struct s_mpd_BSP;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
DECLARE_REF( PRO_REF );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Placeholders used while importing profiles
struct s_pro_import
{
    int   slot;
    int   player;
    int   slot_lst[MAX_PROFILE];
    int   max_slot;
};
typedef struct s_pro_import pro_import_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// This is for random naming

#define CHOPPERMODEL                    32
#define MAXCHOP                         (MAX_PROFILE*CHOPPERMODEL)
#define CHOPSIZE                        8
#define CHOPDATACHUNK                   (MAXCHOP*CHOPSIZE)
#define MAXSECTION                      4              ///< T-wi-n-k...  Most of 4 sections

/// The buffer for the random naming data
struct s_chop_data
{
    size_t  chop_count;             ///< The global number of name parts

    Uint32  carat;                  ///< The data pointer
    char    buffer[CHOPDATACHUNK];  ///< The name parts
    int     start[MAXCHOP];         ///< The first character of each part
};
typedef struct s_chop_data chop_data_t;

chop_data_t * chop_data_init( chop_data_t * pdata );

bool_t        chop_export( const char *szSaveName, const char * szChop );

//--------------------------------------------------------------------------------------------
/// Defintion of a single chop secttion
struct s_chop_section
{
    int size;     ///< Number of choices, 0
    int start;    ///< A reference to a specific offset in the chop_data_t buffer
};
typedef struct s_chop_section chop_section_t;

//--------------------------------------------------------------------------------------------
/// Defintion of the chop info needed to create a name
struct s_chop_definition
{
    chop_section_t  section[MAXSECTION];
};
typedef struct s_chop_definition chop_definition_t;

chop_definition_t * chop_definition_init( chop_definition_t * pdefinition );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// a wrapper for all the datafiles in the *.obj dir
struct s_object_profile
{
    EGO_PROFILE_STUFF;

    // the sub-profiles
    REF_T   iai;                              ///< the AI  for this profile
    REF_T   icap;                             ///< the cap for this profile
    REF_T   imad;                             ///< the mad for this profile
    REF_T   ieve;                             ///< the eve for this profile

    REF_T   prtpip[MAX_PIP_PER_PROFILE];      ///< Local particles

    // the profile skins
    size_t  skins;                            ///< Number of skins
    int     tex_ref[MAX_SKIN];                 ///< references to the icon textures
    int     ico_ref[MAX_SKIN];                 ///< references to the skin textures

    // the profile message info
    int     message_start;                    ///< The first message

    /// the random naming info
    chop_definition_t chop;

    // sounds
    struct Mix_Chunk *  wavelist[MAX_WAVE];             ///< sounds in a object
};

typedef struct s_object_profile object_profile_t;
typedef struct s_object_profile pro_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// the profile list

DEFINE_LIST_EXTERN( pro_t, ProList, MAX_PROFILE );

int          pro_get_slot( const char * tmploadname, int slot_override );
const char * pro_create_chop( PRO_REF profile_ref );
bool_t       pro_load_chop( PRO_REF profile_ref, const char *szLoadname );

void    ProList_init();
//void    ProList_free_all();
size_t  ProList_get_free( PRO_REF override_ref );
bool_t  ProList_free_one( PRO_REF object_ref );

#define VALID_PRO_RANGE( IOBJ ) ( ((IOBJ) >= 0) && ((IOBJ) < MAX_PROFILE) )
#define LOADED_PRO( IOBJ )       ( VALID_PRO_RANGE( IOBJ ) && ProList.lst[IOBJ].loaded )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// the BSP structure housing the object
struct s_obj_BSP
{
    // the BSP of characters for character-character and character-particle interactions
    BSP_tree_t   tree;
};

typedef struct s_obj_BSP obj_BSP_t;

bool_t obj_BSP_ctor( obj_BSP_t * pbsp, struct s_mpd_BSP * pmesh_bsp );
bool_t obj_BSP_dtor( obj_BSP_t * pbsp );

bool_t obj_BSP_alloc( obj_BSP_t * pbsp, int depth );
bool_t obj_BSP_free( obj_BSP_t * pbsp );

bool_t obj_BSP_fill( obj_BSP_t * pbsp );
bool_t obj_BSP_empty( obj_BSP_t * pbsp );

//bool_t obj_BSP_insert_leaf( obj_BSP_t * pbsp, BSP_leaf_t * pnode, int depth, int address_x[], int address_y[], int address_z[] );
bool_t obj_BSP_insert_chr( obj_BSP_t * pbsp, struct s_chr * pchr );
bool_t obj_BSP_insert_prt( obj_BSP_t * pbsp, struct s_prt * pprt );

int    obj_BSP_collide( obj_BSP_t * pbsp, BSP_aabb_t * paabb, BSP_leaf_pary_t * colst );

extern obj_BSP_t obj_BSP_root;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern size_t  bookicon_count;
extern REF_T   bookicon_ref[MAX_SKIN];                      ///< The first book icon

extern pro_import_t import_data;
extern chop_data_t chop_mem;

DEFINE_STACK_EXTERN( int, MessageOffset, MAXTOTALMESSAGE );

extern Uint32          message_buffer_carat;                                  ///< Where to put letter
extern char            message_buffer[MESSAGEBUFFERSIZE];                     ///< The text buffer

extern int             BSP_chr_count;                                         ///< the number of characters in the obj_BSP_root structure
extern int             BSP_prt_count;                                         ///< the number of particles  in the obj_BSP_root structure

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void profile_system_begin();
void profile_system_end();

void   init_all_profiles();
int    load_profile_skins( const char * tmploadname, PRO_REF object_ref );
void   load_all_messages( const char *loadname, PRO_REF object_ref );
void   release_all_pro_data();
void   release_all_profiles();
void   release_all_pro();
void   release_all_local_pips();
bool_t release_one_pro( PRO_REF object_ref );
bool_t release_one_local_pips( PRO_REF object_ref );

int load_one_profile( const char* tmploadname, int slot_override );

void reset_messages();

const char *  chop_create( chop_data_t * pdata, chop_definition_t * pdef );
bool_t        chop_load( chop_data_t * pchop_data, const char *szLoadname, chop_definition_t * pchop_definition );
