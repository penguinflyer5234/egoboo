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

/// @file  game/collision.c
/// @brief The code that handles collisions between in-game objects
/// @details

#include "game/collision.h"
#include "game/obj_BSP.h"
#include "game/bsp.h"
#include "game/game.h"
#include "game/graphic_billboard.h"
#include "game/char.h"
#include "game/physics.h"
#include "egolib/Logic/Action.hpp"
#include "game/Entities/_Include.hpp"
#include "game/Module/Module.hpp"
#include "egolib/Profiles/_Include.hpp"
#include "egolib/Graphics/ModelDescriptor.hpp"

CollisionSystem *CollisionSystem::_singleton = nullptr;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAKE_HASH(AA,BB)         CLIP_TO_08BITS( ((AA) * 0x0111 + 0x006E) + ((BB) * 0x0111 + 0x006E) )



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// data block used to communicate between the different "modules" governing the character-particle collision
struct chr_prt_collision_data_t
{
public:
    chr_prt_collision_data_t();

public:
    // object parameters
    CHR_REF ichr;
    Object *pchr;

    PRT_REF iprt;
    std::shared_ptr<Ego::Particle> pprt;
    std::shared_ptr<pip_t> ppip;

    //---- collision parameters

    // true collisions
    bool int_min;
    float depth_min;

    // hit-box collisions
    bool int_max;
    float depth_max;

    // platform interactions
    //bool int_plat;
    //float plat_lerp;

    bool is_impact;
    bool is_pressure;
    bool is_collision;
    float dot;
    fvec3_t nrm;

    // collision modifications
    bool mana_paid;
    int max_damage, actual_damage;
    fvec3_t vdiff, vdiff_para, vdiff_perp;
    float block_factor;

    // collision reaction
    //fvec3_t vimpulse;                      ///< the velocity impulse
    //fvec3_t pimpulse;                      ///< the position impulse
    bool terminate_particle;
    bool prt_bumps_chr;
    bool prt_damages_chr;

    static chr_prt_collision_data_t *init(chr_prt_collision_data_t *);
};

static bool do_chr_prt_collision_deflect(chr_prt_collision_data_t * pdata);
//static bool do_chr_prt_collision_recoil(chr_prt_collision_data_t * pdata);
static bool do_chr_prt_collision_damage(chr_prt_collision_data_t * pdata);
//static bool do_chr_prt_collision_impulse(chr_prt_collision_data_t * pdata);
static bool do_chr_prt_collision_bump(chr_prt_collision_data_t * pdata);
static bool do_chr_prt_collision_handle_bump(chr_prt_collision_data_t * pdata);

chr_prt_collision_data_t::chr_prt_collision_data_t() :
    ichr(INVALID_CHR_REF),
    pchr(nullptr),

    iprt(INVALID_PRT_REF),
    pprt(nullptr),
    ppip(nullptr),

    int_min(false),
    depth_min(0.0f),
    int_max(false),
    depth_max(0.0f),

    is_impact(false),
    is_pressure(false),
    is_collision(false),
    dot(0.0f),
    nrm(),

    mana_paid(false),
    max_damage(0),
    actual_damage(0),
    vdiff(), 
    vdiff_para(), 
    vdiff_perp(),
    block_factor(0.0f),

    //vimpulse(),                      ///< the velocity impulse
    //pimpulse(),                      ///< the position impulse
    terminate_particle(false),
    prt_bumps_chr(false),
    prt_damages_chr(false)
{
    //ctor
}

//--------------------------------------------------------------------------------------------

/// one element of the data for partitioning character and particle positions
struct bumplist_t
{
    CHR_REF chr;                    // For character collisions
    CHR_REF prt;                    // For particle collisions
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool detect_chr_chr_interaction_valid( const CHR_REF ichr_a, const CHR_REF ichr_b );
static bool detect_chr_prt_interaction_valid( const CHR_REF ichr_a, const PRT_REF iprt_b );

static bool do_chr_platform_detection( const CHR_REF ichr_a, const CHR_REF ichr_b );
static bool do_prt_platform_detection( const CHR_REF ichr_a, const PRT_REF iprt_b );

static bool fill_interaction_list(CoHashList_t *coHashList, CollisionSystem::CollNodeAry& collNodeAry, CollisionSystem::HashNodeAry& hashNodeAry);
static bool fill_bumplists();

static bool bump_all_platforms( Ego::DynamicArray<CoNode_t> *pcn_ary );
static bool bump_all_mounts( Ego::DynamicArray<CoNode_t> *pcn_ary );
static bool bump_all_collisions( Ego::DynamicArray<CoNode_t> *pcn_ary );

static bool bump_one_mount( const CHR_REF ichr_a, const CHR_REF ichr_b );
static bool do_chr_platform_physics( Object * pitem, Object * pplat );
//static float estimate_chr_prt_normal( const Object * pchr, const prt_t * pprt, fvec3_t& nrm, fvec3_t& vdiff );
static bool do_chr_chr_collision( CoNode_t * d );

static bool do_chr_prt_collision_init( const CHR_REF ichr, const PRT_REF iprt, chr_prt_collision_data_t * pdata );

static bool do_chr_prt_collision( CoNode_t * d );

//static bool do_prt_platform_physics( chr_prt_collision_data_t * pdata );
static bool do_chr_prt_collision_get_details( CoNode_t * d, chr_prt_collision_data_t * pdata );
//static bool do_chr_chr_collision_pressure_normal(const Object *pchr_a, const Object *pchr_b, const float exponent, oct_vec_v2_t& odepth, fvec3_t& nrm, float& depth);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CollisionSystem::CollisionSystem() :
    _hn_ary_2(), 
    _cn_ary_2(),
    _hash(nullptr),
    _coll_leaf_lst(),
    _coll_node_lst()
{
    if (!_coll_leaf_lst.ctor(COLLISION_LIST_SIZE))
    {
        goto Fail;
    }
    if (!_coll_node_lst.ctor(COLLISION_LIST_SIZE))
    {
        _coll_leaf_lst.dtor();
        goto Fail;
    }
    try
    {
        _hash = new CoHashList_t(512); /** @todo Why is this not factored out to a named constant? */
    }
    catch (std::bad_alloc& ex)
    {
        _coll_node_lst.dtor();
        _coll_leaf_lst.dtor();
        goto Fail;
    }
    return;
Fail:
    throw std::runtime_error("unable to initialize collision system\n");
}

CollisionSystem::~CollisionSystem()
{
    reset();
    if (_hash)
    {
        delete _hash;
        _hash = nullptr;
    }
    _coll_leaf_lst.dtor();
    _coll_node_lst.dtor();
}

bool CollisionSystem::initialize()
{
    if (CollisionSystem::_singleton)
    {
        log_warning("%s:%d: collision system already initialized - ignoring\n",__FILE__,__LINE__);
        return true;
    }
    try
    {
        CollisionSystem::_singleton = new CollisionSystem();
    }
    catch (std::exception& ex)
    {
        return false;
    }
    return true;
}

void CollisionSystem::uninitialize()
{
    if (!CollisionSystem::_singleton)
    {
        log_warning("%s:%d: collision system not initialized - ignoring\n", __FILE__, __LINE__);
        return;
    }
    delete CollisionSystem::_singleton;
    CollisionSystem::_singleton = nullptr;
}

void CollisionSystem::reset()
{
    if (!CollisionSystem::_singleton)
    {
        log_warning("%s:%d: collision system not initialized - ignoring\n", __FILE__, __LINE__);
        return;
    }
    // Reset the collision node magazine.
    _cn_ary_2.reset();

    // Reset the hash node magazine.
    _hn_ary_2.reset();

    // Clear the collision hash.
    _hash->clear();

    // Clear the collisions.
    _coll_node_lst.clear();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CoNode_t::CoNode_t() :
    chra(INVALID_CHR_REF),
    prta(INVALID_PRT_REF),
    chrb(INVALID_CHR_REF),
    prtb(INVALID_PRT_REF),
    
    tileb(MAP_FANOFF),
    tmin(-1.0f),
    tmax(-1.0f),
    cv()
{
    //ctor
}

CoNode_t *CoNode_t::ctor(CoNode_t *self)
{
    if (!self) return nullptr;

    // The "colliding" objects.
    self->chra = INVALID_CHR_REF;
    self->prta = INVALID_PRT_REF;

    // The "collided with" objects.
    self->chrb  = INVALID_CHR_REF;
    self->prtb  = INVALID_PRT_REF;
    self->tileb = MAP_FANOFF;

    // The time.
    self->tmin = self->tmax = -1.0f;
    oct_bb_t::ctor(self->cv);

    return self;
}

//--------------------------------------------------------------------------------------------
Uint8 CoNode_t::generate_hash(const CoNode_t *self)
{
    Uint32 AA, BB;

    AA = UINT32_MAX;
    if ( VALID_CHR_RANGE( self->chra ) )
    {
        AA = REF_TO_INT( self->chra );
    }
    else if ( self->prta != INVALID_PRT_REF )
    {
        AA = REF_TO_INT( self->prta );
    }

    BB = UINT32_MAX;
    if ( VALID_CHR_RANGE( self->chrb ) )
    {
        BB = REF_TO_INT( self->chrb );
    }
    else if ( self->prtb != INVALID_PRT_REF )
    {
        BB = REF_TO_INT( self->prtb );
    }
    else if ( MAP_FANOFF != self->tileb )
    {
        BB = self->tileb;
    }

    return MAKE_HASH( AA, BB );
}

//--------------------------------------------------------------------------------------------
int CoNode_t::cmp(const CoNode_t *self, const CoNode_t *other)
{
    float ftmp;

    // Sort by initial time first.
    ftmp = self->tmin - other->tmin;
    if (ftmp <= 0.0f) return -1;
    else if (ftmp >= 0.0f) return 1;

    // Sort by final time second.
    ftmp = self->tmax - other->tmax;
    if (ftmp <= 0.0f) return -1;
    else if (ftmp >= 0.0f) return 1;

    return CoNode_t::cmp_unique(self, other);
}

//--------------------------------------------------------------------------------------------
int CoNode_t::cmp_unique(const CoNode_t *self, const CoNode_t *other)
{
    int   itmp;

    // Don't compare the times.

    itmp = (signed)REF_TO_INT(self->chra) - (signed)REF_TO_INT(other->chra);
    if (0 != itmp) return itmp;

    itmp = (signed)REF_TO_INT(self->prta) - (signed)REF_TO_INT(other->prta);
    if (0 != itmp) return itmp;

    itmp = (signed)REF_TO_INT(self->chrb) - (signed)REF_TO_INT(other->chrb);
    if (0 != itmp) return itmp;

    itmp = (signed)REF_TO_INT(self->prtb) - (signed)REF_TO_INT(other->prtb);
    if (0 != itmp) return itmp;

    itmp = (signed)self->tileb            - (signed)other->tileb;
    if (0 != itmp) return itmp;

    return 0;
}

//--------------------------------------------------------------------------------------------
int CoNode_t::matches(const CoNode_t *self, const CoNode_t *other)
{
    CoNode_t reversed;

	if (0 == CoNode_t::cmp_unique(self, other)) return true;

    // Make a reversed version of other.
	reversed.tmin = other->tmin;
	reversed.tmax = other->tmax;
	reversed.chra = other->chrb;
	reversed.prta = other->prtb;
	reversed.chrb = other->chra;
	reversed.prtb = other->prta;
	reversed.tileb = other->tileb;

	if (0 == CoNode_t::cmp_unique(self,&reversed)) return true;

    return false;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool CoHashList_insert_unique(CoHashList_t *coHashList, CoNode_t *data, CollisionSystem::CollNodeAry& collNodeAry, CollisionSystem::HashNodeAry& hashNodeAry)
{
	if (NULL == coHashList || NULL == data)
	{
		return false;
	}
    // Compute the hash for this collision.
	Uint32 hash = CoNode_t::generate_hash(data);
    if (hash >= coHashList->getCapacity())
    {
        throw std::runtime_error("hash out of bounds");
    }
	// Get the number of entries in this bucket.
    size_t bucketSize = hash_list_t::get_count(coHashList,hash);
    // If the bucket is not empty ...
	if (bucketSize > 0)
    {
		// ... search the bucket for an entry for this collision.
        for (hash_node_t *node = coHashList->sublist[hash]; nullptr != node; node = node->next)
        {
            if (CoNode_t::matches((CoNode_t *)(node->data), data))
            {
                return false;
            }
        }
    }

    // If no entry for this collision was found ...
    {
		// ... add it:
        // Pick a free collision data ...
        CoNode_t *cnode = collNodeAry.acquire();
        if (!cnode)
        {
            throw std::runtime_error("no more free collision nodes");
        }
		// ... and store the data in that.
		*cnode = *data;

        // Generate a new hash node.
		hash_node_t *hnode = hashNodeAry.acquire();
        if (!hnode)
        {
            throw std::runtime_error("no more free hash nodes");
        }

        // link the hash node to the free CoNode
        hnode->data = cnode;

        // insert the node at the front of the collision list for this hash
        hash_node_t *old_head = hash_list_get_node(coHashList, hash);
        hash_node_t *new_head = hash_node_insert_before(old_head, hnode);
        hash_list_set_node(coHashList, hash, new_head);

        // add 1 to the count at this hash
        size_t old_count = hash_list_t::get_count(coHashList, hash);
        hash_list_set_count(coHashList, hash, old_count + 1);
    }

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool get_chr_mass( Object * pchr, float * wt )
{
    /// @author BB
    /// @details calculate a "mass" for an object, taking into account possible infinite masses.

    float loc_wta;

    if ( !ACTIVE_PCHR( pchr ) ) return false;

    // handle oprtional parameters
    if ( NULL == wt ) wt = &loc_wta;

    if ( CHR_INFINITE_WEIGHT == pchr->phys.weight )
    {
        *wt = -static_cast<float>(CHR_INFINITE_WEIGHT);
    }
    else if ( 0.0f == pchr->phys.bumpdampen )
    {
        *wt = -static_cast<float>(CHR_INFINITE_WEIGHT);
    }
    else
    {
        *wt = pchr->phys.weight / pchr->phys.bumpdampen;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool get_prt_mass( Ego::Particle * pprt, Object * pchr, float * wt )
{
    /// @author BB
    /// @details calculate a "mass" for each object, taking into account possible infinite masses.

    float loc_wprt;

    if ( NULL == pprt || NULL == pchr ) return false;

    if ( NULL == wt ) wt = &loc_wprt;

    // determine an approximate mass for the particle
    if ( 0.0f == pprt->phys.bumpdampen )
    {
        *wt = -( float )CHR_INFINITE_WEIGHT;
    }
    else if ( pprt->isAttached() )
    {
        if ( CHR_INFINITE_WEIGHT == pprt->phys.weight || 0.0f == pprt->phys.bumpdampen )
        {
            *wt = -( float )CHR_INFINITE_WEIGHT;
        }
        else
        {
            *wt = pprt->phys.weight / pprt->phys.bumpdampen;
        }
    }
    else
    {
        float max_damage = std::abs(pprt->damage.base) + std::abs(pprt->damage.rand);

        *wt = 1.0f;

        if ( 0 == max_damage )
        {
            // this is a particle like the wind particles in the whirlwind
            // make the particle have some kind of predictable constant effect
            // relative to any character;
            *wt = pchr->phys.weight / 10.0f;
        }
        else
        {
            // determine an "effective mass" for the particle, based on it's max damage
            // and velocity

            float prt_vel2;
            float prt_ke;
            fvec3_t vdiff;

            vdiff = pprt->vel - pchr->vel;

            // the damage is basically like the kinetic energy of the particle
            prt_vel2 = vdiff.dot(vdiff);

            // It can happen that a damage particle can hit something
            // at almost zero velocity, which would make for a huge "effective mass".
            // by making a reasonable "minimum velocity", we limit the maximum mass to
            // something reasonable
            prt_vel2 = std::max( 100.0f, prt_vel2 );

            // get the "kinetic energy" from the damage
            prt_ke = 3.0f * max_damage;

            // the faster the particle is going, the smaller the "mass" it
            // needs to do the damage
            *wt = prt_ke / ( 0.5f * prt_vel2 );
        }

        *wt /= pprt->phys.bumpdampen;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void get_recoil_factors( float wta, float wtb, float * recoil_a, float * recoil_b )
{
    float loc_recoil_a, loc_recoil_b;

    if ( NULL == recoil_a ) recoil_a = &loc_recoil_a;
    if ( NULL == recoil_b ) recoil_b = &loc_recoil_b;

    if ( wta >= ( float )CHR_INFINITE_WEIGHT ) wta = -( float )CHR_INFINITE_WEIGHT;
    if ( wtb >= ( float )CHR_INFINITE_WEIGHT ) wtb = -( float )CHR_INFINITE_WEIGHT;

    if ( wta < 0.0f && wtb < 0.0f )
    {
        *recoil_a = 0.5f;
        *recoil_b = 0.5f;
    }
    else if ( wta == wtb )
    {
        *recoil_a = 0.5f;
        *recoil_b = 0.5f;
    }
    else if ( wta < 0.0f || 0.0f == wtb )
    {
        *recoil_a = 0.0f;
        *recoil_b = 1.0f;
    }
    else if ( wtb < 0.0f || 0.0f == wta )
    {
        *recoil_a = 1.0f;
        *recoil_b = 0.0f;
    }
    else
    {
        *recoil_a = wtb / ( wta + wtb );
        *recoil_b = wta / ( wta + wtb );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool detect_chr_chr_interaction_valid( const CHR_REF ichr_a, const CHR_REF ichr_b )
{
    Object *pchr_a, *pchr_b;

    // Don't interact with self
    if ( ichr_a == ichr_b ) return false;

    // Ignore invalid characters
    if ( !_currentModule->getObjectHandler().exists( ichr_a ) ) return false;
    pchr_a = _currentModule->getObjectHandler().get( ichr_a );

    // Ignore invalid characters
    if ( !_currentModule->getObjectHandler().exists( ichr_b ) ) return false;
    pchr_b = _currentModule->getObjectHandler().get( ichr_b );

    // "non-interacting" objects interact with platforms
    if (( 0 == pchr_a->bump.size && !pchr_b->platform ) ||
        ( 0 == pchr_b->bump.size && !pchr_a->platform ) )
    {
        return false;
    }

    // reject characters that are hidden
    if ( pchr_a->is_hidden || pchr_b->is_hidden ) return false;

    // don't interact with your mount, or your held items
    if ( ichr_a == pchr_b->attachedto || ichr_b == pchr_a->attachedto ) return false;

    // handle the dismount exception
    if ( pchr_a->dismount_timer > 0 && pchr_a->dismount_object == ichr_b ) return false;
    if ( pchr_b->dismount_timer > 0 && pchr_b->dismount_object == ichr_a ) return false;

    return true;
}

//--------------------------------------------------------------------------------------------
bool detect_chr_prt_interaction_valid( const CHR_REF ichr_a, const PRT_REF iprt_b )
{

    // Ignore invalid characters
    const std::shared_ptr<Object> &pchr_a = _currentModule->getObjectHandler()[ichr_a];
    if ( !pchr_a ) return false;

    // Ignore invalid particles
    const std::shared_ptr<Ego::Particle> &pprt_b = ParticleHandler::get()[iprt_b];
    if(pprt_b == nullptr || pprt_b->isTerminated()) return false;

    // reject characters that are hidden
    if ( pchr_a->is_hidden || pprt_b->isHidden() ) return false;

    // particles don't "collide" with anything they are attached to.
    // that only happes through doing bump particle damamge
    if ( pchr_a == pprt_b->getAttachedObject() ) return false;

    // don't interact if there is no interaction...
    // the particles and characters should not have been added to the list unless they
    // are valid for collision

    return true;
}

//--------------------------------------------------------------------------------------------
bool fill_interaction_list(CoHashList_t *coHashList, CollisionSystem::CollNodeAry& collNodeAry, CollisionSystem::HashNodeAry& hashNodeAry)
{
    int              cnt;
    int              reaffirmation_count;
    int              reaffirmation_list[DAMAGE_COUNT];
    aabb_t           tmp_aabb;

    if ( NULL == coHashList) return false;

    // Clear the collision hash.
	coHashList->clear();

    // initialize the reaffirmation counters
    reaffirmation_count = 0;
    for ( cnt = 0; cnt < DAMAGE_COUNT; cnt++ )
    {
        reaffirmation_list[cnt] = 0;
    }

    //---- find the character/particle interactions

    // Find the character-character interactions. Use the ChrList.used_ref, for a change
    for(const std::shared_ptr<Object> &pchr_a : _currentModule->getObjectHandler().iterator())
    {
        oct_bb_t   tmp_oct;

        // ignore in-accessible objects
        if ( _currentModule->getObjectHandler().exists( pchr_a->inwhich_inventory ) || pchr_a->is_hidden ) continue;

        // keep track of how many objects use reaffirmation, and what kinds of reaffirmation
        if ( pchr_a->reaffirm_damagetype < DAMAGE_COUNT )
        {
            if ( pchr_a->getProfile()->getAttachedParticleAmount() > 0 )
            {
                // we COULD use number_of_attached_particles() to determin if the
                // character is full of particles, BUT since it scans through the
                // entire particle list I don't think it's worth it

                reaffirmation_count++;
                reaffirmation_list[pchr_a->reaffirm_damagetype]++;
            }
        }

        // use the object velocity to figure out where the volume that the object will occupy during this
        // update
        phys_expand_chr_bb(pchr_a.get(), 0.0f, 1.0f, tmp_oct);

        // convert the oct_bb_t to a correct BSP_aabb_t
        tmp_aabb = tmp_oct.toAABB();

        // find all collisions with other characters and particles
        CollisionSystem::get()->_coll_leaf_lst.clear();
        getChrBSP()->collide(tmp_aabb, chr_BSP_can_collide, CollisionSystem::get()->_coll_leaf_lst);

        // transfer valid _coll_leaf_lst entries to pchlst entries
        // and sort them by their initial times
        if (!CollisionSystem::get()->_coll_leaf_lst.empty())
        {
            for (size_t j = 0; j < CollisionSystem::get()->_coll_leaf_lst.size(); j++)
            {
                BSP_leaf_t * pleaf;
                CoNode_t    tmp_codata;
                bool      do_insert;
                BIT_FIELD   test_platform;

                pleaf = CollisionSystem::get()->_coll_leaf_lst.ary[j];
                if ( NULL == pleaf ) continue;

                do_insert = false;

                if ( BSP_LEAF_CHR == pleaf->_type )
                {
                    // collided with a character
                    CHR_REF ichr_b = ( CHR_REF )( pleaf->_index );

                    // do some logic on this to determine whether the collision is valid
                    if ( detect_chr_chr_interaction_valid( pchr_a->getCharacterID(), ichr_b ) )
                    {
                        Object * pchr_b = _currentModule->getObjectHandler().get( ichr_b );

                        CoNode_t::ctor( &tmp_codata );

                        // do a simple test, since I do not want to resolve the ObjectPRofile for these objects here
                        test_platform = EMPTY_BIT_FIELD;
                        if ( pchr_a->platform && pchr_b->canuseplatforms ) SET_BIT( test_platform, PHYS_PLATFORM_OBJ1 );
                        if ( pchr_b->platform && pchr_a->canuseplatforms ) SET_BIT( test_platform, PHYS_PLATFORM_OBJ2 );

                        // detect a when the possible collision occurred
                        if (phys_intersect_oct_bb(pchr_a->chr_max_cv, pchr_a->getPosition(), pchr_a->vel, pchr_b->chr_max_cv, pchr_b->getPosition(), pchr_b->vel, test_platform, tmp_codata.cv, &(tmp_codata.tmin), &(tmp_codata.tmax)))
                        {
                            tmp_codata.chra = pchr_a->getCharacterID();
                            tmp_codata.chrb = ichr_b;

                            do_insert = true;
                        }
                    }
                }
                else
                {
                    // how did we get here?
                    log_warning( "fill_interaction_list() - found non-character in the character BSP\n" );
                }

                if ( do_insert )
                {
					CoHashList_insert_unique(coHashList, &tmp_codata, collNodeAry, hashNodeAry);
                }
            }
        }

        CollisionSystem::get()->_coll_leaf_lst.clear();
        getPrtBSP()->collide(tmp_aabb, prt_BSP_can_collide, CollisionSystem::get()->_coll_leaf_lst);
        if (!CollisionSystem::get()->_coll_leaf_lst.empty())
        {
            for (size_t j = 0; j < CollisionSystem::get()->_coll_leaf_lst.size(); j++)
            {
                BSP_leaf_t * pleaf;
                CoNode_t    tmp_codata;
                bool      do_insert;
                BIT_FIELD   test_platform;

                pleaf = CollisionSystem::get()->_coll_leaf_lst.ary[j];
                if ( NULL == pleaf ) continue;

                do_insert = false;

                if ( BSP_LEAF_PRT == pleaf->_type )
                {
                    // collided with a particle
                    PRT_REF iprt_b = ( PRT_REF )( pleaf->_index );

                    // do some logic on this to determine whether the collision is valid
                    if ( detect_chr_prt_interaction_valid( pchr_a->getCharacterID(), iprt_b ) )
                    {
                        const std::shared_ptr<Ego::Particle> &pprt_b = ParticleHandler::get()[iprt_b];

                        CoNode_t::ctor( &tmp_codata );

                        // do a simple test, since I do not want to resolve the ObjectProfile for these objects here
                        test_platform = pchr_a->platform ? PHYS_PLATFORM_OBJ1 : 0;

                        // detect a when the possible collision occurred
                        if (phys_intersect_oct_bb(pchr_a->chr_max_cv, pchr_a->getPosition(), pchr_a->vel, pprt_b->prt_max_cv, pprt_b->getPosition(), pprt_b->vel, test_platform, tmp_codata.cv, &(tmp_codata.tmin), &(tmp_codata.tmax)))
                        {
                            tmp_codata.chra = pchr_a->getCharacterID();
                            tmp_codata.prtb = iprt_b;

                            do_insert = true;
                        }
                    }
                }
                else
                {
                    // how did we get here?
                    log_warning( "fill_interaction_list() - found non-particle in the particle BSP\n" );
                }

                if ( do_insert )
                {
					CoHashList_insert_unique(coHashList, &tmp_codata, collNodeAry, hashNodeAry);
                }
            }
        }
    }

    //---- find some specialized character-particle interactions
    //     namely particles that end-bump or particles that reaffirm characters

    for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
    {
        oct_bb_t   tmp_oct;
        bool     can_reaffirm, needs_bump;

		if (particle->isTerminated()) continue;

        BSP_leaf_t *pleaf = &particle->getBSPLeaf();

        // if the particle is in the BSP, then it has already had it's chance to collide
        if (pleaf->isInList()) continue;

        // does the particle potentially reaffirm a character?
        can_reaffirm = TO_C_BOOL(( particle->damagetype < DAMAGE_COUNT ) && ( 0 != reaffirmation_list[particle->damagetype] ) );

        // does the particle end_bump or end_ground?
        needs_bump = TO_C_BOOL( particle->getProfile()->end_bump || particle->getProfile()->end_ground );

        if ( !can_reaffirm && !needs_bump ) continue;

        // use the object velocity to figure out where the volume that the object will occupy during this
        // update
        phys_expand_prt_bb(particle.get(), 0.0f, 1.0f, tmp_oct);

        // convert the oct_bb_t to a correct BSP_aabb_t
        tmp_aabb = tmp_oct.toAABB();

        // find all collisions with characters
        CollisionSystem::get()->_coll_leaf_lst.clear();
        getChrBSP()->collide(tmp_aabb, chr_BSP_can_collide, CollisionSystem::get()->_coll_leaf_lst);

        // transfer valid _coll_leaf_lst entries to pchlst entries
        // and sort them by their initial times
        if (!CollisionSystem::get()->_coll_leaf_lst.empty())
        {
            CoNode_t     tmp_codata;
            BIT_FIELD    test_platform;
            CHR_REF      ichr_a = INVALID_CHR_REF;
            BSP_leaf_t * pleaf = NULL;
            bool       do_insert = false;

            for (size_t j = 0; j < CollisionSystem::get()->_coll_leaf_lst.size(); j++)
            {
                pleaf = CollisionSystem::get()->_coll_leaf_lst.ary[j];
                if ( NULL == pleaf ) continue;

                ichr_a = ( CHR_REF )( pleaf->_index );

                do_insert = false;

                if ( BSP_LEAF_CHR == pleaf->_type && VALID_CHR_RANGE( ichr_a ) )
                {
                    // collided with a character
                    bool loc_reaffirms     = can_reaffirm;
                    bool loc_needs_bump    = needs_bump;
                    bool interaction_valid = false;

                    Object * pchr_a = _currentModule->getObjectHandler().get( ichr_a );

                    // can this particle affect the character through reaffirmation
                    if ( loc_reaffirms )
                    {
                        // does this interaction support affirmation?
                        if ( particle->damagetype != pchr_a->reaffirm_damagetype )
                        {
                            loc_reaffirms = false;
                        }

                        // if it is already attached to this character, no more reaffirmation
                        if ( particle->getAttachedObject().get() == pchr_a )
                        {
                            loc_reaffirms = false;
                        }
                    }

                    // you can't be bumped by items that you are attached to
                    if ( loc_needs_bump && particle->getAttachedObject().get() == pchr_a )
                    {
                        loc_needs_bump = false;
                    }

                    // can this character affect this particle through bumping?
                    if ( loc_needs_bump )
                    {
                        // the valid bump interactions
                        bool end_money  = TO_C_BOOL(( particle->getProfile()->bump_money > 0 ) && pchr_a->cangrabmoney );
                        bool end_bump   = TO_C_BOOL(( particle->getProfile()->end_bump ) && ( 0 != pchr_a->bump_stt.size ) );
                        bool end_ground = TO_C_BOOL(( particle->getProfile()->end_ground ) && (( 0 != pchr_a->bump_stt.size ) || pchr_a->platform ) );

                        if ( !end_money && !end_bump && !end_ground )
                        {
                            loc_needs_bump = false;
                        }
                    }

                    // do a little more logic on this to determine whether the collision is valid
                    interaction_valid = false;
                    if ( loc_reaffirms || loc_needs_bump )
                    {
                        if ( detect_chr_prt_interaction_valid( ichr_a, particle->getParticleID() ) )
                        {
                            interaction_valid = true;
                        }
                        else
                        {
                            interaction_valid = false;
                        }
                    }

                    // only do the more expensive calculation if the
                    // particle can interact with the object
                    if ( interaction_valid )
                    {
                        CoNode_t::ctor( &tmp_codata );

                        // do a simple test, since I do not want to resolve the ObjectProfile for these objects here
                        test_platform = EMPTY_BIT_FIELD;
                        if ( pchr_a->platform && ( SPRITE_SOLID == particle->type ) ) SET_BIT( test_platform, PHYS_PLATFORM_OBJ1 );

                        // detect a when the possible collision occurred
                        if (phys_intersect_oct_bb(pchr_a->chr_min_cv, pchr_a->getPosition(), pchr_a->vel, particle->prt_max_cv, particle->getPosition(), particle->vel, test_platform, tmp_codata.cv, &(tmp_codata.tmin), &(tmp_codata.tmax)))
                        {

                            tmp_codata.chra = ichr_a;
                            tmp_codata.prtb = particle->getParticleID();

                            do_insert = true;
                        }
                    }
                }
                else if ( BSP_LEAF_PRT == pleaf->_type )
                {
                    // this should never happen
                }

                if ( do_insert )
                {
					CoHashList_insert_unique(coHashList, &tmp_codata, collNodeAry, hashNodeAry);
                }
            }
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool fill_bumplists()
{
    /// @brief Clear, then fill the chr and prt (aka obj) BSPs for this frame.
    ///
    /// @note Do not use BSP_tree_t::prune every frame, because the number of pre-allocated branches can be quite
	/// large. Instead, just remove the leaves from the tree, fill the tree, and then prune any empty branches.

    // empty out the BSP node lists
    chr_BSP_removeAllLeaves();
    prt_BSP_removeAllLeaves();

    // fill up the BSP list based on the current locations
    chr_BSP_fill();
    prt_BSP_fill();

    // Remove empty branches from the tree.
    if (63 == ( game_frame_all & 63))
    {
		size_t pruned;
		log_info("begin pruning\n");
        pruned = getChrBSP()->prune();
		/*if (pruned)*/
		{
			size_t free, used;
			getChrBSP()->getStats(free, used);
			std::ostringstream msg;
			msg << __FILE__ << ":" << __LINE__ << ": "
				<< "pruned: " << pruned << ", "
			    << "free:   " << free << ", "
				<< "used:   " << used << std::endl;
			log_info("%s", msg.str().c_str());
		}
        pruned = getPrtBSP()->prune();
		/*if (pruned)*/
		{
			size_t numFree, used;
			getPrtBSP()->getStats(numFree, used);
			std::ostringstream msg;
			msg << __FILE__ << ":" << __LINE__ << ": "
				<< "pruned: " << pruned << ", "
				<< "free:   " << numFree << ", "
				<< "used:   " << used << std::endl;
			log_info("%s", msg.str().c_str());
		}
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool do_chr_platform_detection( const CHR_REF ichr_a, const CHR_REF ichr_b )
{
    Object * pchr_a, * pchr_b;

    bool platform_a, platform_b;

    oct_vec_v2_t odepth;
    bool collide_x  = false;
    bool collide_y  = false;
    bool collide_xy = false;
    bool collide_yx = false;
    bool collide_z  = false;
    bool chara_on_top;

    // make sure that A is valid
    if ( !_currentModule->getObjectHandler().exists( ichr_a ) ) return false;
    pchr_a = _currentModule->getObjectHandler().get( ichr_a );

    // make sure that B is valid
    if ( !_currentModule->getObjectHandler().exists( ichr_b ) ) return false;
    pchr_b = _currentModule->getObjectHandler().get( ichr_b );

    // if you are mounted, only your mount is affected by platforms
    if ( _currentModule->getObjectHandler().exists( pchr_a->attachedto ) || _currentModule->getObjectHandler().exists( pchr_b->attachedto ) ) return false;

    // only check possible object-platform interactions
    platform_a = TO_C_BOOL( pchr_b->canuseplatforms && pchr_a->platform );
    platform_b = TO_C_BOOL( pchr_a->canuseplatforms && pchr_b->platform );
    if ( !platform_a && !platform_b ) return false;

    //---- since we are doing bump_all_mounts() before bump_all_platforms()
    // mount detection is done before platform attachment, and these lines of code
    // aren't needed
    //
    //// If we can mount this platform, skip it
    //mount_a = chr_can_mount( ichr_b, ichr_a );
    //if ( mount_a && pchr_a->enviro.level < pchr_b->getPosZ() + pchr_b->bump.height + PLATTOLERANCE )
    //    return false;
    //
    //// If we can mount this platform, skip it
    //mount_b = chr_can_mount( ichr_a, ichr_b );
    //if ( mount_b && pchr_b->enviro.level < pchr_a->getPosZ() + pchr_a->bump.height + PLATTOLERANCE )
    //    return false;

	odepth[OCT_Z] = std::min(pchr_b->chr_min_cv._maxs[OCT_Z] + pchr_b->getPosZ(), pchr_a->chr_min_cv._maxs[OCT_Z] + pchr_a->getPosZ()) -
                    std::max( pchr_b->chr_min_cv._mins[OCT_Z] + pchr_b->getPosZ(), pchr_a->chr_min_cv._mins[OCT_Z] + pchr_a->getPosZ() );

    collide_z  = TO_C_BOOL( odepth[OCT_Z] > -PLATTOLERANCE && odepth[OCT_Z] < PLATTOLERANCE );

    if ( !collide_z ) return false;

    // determine how the characters can be attached
    chara_on_top = true;
    odepth[OCT_Z] = 2 * PLATTOLERANCE;
    if ( platform_a && platform_b )
    {
        float depth_a, depth_b;

        depth_a = ( pchr_b->getPosZ() + pchr_b->chr_min_cv._maxs[OCT_Z] ) - ( pchr_a->getPosZ() + pchr_a->chr_min_cv._mins[OCT_Z] );
        depth_b = ( pchr_a->getPosZ() + pchr_a->chr_min_cv._maxs[OCT_Z] ) - ( pchr_b->getPosZ() + pchr_b->chr_min_cv._mins[OCT_Z] );

        odepth[OCT_Z] = std::min( pchr_b->getPosZ() + pchr_b->chr_min_cv._maxs[OCT_Z], pchr_a->getPosZ() + pchr_a->chr_min_cv._maxs[OCT_Z] ) -
                        std::max( pchr_b->getPosZ() + pchr_b->chr_min_cv._mins[OCT_Z], pchr_a->getPosZ() + pchr_a->chr_min_cv._mins[OCT_Z] );

		chara_on_top = TO_C_BOOL(std::abs(odepth[OCT_Z] - depth_a) < std::abs(odepth[OCT_Z] - depth_b));

        // the collision is determined by the platform size
        if ( chara_on_top )
        {
            // size of a doesn't matter
            odepth[OCT_X]  = std::min(( pchr_b->chr_min_cv._maxs[OCT_X] + pchr_b->getPosX() ) - pchr_a->getPosX(),
                                        pchr_a->getPosX() - ( pchr_b->chr_min_cv._mins[OCT_X] + pchr_b->getPosX() ) );

            odepth[OCT_Y]  = std::min(( pchr_b->chr_min_cv._maxs[OCT_Y] + pchr_b->getPosY() ) -  pchr_a->getPosY(),
                                        pchr_a->getPosY() - ( pchr_b->chr_min_cv._mins[OCT_Y] + pchr_b->getPosY() ) );

            odepth[OCT_XY] = std::min(( pchr_b->chr_min_cv._maxs[OCT_XY] + ( pchr_b->getPosX() + pchr_b->getPosY() ) ) - ( pchr_a->getPosX() + pchr_a->getPosY() ),
                                      ( pchr_a->getPosX() + pchr_a->getPosY() ) - ( pchr_b->chr_min_cv._mins[OCT_XY] + ( pchr_b->getPosX() + pchr_b->getPosY() ) ) );

            odepth[OCT_YX] = std::min(( pchr_b->chr_min_cv._maxs[OCT_YX] + ( -pchr_b->getPosX() + pchr_b->getPosY() ) ) - ( -pchr_a->getPosX() + pchr_a->getPosY() ),
                                      ( -pchr_a->getPosX() + pchr_a->getPosY() ) - ( pchr_b->chr_min_cv._mins[OCT_YX] + ( -pchr_b->getPosX() + pchr_b->getPosY() ) ) );
        }
        else
        {
            // size of b doesn't matter

            odepth[OCT_X]  = std::min(( pchr_a->chr_min_cv._maxs[OCT_X] + pchr_a->getPosX() ) - pchr_b->getPosX(),
                                        pchr_b->getPosX() - ( pchr_a->chr_min_cv._mins[OCT_X] + pchr_a->getPosX() ) );

            odepth[OCT_Y]  = std::min(( pchr_a->chr_min_cv._maxs[OCT_Y] + pchr_a->getPosY() ) -  pchr_b->getPosY(),
                                        pchr_b->getPosY() - ( pchr_a->chr_min_cv._mins[OCT_Y] + pchr_a->getPosY() ) );

            odepth[OCT_XY] = std::min(( pchr_a->chr_min_cv._maxs[OCT_XY] + ( pchr_a->getPosX() + pchr_a->getPosY() ) ) - ( pchr_b->getPosX() + pchr_b->getPosY() ),
                                      ( pchr_b->getPosX() + pchr_b->getPosY() ) - ( pchr_a->chr_min_cv._mins[OCT_XY] + ( pchr_a->getPosX() + pchr_a->getPosY() ) ) );

            odepth[OCT_YX] = std::min(( pchr_a->chr_min_cv._maxs[OCT_YX] + ( -pchr_a->getPosX() + pchr_a->getPosY() ) ) - ( -pchr_b->getPosX() + pchr_b->getPosY() ),
                                      ( -pchr_b->getPosX() + pchr_b->getPosY() ) - ( pchr_a->chr_min_cv._mins[OCT_YX] + ( -pchr_a->getPosX() + pchr_a->getPosY() ) ) );
        }
    }
    else if ( platform_a )
    {
        chara_on_top = false;
        odepth[OCT_Z] = ( pchr_a->getPosZ() + pchr_a->chr_min_cv._maxs[OCT_Z] ) - ( pchr_b->getPosZ() + pchr_b->chr_min_cv._mins[OCT_Z] );

        // size of b doesn't matter

        odepth[OCT_X] = std::min((pchr_a->chr_min_cv._maxs[OCT_X] + pchr_a->getPosX() ) - pchr_b->getPosX(),
                                  pchr_b->getPosX() - ( pchr_a->chr_min_cv._mins[OCT_X] + pchr_a->getPosX() ) );

		odepth[OCT_Y] = std::min((pchr_a->chr_min_cv._maxs[OCT_Y] + pchr_a->getPosY()) - pchr_b->getPosY(),
                                  pchr_b->getPosY() - ( pchr_a->chr_min_cv._mins[OCT_Y] + pchr_a->getPosY() ) );

		odepth[OCT_XY] = std::min((pchr_a->chr_min_cv._maxs[OCT_XY] + (pchr_a->getPosX() + pchr_a->getPosY())) - (pchr_b->getPosX() + pchr_b->getPosY()),
                                  ( pchr_b->getPosX() + pchr_b->getPosY() ) - ( pchr_a->chr_min_cv._mins[OCT_XY] + ( pchr_a->getPosX() + pchr_a->getPosY() ) ) );

		odepth[OCT_YX] = std::min((pchr_a->chr_min_cv._maxs[OCT_YX] + (-pchr_a->getPosX() + pchr_a->getPosY())) - (-pchr_b->getPosX() + pchr_b->getPosY()),
                                  ( -pchr_b->getPosX() + pchr_b->getPosY() ) - ( pchr_a->chr_min_cv._mins[OCT_YX] + ( -pchr_a->getPosX() + pchr_a->getPosY() ) ) );
    }
    else if ( platform_b )
    {
        chara_on_top = true;
        odepth[OCT_Z] = ( pchr_b->getPosZ() + pchr_b->chr_min_cv._maxs[OCT_Z] ) - ( pchr_a->getPosZ() + pchr_a->chr_min_cv._mins[OCT_Z] );

        // size of a doesn't matter
		odepth[OCT_X] = std::min((pchr_b->chr_min_cv._maxs[OCT_X] + pchr_b->getPosX()) - pchr_a->getPosX(),
                                  pchr_a->getPosX() - ( pchr_b->chr_min_cv._mins[OCT_X] + pchr_b->getPosX() ) );

		odepth[OCT_Y] = std::min(pchr_b->chr_min_cv._maxs[OCT_Y] + (pchr_b->getPosY() - pchr_a->getPosY()),
                                 ( pchr_a->getPosY() - pchr_b->chr_min_cv._mins[OCT_Y] ) + pchr_b->getPosY() );

		odepth[OCT_XY] = std::min((pchr_b->chr_min_cv._maxs[OCT_XY] + (pchr_b->getPosX() + pchr_b->getPosY())) - (pchr_a->getPosX() + pchr_a->getPosY()),
                                  ( pchr_a->getPosX() + pchr_a->getPosY() ) - ( pchr_b->chr_min_cv._mins[OCT_XY] + ( pchr_b->getPosX() + pchr_b->getPosY() ) ) );

        odepth[OCT_YX] = std::min(( pchr_b->chr_min_cv._maxs[OCT_YX] + ( -pchr_b->getPosX() + pchr_b->getPosY() ) ) - ( -pchr_a->getPosX() + pchr_a->getPosY() ),
                                  ( -pchr_a->getPosX() + pchr_a->getPosY() ) - ( pchr_b->chr_min_cv._mins[OCT_YX] + ( -pchr_b->getPosX() + pchr_b->getPosY() ) ) );

    }

    collide_x  = TO_C_BOOL( odepth[OCT_X]  > 0.0f );
    collide_y  = TO_C_BOOL( odepth[OCT_Y]  > 0.0f );
    collide_xy = TO_C_BOOL( odepth[OCT_XY] > 0.0f );
    collide_yx = TO_C_BOOL( odepth[OCT_YX] > 0.0f );
    collide_z  = TO_C_BOOL( odepth[OCT_Z] > -PLATTOLERANCE && odepth[OCT_Z] < PLATTOLERANCE );

    if ( collide_x && collide_y && collide_xy && collide_yx && collide_z )
    {
        // check for the best possible attachment
        if ( chara_on_top )
        {
            if ( pchr_b->getPosZ() + pchr_b->chr_min_cv._maxs[OCT_Z] > pchr_a->targetplatform_level )
            {
                // set, but do not attach the platforms yet
                pchr_a->targetplatform_level = pchr_b->getPosZ() + pchr_b->chr_min_cv._maxs[OCT_Z];
                pchr_a->targetplatform_ref   = ichr_b;
            }
        }
        else
        {
            if ( pchr_a->getPosZ() + pchr_a->chr_min_cv._maxs[OCT_Z] > pchr_b->targetplatform_level )
            {
                // set, but do not attach the platforms yet
                pchr_b->targetplatform_level = pchr_a->getPosZ() + pchr_a->chr_min_cv._maxs[OCT_Z];
                pchr_b->targetplatform_ref   = ichr_a;
            }
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool do_prt_platform_detection( const CHR_REF ichr_a, const PRT_REF iprt_b )
{
    Object * pchr_a;

    bool platform_a;

    oct_vec_v2_t odepth;
    bool collide_x  = false;
    bool collide_y  = false;
    bool collide_xy = false;
    bool collide_yx = false;
    bool collide_z  = false;

    // make sure that A is valid
    if ( !_currentModule->getObjectHandler().exists( ichr_a ) ) return false;
    pchr_a = _currentModule->getObjectHandler().get( ichr_a );

    // make sure that B is valid
    const std::shared_ptr<Ego::Particle> &pprt_b = ParticleHandler::get()[iprt_b];
    if ( !pprt_b || pprt_b->isTerminated() ) return false;

    // if you are mounted, only your mount is affected by platforms
    if ( _currentModule->getObjectHandler().exists( pchr_a->attachedto ) || pprt_b->isAttached() ) return false;

    // only check possible object-platform interactions
    platform_a = /* pprt_b->canuseplatforms && */ pchr_a->platform;
    if ( !platform_a ) return false;

    odepth[OCT_Z]  = std::min( pprt_b->prt_max_cv._maxs[OCT_Z] + pprt_b->pos[kZ], pchr_a->chr_min_cv._maxs[OCT_Z] + pchr_a->getPosZ() ) -
                     std::max( pprt_b->prt_max_cv._mins[OCT_Z] + pprt_b->pos[kZ], pchr_a->chr_min_cv._mins[OCT_Z] + pchr_a->getPosZ() );

	collide_z = TO_C_BOOL(odepth[OCT_Z] > -PLATTOLERANCE && odepth[OCT_Z] < PLATTOLERANCE);

    if ( !collide_z ) return false;

    // determine how the characters can be attached
    odepth[OCT_Z] = ( pchr_a->getPosZ() + pchr_a->chr_min_cv._maxs[OCT_Z] ) - ( pprt_b->pos[kZ] + pprt_b->prt_max_cv._mins[OCT_Z] );

    // size of b doesn't matter

	odepth[OCT_X] = std::min((pchr_a->chr_min_cv._maxs[OCT_X] + pchr_a->getPosX()) - pprt_b->pos[kX],
                              pprt_b->pos[kX] - ( pchr_a->chr_min_cv._mins[OCT_X] + pchr_a->getPosX() ) );

    odepth[OCT_Y]  = std::min(( pchr_a->chr_min_cv._maxs[OCT_Y] + pchr_a->getPosY() ) -  pprt_b->pos[kY],
                                pprt_b->pos[kY] - ( pchr_a->chr_min_cv._mins[OCT_Y] + pchr_a->getPosY() ) );

    odepth[OCT_XY] = std::min(( pchr_a->chr_min_cv._maxs[OCT_XY] + ( pchr_a->getPosX() + pchr_a->getPosY() ) ) - ( pprt_b->pos[kX] + pprt_b->pos[kY] ),
                              ( pprt_b->pos[kX] + pprt_b->pos[kY] ) - ( pchr_a->chr_min_cv._mins[OCT_XY] + ( pchr_a->getPosX() + pchr_a->getPosY() ) ) );

    odepth[OCT_YX] = std::min(( pchr_a->chr_min_cv._maxs[OCT_YX] + ( -pchr_a->getPosX() + pchr_a->getPosY() ) ) - ( -pprt_b->pos[kX] + pprt_b->pos[kY] ),
                              ( -pprt_b->pos[kX] + pprt_b->pos[kY] ) - ( pchr_a->chr_min_cv._mins[OCT_YX] + ( -pchr_a->getPosX() + pchr_a->getPosY() ) ) );

    collide_x  = TO_C_BOOL( odepth[OCT_X]  > 0.0f );
    collide_y  = TO_C_BOOL( odepth[OCT_Y]  > 0.0f );
    collide_xy = TO_C_BOOL( odepth[OCT_XY] > 0.0f );
    collide_yx = TO_C_BOOL( odepth[OCT_YX] > 0.0f );
    collide_z  = TO_C_BOOL( odepth[OCT_Z] > -PLATTOLERANCE && odepth[OCT_Z] < PLATTOLERANCE );

    if ( collide_x && collide_y && collide_xy && collide_yx && collide_z )
    {
        // check for the best possible attachment
        if ( pchr_a->getPosZ() + pchr_a->chr_min_cv._maxs[OCT_Z] > pprt_b->targetplatform_level )
        {
            // set, but do not attach the platforms yet
            pprt_b->targetplatform_level = pchr_a->getPosZ() + pchr_a->chr_min_cv._maxs[OCT_Z];
            pprt_b->targetplatform_ref   = ichr_a;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void bump_all_objects()
{
    /// @author ZZ
    /// @details This function sets handles characters hitting other characters or particles
    size_t        co_node_count;

    // Get the collision hash table.
    CoHashList_t *hash = CollisionSystem::get()->_hash;
    if (!hash)
    {
        log_error( "bump_all_objects() - cannot access the CoHashList_t singleton" );
		return;
    }

    // Reset the collision node magazine.
    CollisionSystem::get()->_cn_ary_2.reset();

    // Reset the hash node magazine.
    CollisionSystem::get()->_hn_ary_2.reset();

    // fill up the BSP structures
    fill_bumplists();

    // use the BSP structures to detect possible binary interactions
    fill_interaction_list(hash, CollisionSystem::get()->_cn_ary_2, CollisionSystem::get()->_hn_ary_2);

    // convert the CHashList_t into a CoNode_ary_t and sort
    co_node_count = hash->getSize();

    if ( co_node_count > 0 )
    {
        hash_list_iterator_t it;

        CollisionSystem::get()->_coll_node_lst.clear();

		it.ctor();
        hash_list_iterator_set_begin(&it, hash);
        for (/* Nothing. */; !hash_list_iterator_done(&it, hash); hash_list_iterator_next(&it, hash))
        {
            CoNode_t *coNode = (CoNode_t *)hash_list_iterator_ptr(&it);
            if (NULL == coNode) break;

            CollisionSystem::get()->_coll_node_lst.push_back(*coNode);
        }

        if (CollisionSystem::get()->_coll_node_lst.size() > 1)
        {
            // arrange the actual nodes by time order
            qsort(CollisionSystem::get()->_coll_node_lst.ary, CollisionSystem::get()->_coll_node_lst.size(),
                  sizeof(CoNode_t),(int (*)(const void *,const void *))(&CoNode_t::cmp));
        }

        // handle interaction with mounts
        // put this before platforms, otherwise pointing is just too hard
        bump_all_mounts(&CollisionSystem::get()->_coll_node_lst);

        // handle interaction with platforms
        bump_all_platforms(&CollisionSystem::get()->_coll_node_lst);

        // handle all the collisions
        bump_all_collisions(&CollisionSystem::get()->_coll_node_lst);
    }

#if 0
    // The following functions need to be called any time you actually change a charcter's position
    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator())
    {
        keep_weapons_with_holder(object);
        chr_update_matrix(object.get(), true);
    }

    //keep_weapons_with_holders();
    attach_all_particles();
    //update_all_character_matrices();
#endif
}

//--------------------------------------------------------------------------------------------
bool bump_all_platforms( Ego::DynamicArray<CoNode_t> *pcn_ary )
{
    /// @author BB
    /// @details Detect all character and particle interactions with platforms, then attach them.
    ///
    /// @note it is important to only attach the character to a platform once, so its
    ///  weight does not get applied to multiple platforms
    ///
    /// @note the function move_one_character_get_environment() has already been called from within the
    ///  move_one_character() function, so the environment has already been determined this round

	if ( NULL == pcn_ary ) return false;

    //---- Detect all platform attachments
    for (size_t cnt = 0; cnt < pcn_ary->size(); cnt++ )
    {
		CoNode_t *d = pcn_ary->ary + cnt;

        // only look at character-platform or particle-platform interactions interactions
        if ( INVALID_PRT_REF != d->prta && INVALID_PRT_REF != d->prtb ) continue;

        if ( INVALID_CHR_REF != d->chra && INVALID_CHR_REF != d->chrb )
        {
            do_chr_platform_detection( d->chra, d->chrb );
        }
        else if ( INVALID_CHR_REF != d->chra && INVALID_PRT_REF != d->prtb )
        {
            do_prt_platform_detection( d->chra, d->prtb );
        }
        if ( INVALID_PRT_REF != d->prta && INVALID_CHR_REF != d->chrb )
        {
            do_prt_platform_detection( d->chrb, d->prta );
        }
    }

    //---- Do the actual platform attachments.

    // Doing the attachments after detecting the best platform
    // prevents an object from attaching it to multiple platforms as it
    // is still trying to find the best one
    for (size_t cnt = 0; cnt < pcn_ary->size(); cnt++ )
    {
		CoNode_t *d = pcn_ary->ary + cnt;

        // only look at character-character interactions
        //if ( INVALID_PRT_REF != d->prta && INVALID_PRT_REF != d->prtb ) continue;

        if ( INVALID_CHR_REF != d->chra && INVALID_CHR_REF != d->chrb )
        {
            if ( _currentModule->getObjectHandler().exists( d->chra ) && _currentModule->getObjectHandler().exists( d->chrb ) )
            {
                if ( _currentModule->getObjectHandler().get(d->chra)->targetplatform_ref == d->chrb )
                {
                    attach_Objecto_platform( _currentModule->getObjectHandler().get( d->chra ), _currentModule->getObjectHandler().get( d->chrb ) );
                }
                else if ( _currentModule->getObjectHandler().get(d->chrb)->targetplatform_ref == d->chra )
                {
                    attach_Objecto_platform( _currentModule->getObjectHandler().get( d->chrb ), _currentModule->getObjectHandler().get( d->chra ) );
                }

            }
        }
        else if ( INVALID_CHR_REF != d->chra && INVALID_PRT_REF != d->prtb )
        {
            if ( _currentModule->getObjectHandler().exists( d->chra ) && ParticleHandler::get()[d->prtb] != nullptr )
            {
                if ( ParticleHandler::get()[d->prtb]->targetplatform_ref == d->chra )
                {
                    attach_prt_to_platform( ParticleHandler::get()[d->prtb].get(), _currentModule->getObjectHandler().get( d->chra ) );
                }
            }
        }
        else if ( INVALID_CHR_REF != d->chrb && INVALID_PRT_REF != d->prta )
        {
            if ( _currentModule->getObjectHandler().exists( d->chrb ) && ParticleHandler::get()[d->prta] != nullptr )
            {
                if ( ParticleHandler::get()[d->prta]->targetplatform_ref == d->chrb )
                {
                    attach_prt_to_platform(ParticleHandler::get()[d->prta].get(), _currentModule->getObjectHandler().get(d->chrb));
                }
            }
        }
    }

    //---- remove any bad platforms

    // attach_prt_to_platform() erases targetplatform_ref, so any character with
    // (INVALID_CHR_REF != targetplatform_ref) must not be connected to a platform at all
    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator())
    {
        if ( object->onwhichplatform_update < update_wld && _currentModule->getObjectHandler().exists(object->onwhichplatform_ref) )
        {
            detach_character_from_platform( object.get() );
        }
    }

    // attach_prt_to_platform() erases targetplatform_ref, so any particle with
    // (INVALID_CHR_REF != targetplatform_ref) must not be connected to a platform at all
    for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
    {
        if(particle->isTerminated()) continue;

        if ( INVALID_CHR_REF != particle->onwhichplatform_ref && particle->onwhichplatform_update < update_wld )
        {
            detach_particle_from_platform( particle.get() );
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool bump_all_mounts( Ego::DynamicArray<CoNode_t> *pcn_ary )
{
    /// @author BB
    /// @details Detect all character interactions with mounts, then attach them.
    if ( NULL == pcn_ary ) return false;

    // Do mounts
    for (size_t cnt = 0; cnt < pcn_ary->size(); cnt++)
    {
		CoNode_t *d = pcn_ary->ary + cnt;

        // only look at character-character interactions
        if ( INVALID_CHR_REF == d->chra || INVALID_CHR_REF == d->chrb ) continue;

        bump_one_mount( d->chra, d->chrb );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool bump_all_collisions( Ego::DynamicArray<CoNode_t> *pcn_ary )
{
    /// @author BB
    /// @details Detect all character-character and character-particle collsions (with exclusions
    ///               for the mounts and platforms found in the previous steps)

    // blank the accumulators
    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator())
    {
        phys_data_clear( &( object->phys ) );
    }

    for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
    {
        phys_data_clear( &particle->phys );
    }

    // do all interactions
    for (size_t cnt = 0; cnt < pcn_ary->size(); cnt++ )
    {
        bool handled = false;

        // use this form of the function call so that we could add more modules or
        // rearrange them without needing to change anything
        if ( !handled )
        {
            handled = do_chr_chr_collision( pcn_ary->ary + cnt );
        }

        if ( !handled )
        {
            handled = do_chr_prt_collision( pcn_ary->ary + cnt );
        }
    }

    // accumulate the accumulators
    for(const std::shared_ptr<Object> &pchr : _currentModule->getObjectHandler().iterator())
    {
        float tmpx, tmpy, tmpz;
        float bump_str;
        bool position_updated = false;
        fvec3_t max_apos;

        fvec3_t tmp_pos;

        tmp_pos = pchr->getPosition();

        bump_str = 1.0f;
        if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) )
        {
            bump_str = 0.0f;
        }

        // do the "integration" of the accumulated accelerations
        pchr->vel += pchr->phys.avel;

        position_updated = false;

        // get a net displacement vector from aplat and acoll
        {
            // create a temporary apos_t
            apos_t  apos_tmp;

            // copy 1/2 of the data over
            apos_tmp = pchr->phys.aplat;

            // get the resultant apos_t
            apos_t::self_union( &apos_tmp, &( pchr->phys.acoll ) );

            // turn this into a vector
            apos_t::evaluate(&apos_tmp, max_apos);
        }

        // limit the size of the displacement
        max_apos[kX] = CLIP( max_apos[kX], -GRID_FSIZE, GRID_FSIZE );
        max_apos[kY] = CLIP( max_apos[kY], -GRID_FSIZE, GRID_FSIZE );
        max_apos[kZ] = CLIP( max_apos[kZ], -GRID_FSIZE, GRID_FSIZE );

        // do the "integration" on the position
        if (std::abs(max_apos[kX]) > 0.0f)
        {
            tmpx = tmp_pos[kX];
            tmp_pos[kX] += max_apos[kX];
            if ( EMPTY_BIT_FIELD != pchr->test_wall( tmp_pos, NULL ) )
            {
                // restore the old values
                tmp_pos[kX] = tmpx;
            }
            else
            {
                //pchr->vel[kX] += pchr->phys.apos_coll[kX] * bump_str;
                position_updated = true;
            }
        }

        if (std::abs(max_apos[kY]) > 0.0f)
        {
            tmpy = tmp_pos[kY];
            tmp_pos[kY] += max_apos[kY];
            if ( EMPTY_BIT_FIELD != pchr->test_wall( tmp_pos, NULL ) )
            {
                // restore the old values
                tmp_pos[kY] = tmpy;
            }
            else
            {
                //pchr->vel[kY] += pchr->phys.apos_coll[kY] * bump_str;
                position_updated = true;
            }
        }

        if (std::abs(max_apos[kZ]) > 0.0f)
        {
            tmpz = tmp_pos[kZ];
            tmp_pos[kZ] += max_apos[kZ];
            if ( tmp_pos[kZ] < pchr->enviro.floor_level )
            {
                // restore the old values
                tmp_pos[kZ] = pchr->enviro.floor_level;
                if ( pchr->vel[kZ] < 0 )
                {
                    pchr->vel[kZ] += -( 1.0f + pchr->getProfile()->getBounciness() ) * pchr->vel[kZ];
                }
                position_updated = true;
            }
            else
            {
                //pchr->vel[kZ] += pchr->phys.apos_coll[kZ] * bump_str;
                position_updated = true;
            }
        }

        if ( position_updated )
        {
            pchr->setPosition(tmp_pos);
        }
    }

    // accumulate the accumulators
    for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
    {
        float tmpx, tmpy, tmpz;
        float bump_str;
        bool position_updated = false;
        fvec3_t max_apos;

        if(particle->isTerminated()) {
            continue;
        }

        fvec3_t tmp_pos = particle->getPosition();

        bump_str = 1.0f;
        if ( particle->isAttached() )
        {
            bump_str = 0.0f;
        }

        // do the "integration" of the accumulated accelerations
		particle->vel += particle->phys.avel;

        position_updated = false;

        // get a net displacement vector from aplat and acoll
        {
            // create a temporary apos_t
            apos_t  apos_tmp;

            // copy 1/2 of the data over
            apos_tmp = particle->phys.aplat;

            // get the resultant apos_t
            apos_t::self_union( &apos_tmp, &( particle->phys.acoll ) );

            // turn this into a vector
            apos_t::evaluate(&apos_tmp, max_apos);
        }

        max_apos[kX] = CLIP( max_apos[kX], -GRID_FSIZE, GRID_FSIZE );
        max_apos[kY] = CLIP( max_apos[kY], -GRID_FSIZE, GRID_FSIZE );
        max_apos[kZ] = CLIP( max_apos[kZ], -GRID_FSIZE, GRID_FSIZE );

        // do the "integration" on the position
        if (std::abs(max_apos[kX]) > 0.0f)
        {
            tmpx = tmp_pos[kX];
            tmp_pos[kX] += max_apos[kX];
            if ( EMPTY_BIT_FIELD != particle->test_wall( tmp_pos, NULL ) )
            {
                // restore the old values
                tmp_pos[kX] = tmpx;
            }
            else
            {
                //bdl.prt_ptr->vel[kX] += bdl.prt_ptr->phys.apos_coll[kX] * bump_str;
                position_updated = true;
            }
        }

        if (std::abs(max_apos[kY]) > 0.0f)
        {
            tmpy = tmp_pos[kY];
            tmp_pos[kY] += max_apos[kY];
            if ( EMPTY_BIT_FIELD != particle->test_wall( tmp_pos, NULL ) )
            {
                // restore the old values
                tmp_pos[kY] = tmpy;
            }
            else
            {
                //bdl.prt_ptr->vel[kY] += bdl.prt_ptr->phys.apos_coll[kY] * bump_str;
                position_updated = true;
            }
        }

        if (std::abs(max_apos[kZ]) > 0.0f)
        {
            tmpz = tmp_pos[kZ];
            tmp_pos[kZ] += max_apos[kZ];
            if ( tmp_pos[kZ] < particle->enviro.floor_level )
            {
                // restore the old values
                tmp_pos[kZ] = particle->enviro.floor_level;
                if ( particle->vel[kZ] < 0 )
                {
                    particle->vel[kZ] += -( 1.0f + particle->getProfile()->dampen ) * particle->vel[kZ];
                }
                position_updated = true;
            }
            else
            {
                //bdl.prt_ptr->vel[kZ] += bdl.prt_ptr->phys.apos_coll[kZ] * bump_str;
                position_updated = true;
            }
        }

        // Change the direction of the particle
        if ( particle->getProfile()->rotatetoface )
        {
            // Turn to face new direction
            particle->facing = vec_to_facing( particle->vel[kX] , particle->vel[kY] );
        }

        if ( position_updated )
        {
            particle->setPosition(tmp_pos);
        }
    }

    // blank the accumulators
    for(const std::shared_ptr<Object> &pchr : _currentModule->getObjectHandler().iterator())
    {
        phys_data_clear( &( pchr->phys ) );
    }

    for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
    {
        phys_data_clear( &particle->phys );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool bump_one_mount( const CHR_REF ichr_a, const CHR_REF ichr_b )
{
    fvec3_t vdiff = fvec3_t::zero();

    oct_vec_v2_t apos, bpos;

    bool mounted = false;
    bool handled = false;

    // make sure that A is valid
    const std::shared_ptr<Object> &pchr_a = _currentModule->getObjectHandler()[ichr_a];
    if(!pchr_a) {
        return false;
    }

    // make sure that B is valid
    const std::shared_ptr<Object> &pchr_b = _currentModule->getObjectHandler()[ichr_b];
    if(!pchr_b) {
        return false;
    }

    // find the difference in velocities
    vdiff = pchr_b->vel - pchr_a->vel;

    // can either of these objects mount the other?
    bool mount_a = pchr_b->canMount(pchr_a);
    bool mount_b = pchr_a->canMount(pchr_b);

    if ( !mount_a && !mount_b ) return false;

    // Ready for position calulations
    apos.ctor(pchr_a->getPosition() );
    bpos.ctor( pchr_b->getPosition() );

    // assume the worst
    mounted = false;

    // mount a on b ?
    if ( !mounted && mount_b )
    {
        oct_bb_t  tmp_cv, saddle_cv;

        //---- find out whether the object is overlapping with the saddle

        // the position of the saddle over the frame
        oct_bb_t::translate(pchr_b->slot_cv[SLOT_LEFT], pchr_b->getPosition(), tmp_cv);
        phys_expand_oct_bb(tmp_cv, pchr_b->vel, 0.0f, 1.0f, saddle_cv);

        if (oct_bb_t::contains(saddle_cv, apos))
        {
            oct_vec_v2_t saddle_pos;
            fvec3_t   pdiff;

            saddle_pos = saddle_cv.getMid();
            pdiff[kX] = saddle_pos[OCT_X] - apos[OCT_X];
            pdiff[kY] = saddle_pos[OCT_Y] - apos[OCT_Y];
            pdiff[kZ] = saddle_pos[OCT_Z] - apos[OCT_Z];

            if (pdiff.dot(vdiff) >= 0.0f)
            {
                // the rider is in a mountable position, don't do any more collisions
                // even if the object is doesn't actually mount
                handled = true;

                if ( rv_success == attach_character_to_mount( ichr_a, ichr_b, GRIP_ONLY ) )
                {
                    mounted = _currentModule->getObjectHandler().exists( pchr_a->attachedto );
                }
            }
        }
    }

    // mount b on a ?
    if ( !mounted && mount_a )
    {
        oct_bb_t  tmp_cv, saddle_cv;

        //---- find out whether the object is overlapping with the saddle

        // the position of the saddle over the frame
        oct_bb_t::translate(pchr_a->slot_cv[SLOT_LEFT], pchr_a->getPosition(), tmp_cv);
        phys_expand_oct_bb(tmp_cv, pchr_a->vel, 0.0f, 1.0f, saddle_cv);

        if (oct_bb_t::contains(saddle_cv, bpos))
        {
            oct_vec_v2_t saddle_pos;
            fvec3_t   pdiff;

            saddle_pos = saddle_cv.getMid();

            // vdiff is computed as b - a. keep the pdiff in the same sense
            pdiff[kX] = bpos[OCT_X] - saddle_pos[OCT_X];
            pdiff[kY] = bpos[OCT_Y] - saddle_pos[OCT_Y];
            pdiff[kZ] = bpos[OCT_Z] - saddle_pos[OCT_Z];

            if (pdiff.dot(vdiff ) >= 0.0f)
            {
                // the rider is in a mountable position, don't do any more collisions
                // even if the object is doesn't actually mount
                handled = true;

                if ( rv_success == attach_character_to_mount( ichr_b, ichr_a, GRIP_ONLY ) )
                {
                    mounted = _currentModule->getObjectHandler().exists( pchr_b->attachedto );
                }
            }
        }
    }

    return handled;
}

//--------------------------------------------------------------------------------------------
bool do_chr_platform_physics( Object * pitem, Object * pplat )
{
    // we know that ichr_a is a platform and ichr_b is on it
    Sint16 rot_a, rot_b;
    float lerp_z, vlerp_z;

    if ( !ACTIVE_PCHR( pitem ) ) return false;
    if ( !ACTIVE_PCHR( pplat ) ) return false;

    if ( pitem->onwhichplatform_ref != GET_INDEX_PCHR( pplat ) ) return false;

    // grab the pre-computed zlerp value, and map it to our needs
    lerp_z = 1.0f - pitem->enviro.zlerp;

    // if your velocity is going up much faster then the
    // platform, there is no need to suck you to the level of the platform
    // this was one of the things preventing you from jumping from platforms
    // properly
    vlerp_z = std::abs(pitem->vel[kZ] - pplat->vel[kZ]) / 5;
    vlerp_z  = 1.0f - CLIP( vlerp_z, 0.0f, 1.0f );

    // determine the rotation rates
    rot_b = pitem->ori.facing_z - pitem->ori_old.facing_z;
    rot_a = pplat->ori.facing_z - pplat->ori_old.facing_z;

    if ( lerp_z == 1.0f )
    {
        phys_data_sum_aplat_index( &( pitem->phys ), ( pitem->enviro.level - pitem->getPosZ() ) * 0.125f, kZ );
        phys_data_sum_avel_index( &( pitem->phys ), ( pplat->vel[kZ]  - pitem->vel[kZ] ) * 0.25f, kZ );
        pitem->ori.facing_z += ( rot_a - rot_b ) * PLATFORM_STICKINESS;
    }
    else
    {
        phys_data_sum_aplat_index( &( pitem->phys ), ( pitem->enviro.level - pitem->getPosZ() ) * 0.125f * lerp_z * vlerp_z, kZ );
        phys_data_sum_avel_index( &( pitem->phys ), ( pplat->vel[kZ]  - pitem->vel[kZ] ) * 0.25f * lerp_z * vlerp_z, kZ );
        pitem->ori.facing_z += ( rot_a - rot_b ) * PLATFORM_STICKINESS * lerp_z * vlerp_z;
    };

    return true;
}

//--------------------------------------------------------------------------------------------
#if 0
float estimate_chr_prt_normal( const Object * pchr, const prt_t * pprt, fvec3_t& nrm, fvec3_t& vdiff )
{
    fvec3_t collision_size;
    float dot;

    collision_size[kX] = std::max( pchr->chr_max_cv._maxs[OCT_X] - pchr->chr_max_cv._mins[OCT_X], 2.0f * pprt->bump_padded.size );
    if ( 0.0f == collision_size[kX] ) return -1.0f;

    collision_size[kY] = std::max( pchr->chr_max_cv._maxs[OCT_Y] - pchr->chr_max_cv._mins[OCT_Y], 2.0f * pprt->bump_padded.size );
    if ( 0.0f == collision_size[kY] ) return -1.0f;

    collision_size[kZ] = std::max( pchr->chr_max_cv._maxs[OCT_Z] - pchr->chr_max_cv._mins[OCT_Z], 2.0f * pprt->bump_padded.height );
    if ( 0.0f == collision_size[kZ] ) return -1.0f;

    // estimate the "normal" for the collision, using the center-of-mass difference
    nrm = pprt->pos - pchr->pos;
    nrm[kZ] -= 0.5f * (pchr->chr_max_cv._maxs[OCT_Z] + pchr->chr_max_cv._mins[OCT_Z]);

    // scale the collision box
    nrm[kX] /= collision_size[kX];
    nrm[kY] /= collision_size[kY];
    nrm[kZ] /= collision_size[kZ];

    // scale the normals so that the collision volume will act somewhat like a cylinder
    if ( pchr->platform )
    {
        nrm[kX] *= nrm[kX] * nrm[kX] + nrm[kY] * nrm[kY];
        nrm[kY] *= nrm[kX] * nrm[kX] + nrm[kY] * nrm[kY];

        nrm[kZ] *= nrm[kZ] * nrm[kZ];
    }

    // reject the reflection request if the particle is moving in the wrong direction
    vdiff = pchr->vel - pprt->vel;
    dot       = vdiff.dot(nrm);

    // we really never should have the condition that dot > 0, unless the particle is "fast"
    if ( dot >= 0.0f )
    {
        fvec3_t vtmp;

        // If the particle is "fast" relative to the object size, it can happen that the particle
        // can be more than halfway through the character before it is detected.

        vtmp[kX] = vdiff[kX] / collision_size[kX];
        vtmp[kY] = vdiff[kY] / collision_size[kY];
        vtmp[kZ] = vdiff[kZ] / collision_size[kZ];

        // If it is fast, re-evaluate the normal in a different way
        if ( vtmp.length_2() > 0.5f*0.5f )
        {
            // use the old position, which SHOULD be before the collision
            // to determine the normal
            nrm = pprt->pos_old - pchr->pos_old;
            nrm[kZ] -= 0.5f * (pchr->chr_max_cv._maxs[OCT_Z] + pchr->chr_max_cv._mins[OCT_Z]);

            // scale the collision box
            nrm[kX] /= collision_size[kX];
            nrm[kY] /= collision_size[kY];
            nrm[kZ] /= collision_size[kZ];

            // scale the z-normals so that the collision volume will act somewhat like a cylinder
            nrm[kZ] *= nrm[kZ] * nrm[kZ];
        }
    }

    // assume the function fails
    dot = 0.0f;

    // does the normal exist?
    if (nrm.length_abs() > 0.0f )
    {
        // Make the normal a unit normal
		nrm.normalize();

        // determine the actual dot product
        dot = vdiff.dot(nrm);
    }

    return dot;
}
#endif

//--------------------------------------------------------------------------------------------
#if 0
bool do_chr_chr_collision_pressure_normal(const Object *pchr_a, const Object *pchr_b, const float exponent, oct_vec_v2_t& odepth, fvec3_t& nrm, float& depth)
{
    oct_bb_t otmp_a, otmp_b;

    oct_bb_t::translate(pchr_a->chr_min_cv, pchr_a->getPosition(), otmp_a);
    oct_bb_t::translate(pchr_b->chr_min_cv, pchr_b->getPosition(), otmp_b);

    return phys_estimate_pressure_normal(otmp_a, otmp_b, exponent, odepth, nrm, depth);
}
#endif

//--------------------------------------------------------------------------------------------
bool do_chr_chr_collision( CoNode_t * d )
{
    CHR_REF ichr_a, ichr_b;
    Object * pchr_a, * pchr_b;

    float depth_min;
    float interaction_strength = 1.0f;

    float wta, wtb;
    float recoil_a, recoil_b;

    // object bounding boxes shifted so that they are in the correct place on the map
    oct_bb_t map_bb_a, map_bb_b;

    fvec3_t   nrm;
    int exponent = 1;

    oct_vec_v2_t odepth;
    bool    collision = false, bump = false, valid_normal = false;

    if ( NULL == d || INVALID_PRT_REF != d->prtb ) return false;
    ichr_a = d->chra;
    ichr_b = d->chrb;

    // make sure that it is on
    if ( !_currentModule->getObjectHandler().exists( ichr_a ) ) return false;
    pchr_a = _currentModule->getObjectHandler().get( ichr_a );

    // make sure that it is on
    if ( !_currentModule->getObjectHandler().exists( ichr_b ) ) return false;
    pchr_b = _currentModule->getObjectHandler().get( ichr_b );

    // skip objects that are inside inventories
    if ( _currentModule->getObjectHandler().exists( pchr_a->inwhich_inventory ) || _currentModule->getObjectHandler().exists( pchr_b->inwhich_inventory ) ) return false;

    // skip all objects that are mounted or attached to something
    if ( _currentModule->getObjectHandler().exists( pchr_a->attachedto ) || _currentModule->getObjectHandler().exists( pchr_b->attachedto ) ) return false;

    // platform interaction. if the onwhichplatform_ref is set, then
    // all collision tests have been met
    if ( ichr_a == pchr_b->onwhichplatform_ref )
    {
        if ( do_chr_platform_physics( pchr_b, pchr_a ) )
        {
            // this is handled
            return true;
        }
    }

    // platform interaction. if the onwhichplatform_ref is set, then
    // all collision tests have been met
    if ( ichr_b == pchr_a->onwhichplatform_ref )
    {
        if ( do_chr_platform_physics( pchr_a, pchr_b ) )
        {
            // this is handled
            return true;
        }
    }

    // items can interact with platforms but not with other characters/objects
    if ( pchr_a->isitem || pchr_b->isitem ) return false;

    // don't interact with your mount, or your held items
    if ( ichr_a == pchr_b->attachedto || ichr_b == pchr_a->attachedto ) return false;

    // don't do anything if there is no interaction strength
    if ( 0.0f == pchr_a->bump_stt.size || 0.0f == pchr_b->bump_stt.size ) return false;

    interaction_strength = 1.0f;
    interaction_strength *= pchr_a->inst.alpha * INV_FF;
    interaction_strength *= pchr_b->inst.alpha * INV_FF;

    // reduce your interaction strength if you have just detached from an object
    if ( pchr_a->dismount_object == ichr_b )
    {
        float dismount_lerp = ( float )pchr_a->dismount_timer / ( float )PHYS_DISMOUNT_TIME;
        dismount_lerp = CLIP( dismount_lerp, 0.0f, 1.0f );

        interaction_strength *= dismount_lerp;
    }

    if ( pchr_b->dismount_object == ichr_a )
    {
        float dismount_lerp = ( float )pchr_b->dismount_timer / ( float )PHYS_DISMOUNT_TIME;
        dismount_lerp = CLIP( dismount_lerp, 0.0f, 1.0f );

        interaction_strength *= dismount_lerp;
    }

    // seriously reduce the interaction_strength with mounts
    // this thould allow characters to mount certain mounts a lot easier
    if (( pchr_a->isMount() && INVALID_CHR_REF == pchr_a->holdingwhich[SLOT_LEFT] && !pchr_b->isMount() ) ||
        ( pchr_b->isMount() && INVALID_CHR_REF == pchr_b->holdingwhich[SLOT_LEFT] && !pchr_a->isMount() ) )
    {
        interaction_strength *= 0.75f;
    }

    // reduce the interaction strength with platforms
    // that are overlapping with the platform you are actually on
    if ( pchr_b->canuseplatforms && pchr_a->platform && INVALID_CHR_REF != pchr_b->onwhichplatform_ref && ichr_a != pchr_b->onwhichplatform_ref )
    {
        float lerp_z = ( pchr_b->getPosZ() - ( pchr_a->getPosZ() + pchr_a->chr_min_cv._maxs[OCT_Z] ) ) / PLATTOLERANCE;
        lerp_z = CLIP( lerp_z, -1.0f, 1.0f );

        if ( lerp_z >= 0.0f )
        {
            interaction_strength = 0.0f;
        }
        else
        {
            interaction_strength *= -lerp_z;
        }
    }

    if ( pchr_a->canuseplatforms && pchr_b->platform && INVALID_CHR_REF != pchr_a->onwhichplatform_ref && ichr_b != pchr_a->onwhichplatform_ref )
    {
        float lerp_z = ( pchr_a->getPosZ() - ( pchr_b->getPosZ() + pchr_b->chr_min_cv._maxs[OCT_Z] ) ) / PLATTOLERANCE;
        lerp_z = CLIP( lerp_z, -1.0f, +1.0f );

        if ( lerp_z >= 0.0f )
        {
            interaction_strength = 0.0f;
        }
        else
        {
            interaction_strength *= -lerp_z;
        }
    }

    // shift the character bounding boxes to be centered on their positions
    oct_bb_t::translate(pchr_a->chr_min_cv, pchr_a->getPosition(), map_bb_a);
    oct_bb_t::translate(pchr_b->chr_min_cv, pchr_b->getPosition(), map_bb_b);

    // make the object more like a table if there is a platform-like interaction
    exponent = 1.0f;
    if ( pchr_a->canuseplatforms && pchr_b->platform ) exponent += 2;
    if ( pchr_b->canuseplatforms && pchr_a->platform ) exponent += 2;

    // use the info from the collision volume to determine whether the objects are colliding
    collision = TO_C_BOOL( d->tmin > 0.0f );

    // estimate the collision normal at the point of contact
    valid_normal = false;
    depth_min    = 0.0f;
    if ( collision )
    {
        // find the collision volumes at 10% overlap
        oct_bb_t exp1, exp2;

        float tmp_min, tmp_max;

        tmp_min = d->tmin;
        tmp_max = d->tmin + ( d->tmax - d->tmin ) * 0.1f;

        // determine the expanded collision volumes for both objects
        phys_expand_oct_bb(map_bb_a, pchr_a->vel, tmp_min, tmp_max, exp1);
        phys_expand_oct_bb(map_bb_b, pchr_b->vel, tmp_min, tmp_max, exp2);

        valid_normal = phys_estimate_collision_normal(exp1, exp2, exponent, odepth, nrm, depth_min);
    }

    if ( !collision || depth_min <= 0.0f )
    {
        valid_normal = phys_estimate_pressure_normal(map_bb_a, map_bb_b, exponent, odepth, nrm, depth_min);
    }

    if ( depth_min <= 0.0f )
        return false;

    // if we can't obtain a valid collision normal, we fail
    if ( !valid_normal ) return false;

    //------------------
    // do character-character interactions

    // calculate a "mass" for each object, taking into account possible infinite masses
    get_chr_mass( pchr_a, &wta );
    get_chr_mass( pchr_b, &wtb );

    // make a special exception for interaction between "Mario platforms"
    if (( wta < 0.0f && pchr_a->platform ) && ( wtb < 0.0f && pchr_a->platform ) )
    {
        return false;
    }

    // make a special exception for immovable scenery objects
    // they can collide, but cannot push each other apart... that might mess up the scenery ;)
    if ( !collision && ( wta < 0.0f && 0.0f == pchr_a->maxaccel ) && ( wtb < 0.0f && 0.0f == pchr_b->maxaccel ) )
    {
        return false;
    }

    // determine the relative effect of impulses, given the known weights
    get_recoil_factors( wta, wtb, &recoil_a, &recoil_b );

    //---- calculate the character-character interactions
    {
        const float max_pressure_strength = 0.125f;
        const float pressure_strength     = max_pressure_strength * interaction_strength;

        fvec3_t   pdiff_a;

        bool need_displacement = false;
        bool need_velocity = false;

        fvec3_t   vdiff_a;

        if ( depth_min <= 0.0f || collision )
        {
            need_displacement = false;
			pdiff_a = fvec3_t::zero();
        }
        else
        {
            // add a small amount to the pressure difference so that
            // the function will actually separate the objects in a finite number
            // of iterations
            need_displacement = TO_C_BOOL(( recoil_a > 0.0f ) || ( recoil_b > 0.0f ) );
			pdiff_a = nrm * (depth_min + 1.0f);
        }

        // find the relative velocity
        vdiff_a = pchr_b->vel - pchr_a->vel;

        need_velocity = false;
        if (vdiff_a.length_abs() > 1e-6)
        {
            need_velocity = TO_C_BOOL(( recoil_a > 0.0f ) || ( recoil_b > 0.0f ) );
        }

        //---- handle the relative velocity
        if ( need_velocity )
        {

            // what type of interaction is this? (collision or pressure)
            if ( collision )
            {
                // !!!! COLLISION !!!!

                // an actual bump, use impulse to make the objects bounce appart

                fvec3_t vdiff_para_a, vdiff_perp_a;

                // generic coefficient of restitution.
                float cr = pchr_a->phys.dampen * pchr_b->phys.dampen;

                // decompose this relative to the collision normal
                fvec3_decompose(vdiff_a, nrm, vdiff_perp_a, vdiff_para_a);

                if (recoil_a > 0.0f)
                {
					fvec3_t vimp_a = vdiff_perp_a * +(recoil_a * (1.0f + cr) * interaction_strength);
                    phys_data_sum_avel(&(pchr_a->phys), vimp_a);
                }

                if (recoil_b > 0.0f)
                {
                    fvec3_t vimp_b = vdiff_perp_a * -(recoil_b * (1.0f + cr) * interaction_strength);
                    phys_data_sum_avel(&(pchr_b->phys), vimp_b);
                }

                // this was definitely a bump
                bump = true;
            }
            // ignore the case of both objects having infinite mass
            // this is normally due to two scenery objects being too close to each other
            else
            {
                // !!!! PRESSURE !!!!

                // not a bump at all. two objects are rubbing against one another
                // and continually overlapping.
                //
                // reduce the relative velocity if the objects are moving towards each other,
                // but ignore it if they are moving away.

                // use pressure to push them appart. reduce their relative velocities.

                float     vdot;

                // are the objects moving towards each other, or appart?
                vdot = vdiff_a.dot(nrm);

                if ( vdot < 0.0f )
                {
                    if (recoil_a > 0.0f)
                    {
                        fvec3_t vimp_a = vdiff_a * +(recoil_a * pressure_strength);
                        phys_data_sum_avel(&(pchr_a->phys), vimp_a);
                    }

                    if (recoil_b > 0.0f)
                    {
						fvec3_t vimp_b = vdiff_a * -(recoil_b * pressure_strength);
                        phys_data_sum_avel(&(pchr_b->phys), vimp_b);
                    }
                }

                // you could "bump" something if you changed your velocity, even if you were still touching
                bump = TO_C_BOOL(( pchr_a->vel.dot(nrm) * pchr_a->vel_old.dot(nrm) < 0 ) ||
                                 ( pchr_b->vel.dot(nrm) * pchr_b->vel_old.dot(nrm) < 0 ) );
            }

        }

        //---- fix the displacement regardless of what kind of interaction
        if ( need_displacement )
        {
            if ( recoil_a > 0.0f )
            {
                fvec3_t pimp_a = pdiff_a * +(recoil_a * pressure_strength);
                phys_data_sum_acoll(&(pchr_a->phys), pimp_a);
            }

            if ( recoil_b > 0.0f )
            {
				fvec3_t pimp_b = pdiff_a * -(recoil_b * pressure_strength);
                phys_data_sum_acoll(&(pchr_b->phys), pimp_b);
            }
        }

        //// add in the friction due to the "collision"
        //// assume coeff of friction of 0.5
        //if (vimp_a.length_abs() > 0.0f && vpara_a.length_abs() > 0.0f &&
        //    pchr_a->dismount_timer <= 0 )
        //{
        //    float imp, vel, factor;

        //    imp = 0.5f * vimp_a.length();
        //    vel = vpara.length();

        //    factor = imp / vel;
        //    factor = CLIP( factor, 0.0f, 1.0f );

        //    pchr_a->phys.avel -=  vpara_a * factor * interaction_strength;
        //    LOG_NAN( pchr_a->phys.avel[kZ] );
        //}

        //if (vimp_b.length_abs() > 0.0f && vpara_b.length_abs() > 0.0f &&
        //    pchr_b->dismount_timer <= 0)
        //{
        //    float imp, vel, factor;

        //    imp = 0.5f * vimp_b.length();
        //    vel = vpara_b.length();

        //    factor = imp / vel;
        //    factor = CLIP( factor, 0.0f, 1.0f );

        //    pchr_b->phys.avel -= vpara_b * factor * interaction_strength;
        //    LOG_NAN( pchr_b->phys.avel[kZ] );
        //}
    }

    if ( bump )
    {
        ai_state_set_bumplast(pchr_a->ai, ichr_b);
        ai_state_set_bumplast(pchr_b->ai, ichr_a);
    }

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool do_chr_prt_collision_get_details( CoNode_t * d, chr_prt_collision_data_t * pdata )
{
    // Get details about the character-particle interaction
    //
    // We already know that the largest particle cv intersects with the a
    // character cv sometime this frame. We need more details to know
    // how to handle the collision.

    bool handled;

    float exponent;
    oct_bb_t cv_chr, cv_prt_max, cv_prt_min;
    oct_vec_v2_t odepth;

    if ( NULL == d || NULL == pdata ) return false;

    // make the object more like a table if there is a platform-like interaction
    exponent = 1;
    if ( SPRITE_SOLID == pdata->pprt->type && pdata->pchr->platform ) exponent += 2;

    // assume the simplest interaction normal
    pdata->nrm = fvec3_t(0, 0, 1);

    // no valid interactions, yet
    handled = false;

    // shift the source bounding boxes to be centered on the given positions
    oct_bb_t::translate(pdata->pchr->chr_min_cv, pdata->pchr->getPosition(), cv_chr);

    // the smallest particle collision volume
    oct_bb_t::translate(pdata->pprt->prt_min_cv, pdata->pprt->getPosition(), cv_prt_min);

    // the largest particle collision volume (the hit-box)
    oct_bb_t::translate(pdata->pprt->prt_max_cv, pdata->pprt->getPosition(), cv_prt_max);

    if ( d->tmin <= 0.0f || std::abs( d->tmin ) > 1e6 || std::abs( d->tmax ) > 1e6 )
    {
        // use "pressure" to determine the normal and overlap
        phys_estimate_pressure_normal(cv_prt_min, cv_chr, exponent, odepth, pdata->nrm, pdata->depth_min);

        handled = true;
        if ( d->tmin <= 0.0f )
        {
            handled = TO_C_BOOL( pdata->depth_min > 0.0f );
        }

        // tag the type of interaction
        pdata->int_min = handled;
        pdata->is_pressure = handled;
    }
    else
    {
        // find the collision volumes at 10% overlap
        oct_bb_t exp1, exp2;

        float tmp_min, tmp_max;

        tmp_min = d->tmin;
        tmp_max = d->tmin + ( d->tmax - d->tmin ) * 0.1f;

        // determine the expanded collision volumes for both objects
        phys_expand_oct_bb(cv_prt_min, pdata->pprt->vel, tmp_min, tmp_max, exp1);
        phys_expand_oct_bb(cv_chr,     pdata->pchr->vel, tmp_min, tmp_max, exp2);

        // use "collision" to determine the normal and overlap
        handled = phys_estimate_collision_normal(exp1, exp2, exponent, odepth, pdata->nrm, pdata->depth_min);

        // tag the type of interaction
        pdata->int_min      = handled;
        pdata->is_collision = handled;
    }

    if ( !handled )
    {
        if ( d->tmin <= 0.0f || std::abs( d->tmin ) > 1e6 || std::abs( d->tmax ) > 1e6 )
        {
            // use "pressure" to determine the normal and overlap
            phys_estimate_pressure_normal(cv_prt_max, cv_chr, exponent, odepth, pdata->nrm, pdata->depth_max);

            handled = true;
            if ( d->tmin <= 0.0f )
            {
                handled = TO_C_BOOL( pdata->depth_max > 0.0f );
            }

            // tag the type of interaction
            pdata->int_max     = handled;
            pdata->is_pressure = handled;
        }
        else
        {
            // find the collision volumes at 10% overlap
            oct_bb_t exp1, exp2;

            float tmp_min, tmp_max;

            tmp_min = d->tmin;
            tmp_max = d->tmin + ( d->tmax - d->tmin ) * 0.1f;

            // determine the expanded collision volumes for both objects
            phys_expand_oct_bb(cv_prt_max, pdata->pprt->vel, tmp_min, tmp_max, exp1);
            phys_expand_oct_bb(cv_chr,     pdata->pchr->vel, tmp_min, tmp_max, exp2);

            // use "collision" to determine the normal and overlap
            handled = phys_estimate_collision_normal(exp1, exp2, exponent, odepth, pdata->nrm, pdata->depth_max);

            // tag the type of interaction
            pdata->int_max      = handled;
            pdata->is_collision = handled;
        }
    }

	return handled;
}

//--------------------------------------------------------------------------------------------
#if 0
bool do_prt_platform_physics( chr_prt_collision_data_t * pdata )
{
    /// @author BB
    /// @details handle the particle interaction with a platform it is not attached "on".
    /// @note gravity is not handled here

    bool plat_collision = false;
    bool z_collide, was_z_collide;

    if ( NULL == pdata ) return false;

    // is the platform a platform?
    if ( !pdata->pchr->platform ) return false;

    // can the particle interact with it?
    if ( _currentModule->getObjectHandler().exists( pdata->pprt->attachedto_ref ) ) return false;

    // this is handled elsewhere
    if ( GET_INDEX_PCHR( pdata->pchr ) == pdata->pprt->onwhichplatform_ref ) return false;

    // Test to see whether the particle is in the right position to interact with the platform.
    // You have to be closer to a platform to interact with it then for a general object,
    // but the vertical distance is looser.
    plat_collision = test_interaction_close_1(pdata->pchr->chr_max_cv, pdata->pchr->getPosition(), pdata->pprt->bump_padded, pdata->pprt->getPosition(), true );

    if ( !plat_collision ) return false;

    // the only way to get to this point is if the two objects don't collide
    // but they are within the PLATTOLERANCE of each other in the z direction
    // it is a valid platform. now figure out the physics

    // are they colliding for the first time?
    z_collide     = TO_C_BOOL(( pdata->pprt->pos[kZ] < pdata->pchr->getPosZ() + pdata->pchr->chr_max_cv._maxs[OCT_Z] ) && ( pdata->pprt->pos[kZ] > pdata->pchr->getPosZ() + pdata->pchr->chr_max_cv._mins[OCT_Z] ) );
    was_z_collide = TO_C_BOOL(( pdata->pprt->pos[kZ] - pdata->pprt->vel[kZ] < pdata->pchr->getPosZ() + pdata->pchr->chr_max_cv._maxs[OCT_Z] - pdata->pchr->vel[kZ] ) && ( pdata->pprt->pos[kZ] - pdata->pprt->vel[kZ]  > pdata->pchr->getPosZ() + pdata->pchr->chr_max_cv._mins[OCT_Z] ) );

    if ( z_collide && !was_z_collide )
    {
        // Particle is falling onto the platform
        phys_data_sum_aplat_index( &( pdata->pprt->phys ), pdata->pchr->getPosZ() + pdata->pchr->chr_max_cv._maxs[OCT_Z] - pdata->pprt->pos[kZ], kZ );
        phys_data_sum_avel_index( &( pdata->pprt->phys ), ( pdata->pchr->getPosZ() - pdata->pprt->vel[kZ] ) *( 1.0f + pdata->ppip->dampen ), kZ );

        // This should prevent raindrops from stacking up on the top of trees and other
        // objects
        if ( pdata->ppip->end_ground && pdata->pchr->platform )
        {
            pdata->terminate_particle = true;
        }

        plat_collision = true;
    }
    else if ( z_collide && was_z_collide )
    {
        // colliding this time and last time. particle is *embedded* in the platform
        phys_data_sum_aplat_index( &( pdata->pprt->phys ), pdata->pchr->getPosZ() + pdata->pchr->chr_max_cv._maxs[OCT_Z] - pdata->pprt->pos[kZ], kZ );

        if ( pdata->pprt->vel[kZ] - pdata->pchr->vel[kZ] < 0 )
        {
            phys_data_sum_avel_index( &( pdata->pprt->phys ), pdata->pchr->vel[kZ] * pdata->ppip->dampen + PLATFORM_STICKINESS * pdata->pchr->vel[kZ] - pdata->pprt->vel[kZ], kZ );
        }
        else
        {
            phys_data_sum_avel_index( &( pdata->pprt->phys ), pdata->pprt->vel[kZ] *( 1.0f - PLATFORM_STICKINESS ) + pdata->pchr->vel[kZ] * PLATFORM_STICKINESS - pdata->pprt->vel[kZ], kZ );
        }
        phys_data_sum_avel_index( &( pdata->pprt->phys ), pdata->pprt->vel[kX] *( 1.0f - PLATFORM_STICKINESS ) + pdata->pchr->vel[kX] * PLATFORM_STICKINESS - pdata->pprt->vel[kX], kX );
        phys_data_sum_avel_index( &( pdata->pprt->phys ), pdata->pprt->vel[kY] *( 1.0f - PLATFORM_STICKINESS ) + pdata->pchr->vel[kY] * PLATFORM_STICKINESS - pdata->pprt->vel[kY], kY );

        plat_collision = true;
    }
    else
    {
        // not colliding this time or last time. particle is just near the platform
        float lerp_z = ( pdata->pprt->pos[kZ] - ( pdata->pchr->getPosZ() + pdata->pchr->chr_max_cv._maxs[OCT_Z] ) ) / PLATTOLERANCE;
        lerp_z = CLIP( lerp_z, -1.0f, +1.0f );

        if ( lerp_z > 0.0f )
        {
            phys_data_sum_avel_index( &( pdata->pprt->phys ), ( pdata->pchr->vel[kX] - pdata->pprt->vel[kX] ) * PLATFORM_STICKINESS * lerp_z, kX );
            phys_data_sum_avel_index( &( pdata->pprt->phys ), ( pdata->pchr->vel[kY] - pdata->pprt->vel[kY] ) * PLATFORM_STICKINESS * lerp_z, kY );
            phys_data_sum_avel_index( &( pdata->pprt->phys ), ( pdata->pchr->vel[kZ] - pdata->pprt->vel[kZ] ) * PLATFORM_STICKINESS * lerp_z, kZ );

            plat_collision = true;
        }
    }

    return plat_collision;
}
#endif

//--------------------------------------------------------------------------------------------
bool do_chr_prt_collision_deflect( chr_prt_collision_data_t * pdata )
{
    bool prt_deflected = false;

    bool chr_is_invictus, chr_can_deflect;
    bool prt_wants_deflection;
    FACING_T direction;

    if ( NULL == pdata ) return false;

    /// @note ZF@> Simply ignore characters with invictus for now, it causes some strange effects
    if ( pdata->pchr->invictus ) return true;

    // find the "attack direction" of the particle
    direction = vec_to_facing( pdata->pchr->getPosX() - pdata->pprt->pos[kX], pdata->pchr->getPosY() - pdata->pprt->pos[kY] );
    direction = pdata->pchr->ori.facing_z - direction + ATK_BEHIND;

    // shield block?
    chr_is_invictus = is_invictus_direction( direction, GET_INDEX_PCHR( pdata->pchr ), pdata->ppip->damfx );

    // determine whether the character is magically protected from missile attacks
    prt_wants_deflection  = TO_C_BOOL(( MISSILE_NORMAL != pdata->pchr->missiletreatment ) &&
                                      ( pdata->pprt->owner_ref != GET_INDEX_PCHR( pdata->pchr ) ) && !pdata->ppip->bump_money );

    chr_can_deflect = TO_C_BOOL(( 0 != pdata->pchr->damage_timer ) && ( pdata->max_damage > 0 ) );

    // try to deflect the particle
    prt_deflected = false;
    pdata->mana_paid = false;
    if ( chr_is_invictus || ( prt_wants_deflection && chr_can_deflect ) )
    {
        // magically deflect the particle or make a ricochet if the character is invictus
        int treatment;

        treatment     = MISSILE_DEFLECT;
        prt_deflected = true;
        if ( prt_wants_deflection )
        {
            treatment = pdata->pchr->missiletreatment;
            const std::shared_ptr<Object> &missileHandler = _currentModule->getObjectHandler()[pdata->pchr->missilehandler];
            if(missileHandler) {
                pdata->mana_paid = missileHandler->costMana(pdata->pchr->missilecost << 8, pdata->pprt->owner_ref);
            }
            prt_deflected = pdata->mana_paid;
        }

        if ( prt_deflected )
        {
            // Treat the missile
            if ( treatment == MISSILE_DEFLECT )
            {
                // Deflect the incoming ray off the normal
                pdata->pprt->phys.avel -= pdata->vdiff_para * 2.0f;

                // the ricochet is not guided
                pdata->pprt->setHoming(false);
            }
            else if ( treatment == MISSILE_REFLECT )
            {
                // Reflect it back in the direction it came
                pdata->pprt->phys.avel -= pdata->vdiff * 2.0f;

                // Change the owner of the missile
                pdata->pprt->team       = pdata->pchr->team;
                pdata->pprt->owner_ref  = pdata->pchr->getCharacterID();
            }

            // Blocked!
            if(0 == pdata->pchr->damage_timer) 
            {
                spawn_defense_ping( pdata->pchr, pdata->pprt->owner_ref );

                // Initialize for the billboard
                const float lifetime = 3;
                const auto text_color = Ego::Math::Colour4f::white();
                const auto tint = Ego::Math::Colour4f(getBlockActionColour(),1);                
                chr_make_text_billboard( pdata->pchr->getCharacterID(), "Blocked!", text_color, tint, lifetime, Billboard::Flags::All );                
            }

            // If the attack was blocked by a shield, then check if the block caused a knockback
            if ( chr_is_invictus && ACTION_IS_TYPE( pdata->pchr->inst.action_which, P ) )
            {
                bool using_shield;
                CHR_REF item;

                // Figure out if we are really using a shield or if it is just a invictus frame
                using_shield = false;
                item         = INVALID_CHR_REF;

                // Check right hand for a shield
                if ( !using_shield )
                {
                    item = pdata->pchr->holdingwhich[SLOT_RIGHT];
                    if ( _currentModule->getObjectHandler().exists( item ) && pdata->pchr->ai.lastitemused == item )
                    {
                        using_shield = true;
                    }
                }

                // Check left hand for a shield
                if ( !using_shield )
                {
                    item = pdata->pchr->holdingwhich[SLOT_LEFT];
                    if ( _currentModule->getObjectHandler().exists( item ) && pdata->pchr->ai.lastitemused == item )
                    {
                        using_shield = true;
                    }
                }

                // Now we have the block rating and know the enemy
                if ( _currentModule->getObjectHandler().exists( pdata->pprt->owner_ref ) && using_shield )
                {
                    int   total_block_rating;

                    Object *pshield   = _currentModule->getObjectHandler().get( item );
                    Object *pattacker = _currentModule->getObjectHandler().get( pdata->pprt->owner_ref );

                    // use the character block skill plus the base block rating of the shield and adjust for strength
                    total_block_rating = pshield->getProfile()->getBaseBlockRating();

                    //Defender Perk gives +100% Block Rating
                    if(pdata->pchr->hasPerk(Ego::Perks::DEFENDER)) {
                        total_block_rating += 100;
                    }

                    // -4% per attacker strength
                    total_block_rating -= 4 * pattacker->getAttribute(Ego::Attribute::MIGHT);

                    // +2% per defender strength
                    total_block_rating += 2 * pdata->pchr->getAttribute(Ego::Attribute::MIGHT);

                    // Now determine the result of the block
                    if ( Random::getPercent() <= total_block_rating )
                    {
                        // Defender won, the block holds
                        // Add a small stun to the attacker = 40/50 (0.8 seconds)
                        pattacker->reload_timer += 40;
                    }
                    else
                    {
                        // Attacker broke the block and batters away the shield
                        // Time to raise shield again = 40/50 (0.8 seconds)
                        pdata->pchr->reload_timer += 40;
                        AudioSystem::get().playSound(pdata->pchr->getPosition(), AudioSystem::get().getGlobalSound(GSND_SHIELDBLOCK));
                    }
                }
            }

        }
    }

    return prt_deflected;
}

//--------------------------------------------------------------------------------------------
#if 0
bool do_chr_prt_collision_recoil( chr_prt_collision_data_t * pdata )
{
    /// @author BB
    /// @details make the character and particle recoil from the collision

    float chr_mass = 0.0f, prt_mass;
    float chr_recoil, prt_recoil;

    float attack_factor;

    if ( !pdata->ppip->allowpush ) return false;

    // do the reaction force of the particle on the character

    // determine how much the attack is "felt"
    attack_factor = 1.0f;
    if ( DAMAGE_CRUSH == pdata->pprt->damagetype )
    {
        // very blunt type of attack, the maximum effect
        attack_factor = 1.0f;
    }
    else if ( DAMAGE_POKE == pdata->pprt->damagetype )
    {
        // very focussed type of attack, the minimum effect
        attack_factor = 0.5f;
    }
    else
    {
        // all other damage types are in the middle
        attack_factor = Ego::Math::invSqrtTwo<float>();
    }

    // get some type of mass info for the particle
    get_chr_mass( pdata->pchr, &chr_mass );
    get_prt_mass( pdata->pprt, pdata->pchr, &prt_mass );

    // get recoil factors for the masses
    get_recoil_factors( chr_mass, prt_mass, &chr_recoil, &prt_recoil );

    // now, we have the particle's impulse and mass
    // Do the impulse to the object that was hit
    // If the particle was magically deflected, there is no rebound on the target
    if ( !pdata->mana_paid )
    {
        fvec3_t tmp_impulse;

        // calculate the "impulse" to the character
        tmp_impulse = pdata->vimpulse * -(chr_recoil * attack_factor * pdata->block_factor);
        phys_data_sum_avel(&(pdata->pchr->phys), tmp_impulse);

        tmp_impulse = pdata->pimpulse * -(chr_recoil * attack_factor * pdata->block_factor);
        phys_data_sum_acoll(&(pdata->pchr->phys), tmp_impulse);
    }

    // if the particle is attached to a weapon, the particle can force the
    // weapon (actually, the weapon's holder) to rebound.
    if ( _currentModule->getObjectHandler().exists( pdata->pprt->attachedto_ref ) )
    {
        // get the attached mass
        Object *pattached = _currentModule->getObjectHandler().get( pdata->pprt->attachedto_ref );

        // assume the worst
        Object *pholder = NULL;

        // who is holding the weapon?
        CHR_REF iholder = chr_get_lowest_attachment(pdata->pprt->attachedto_ref, false);
        if ( _currentModule->getObjectHandler().exists( iholder ) )
        {
            pholder = _currentModule->getObjectHandler().get( iholder );
        }
        else
        {
            iholder = chr_get_lowest_attachment( pdata->pprt->owner_ref, false );
            if ( _currentModule->getObjectHandler().exists( iholder ) )
            {
                pholder = _currentModule->getObjectHandler().get( iholder );
            }
        }

        {
            float holder_mass = 0.0f;
            if (( NULL != pholder ) && ( iholder != pdata->pprt->attachedto_ref ) )
            {
                get_chr_mass( pholder, &holder_mass );
            }

            float attached_mass = 0.0f;
            get_chr_mass( pattached, &attached_mass );

            float total_mass = std::abs( holder_mass ) + std::abs( attached_mass );
            if ( holder_mass < 0.0f ||  attached_mass < 0.0f )
            {
                total_mass = -total_mass;
            }

            float tmp_holder_recoil, tmp_prt_recoil;
            get_recoil_factors( total_mass, prt_mass, &tmp_holder_recoil, &tmp_prt_recoil );

            // get the actual holder recoil
            float holder_recoil = tmp_holder_recoil * attack_factor;

            fvec3_t tmp_impulse;
            // in the SAME direction as the particle
			tmp_impulse = pdata->vimpulse * holder_recoil;
            phys_data_sum_avel(&(pholder->phys), tmp_impulse);

			tmp_impulse = pdata->pimpulse * holder_recoil;
            phys_data_sum_acoll(&(pholder->phys), tmp_impulse);
        }
    }

    // apply the impulse to the particle velocity
    if ( INVALID_CHR_REF == pdata->pprt->attachedto_ref )
    {
        fvec3_t tmp_impulse;

        tmp_impulse = pdata->vimpulse * prt_recoil;
        phys_data_sum_avel(&(pdata->pprt->phys), tmp_impulse);

		tmp_impulse = pdata->pimpulse * prt_recoil;
        phys_data_sum_acoll(&(pdata->pprt->phys), tmp_impulse);
    }

    return true;
}
#endif

//--------------------------------------------------------------------------------------------
bool do_chr_prt_collision_damage( chr_prt_collision_data_t * pdata )
{
    ENC_REF ienc_now, ienc_nxt;
    size_t  ienc_count;

    bool prt_needs_impact;

    Object * powner = NULL;

    if ( NULL == pdata ) return false;

    if ( _currentModule->getObjectHandler().exists( pdata->pprt->owner_ref ) )
    {
        powner = _currentModule->getObjectHandler().get( pdata->pprt->owner_ref );
    }

    //Get the Profile of the Object that spawned this particle (i.e the weapon itself, not the holder)
    const std::shared_ptr<ObjectProfile> &spawnerProfile = ProfileSystem::get().getProfile(pdata->pprt->getSpawnerProfile());

    // clean up the enchant list before doing anything
    cleanup_character_enchants( pdata->pchr );

    // Check all enchants to see if they are removed
    ienc_now = pdata->pchr->firstenchant;
    ienc_count = 0;
    while ( VALID_ENC_RANGE( ienc_now ) && ( ienc_count < ENCHANTS_MAX ) )
    {
        ienc_nxt = EnchantHandler::get().get_ptr(ienc_now)->nextenchant_ref;

        if ( enc_is_removed( ienc_now, pdata->pprt->getSpawnerProfile() ) )
        {
            remove_enchant( ienc_now, NULL );
        }

        ienc_now = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= ENCHANTS_MAX ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

    // Steal some life.
    if ( pdata->pprt->lifedrain > 0 && pdata->pchr->life > 0)
    {
		// As pdata->pchr->life > 0, we can safely cast to unsigned.
		UFP8_T life = (UFP8_T)pdata->pchr->life;

		// Drain as much as allowed and possible.
		UFP8_T drain = std::min(life, pdata->pprt->lifedrain);

		// Remove the drain from the character that was hit ...
		pdata->pchr->life = Ego::Math::constrain(pdata->pchr->life - drain, static_cast<UFP8_T>(0), FLOAT_TO_FP8(pdata->pchr->getAttribute(Ego::Attribute::MAX_LIFE)));

		// ... and add it to the "caster".
		if ( NULL != powner )
		{
			powner->life = Ego::Math::constrain(powner->life + drain, static_cast<UFP8_T>(0), FLOAT_TO_FP8(powner->getAttribute(Ego::Attribute::MAX_LIFE)));
		}
    }

    // Steal some mana.
    if ( pdata->pprt->manadrain > 0 && pdata->pchr->mana > 0)
    {
		// As pdata->pchr->mana > 0, we can safely cast to unsigned.
		UFP8_T mana = (UFP8_T)pdata->pchr->mana;

		// Drain as much as allowed and possible.
		UFP8_T drain = std::min(mana, pdata->pprt->manadrain);

        // Remove the drain from the character that was hit ...
        pdata->pchr->mana = Ego::Math::constrain<uint32_t>(pdata->pchr->mana - drain, 0, FLOAT_TO_FP8(pdata->pchr->getAttribute(Ego::Attribute::MAX_MANA)));

        // add it to the "caster"
        if ( NULL != powner )
        {
            powner->mana = Ego::Math::constrain<uint32_t>(powner->mana + drain, 0, FLOAT_TO_FP8(powner->getAttribute(Ego::Attribute::MAX_MANA)));
        }
    }

    // Do grog
    if (pdata->ppip->grogTime > 0 && pdata->pchr->getProfile()->canBeGrogged())
    {
        SET_BIT( pdata->pchr->ai.alert, ALERTIF_CONFUSED );
        pdata->pchr->grog_timer = std::max(static_cast<unsigned>(pdata->pchr->grog_timer), pdata->ppip->grogTime );

        chr_make_text_billboard(powner->getCharacterID(), "Groggy!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::green(), 3, Billboard::Flags::All);
    }

    // Do daze
    if (pdata->ppip->dazeTime > 0 && pdata->pchr->getProfile()->canBeDazed())
    {
        SET_BIT( pdata->pchr->ai.alert, ALERTIF_CONFUSED );
        pdata->pchr->daze_timer = std::max(static_cast<unsigned>(pdata->pchr->daze_timer), pdata->ppip->dazeTime );

        chr_make_text_billboard(powner->getCharacterID(), "Dazed!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::yellow(), 3, Billboard::Flags::All);
    }

    //---- Damage the character, if necessary
    if ( 0 != std::abs( pdata->pprt->damage.base ) + std::abs( pdata->pprt->damage.rand ) )
    {
        prt_needs_impact = TO_C_BOOL( pdata->ppip->rotatetoface || pdata->pprt->isAttached() );

        if(spawnerProfile != nullptr) {
            if ( spawnerProfile->isRangedWeapon() ) prt_needs_impact = true;            
        }

        // DAMFX_ARRO means that it only does damage to the one it's attached to
        if ( HAS_NO_BITS(pdata->ppip->damfx, DAMFX_ARRO) /*&& (!prt_needs_impact || pdata->is_impact)*/ )
        {
            //Damage adjusted for attributes and weaknesses
            IPair modifiedDamage = pdata->pprt->damage;

            FACING_T direction = vec_to_facing( pdata->pprt->vel[kX] , pdata->pprt->vel[kY] );
            direction = pdata->pchr->ori.facing_z - direction + ATK_BEHIND;

            // These things only apply if the particle has an owner
            if ( nullptr != powner )
            {
                //Check special perk effects
                if(spawnerProfile != nullptr)
                {                
                    // Check Crack Shot perk which applies 3 second Daze with fireweapons
                    if(pdata->pchr->getProfile()->canBeDazed() && powner->hasPerk(Ego::Perks::CRACKSHOT) && DamageType_isPhysical(pdata->pprt->damagetype))
                    {
                        //Is the particle spawned by a gun?
                        if(spawnerProfile->isRangedWeapon() && spawnerProfile->getIDSZ(IDSZ_SKILL) == MAKE_IDSZ('T','E','C','H')) {
                            SET_BIT( pdata->pchr->ai.alert, ALERTIF_CONFUSED );
                            pdata->pchr->daze_timer += 3;

                            chr_make_text_billboard(powner->getCharacterID(), "Crackshot!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::blue(), 3, Billboard::Flags::All);
                        }
                    }

                    //Brutal Strike has chance to inflict 2 second Grog with melee CRUSH attacks
                    if(pdata->pchr->getProfile()->canBeGrogged() && powner->hasPerk(Ego::Perks::BRUTAL_STRIKE) && spawnerProfile->isMeleeWeapon() && pdata->pprt->damagetype == DAMAGE_CRUSH) {
                        SET_BIT( pdata->pchr->ai.alert, ALERTIF_CONFUSED );
                        pdata->pchr->grog_timer += 2;

                        chr_make_text_billboard(powner->getCharacterID(), "Brutal Strike!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::red(), 3, Billboard::Flags::All);
                        AudioSystem::get().playSound(powner->getPosition(), AudioSystem::get().getGlobalSound(GSND_CRITICAL_HIT));
                    }
                }

                // Apply intellect bonus damage for particles with the [IDAM] expansions (Low ability gives penality)
                // +2% bonus for every point of intellect. Below 14 gives -2% instead!
                if ( pdata->ppip->_intellectDamageBonus )
                {
                    float percent = ( powner->getAttribute(Ego::Attribute::INTELLECT) - 14.0f ) * 2.0f;

                    //Sorcery Perk increases spell damage by 10%
                    if(powner->hasPerk(Ego::Perks::SORCERY)) {
                        percent += 10.0f;
                    }

                    //Dark Arts Master perk gives evil damage +20%
                    if(pdata->pprt->damagetype == DAMAGE_EVIL && powner->hasPerk(Ego::Perks::DARK_ARTS_MASTERY)) {
                        percent += 20.0f;
                    }

                    percent /= 100.0f;
                    modifiedDamage.base *= 1.00f + percent;
                    modifiedDamage.rand *= 1.00f + percent;

                    //Disintegrate perk deals +100 ZAP damage at 0.025% chance per Intellect!
                    if(pdata->pprt->damagetype == DAMAGE_ZAP && powner->hasPerk(Ego::Perks::DISINTEGRATE)) {
                        if(Random::nextFloat()*100.0f <= powner->getAttribute(Ego::Attribute::INTELLECT) * 0.025f) {
                            modifiedDamage.base += FLOAT_TO_FP8(100.0f);
                            chr_make_text_billboard(pdata->pchr->getCharacterID(), "Disintegrated!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::purple(), 6, Billboard::Flags::All);

                            //Disintegrate effect
                            ParticleHandler::get().spawnGlobalParticle(pdata->pchr->getPosition(), ATK_FRONT, LocalParticleProfileRef(PIP_DISINTEGRATE_START), 0);
                        }
                    }
                }

                // Notify the attacker of a scored hit
                SET_BIT(powner->ai.alert, ALERTIF_SCOREDAHIT);
                powner->ai.hitlast = pdata->pchr->getCharacterID();

                // Tell the weapons who the attacker hit last
                bool meleeAttack = false;
                const std::shared_ptr<Object> &leftHanditem = powner->getRightHandItem();
                if (leftHanditem)
                {
                    leftHanditem->ai.hitlast = pdata->pchr->getCharacterID();
                    if (powner->ai.lastitemused == leftHanditem->getCharacterID()) {
                        SET_BIT(leftHanditem->ai.alert, ALERTIF_SCOREDAHIT);  
                        if(leftHanditem->getProfile()->getIDSZ(IDSZ_SPECIAL) == MAKE_IDSZ('X', 'W', 'E', 'P') && !leftHanditem->getProfile()->isRangedWeapon()) {
                            meleeAttack = true;
                        }
                    } 
                }

                const std::shared_ptr<Object> &rightHandItem = powner->getRightHandItem();
                if (rightHandItem)
                {
                    rightHandItem->ai.hitlast = pdata->pchr->getCharacterID();
                    if (powner->ai.lastitemused == rightHandItem->getCharacterID()) {
                        SET_BIT(rightHandItem->ai.alert, ALERTIF_SCOREDAHIT);  
                        if(rightHandItem->getProfile()->getIDSZ(IDSZ_SPECIAL) == MAKE_IDSZ('X', 'W', 'E', 'P') && !rightHandItem->getProfile()->isRangedWeapon()) {
                            meleeAttack = true;
                        }
                    } 
                }

                //Unarmed attack?
                if (powner->ai.lastitemused == powner->getCharacterID()) {
                    meleeAttack = true;
                }

                //Grim Reaper (5% chance to trigger +50 EVIL damage)
                if(spawnerProfile != nullptr && powner->hasPerk(Ego::Perks::GRIM_REAPER)) {

                    //Is it a Scythe?
                    if(spawnerProfile->getIDSZ(IDSZ_TYPE) == MAKE_IDSZ('S','C','Y','T') && Random::getPercent() <= 5) {

                        //Make sure they can be damaged by EVIL first
                        if(pdata->pchr->damage_modifier[DAMAGE_EVIL] == NONE) {
                            IPair grimReaperDamage;
                            grimReaperDamage.base = FLOAT_TO_FP8(50.0f);
                            grimReaperDamage.rand = 0.0f;
                            pdata->pchr->damage(direction, grimReaperDamage, DAMAGE_EVIL, pdata->pprt->team, _currentModule->getObjectHandler()[pdata->pprt->owner_ref], DAMFX_TIME, false);
                            chr_make_text_billboard(powner->getCharacterID(), "Grim Reaper!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::red(), 3, Billboard::Flags::All);                                
                            AudioSystem::get().playSound(powner->getPosition(), AudioSystem::get().getGlobalSound(GSND_CRITICAL_HIT));
                        }
                    }
                }                

                //Deadly Strike perk (1% chance per character level to trigger vs non undead)
                if(pdata->pchr->getProfile()->getIDSZ(IDSZ_PARENT) != MAKE_IDSZ('U','N','D','E'))
                {
                    if(powner->hasPerk(Ego::Perks::DEADLY_STRIKE) && powner->getExperienceLevel() >= Random::getPercent() && DamageType_isPhysical(pdata->pprt->damagetype)){
                        //Gain +0.25 damage per Agility
                        modifiedDamage.base += FLOAT_TO_FP8(powner->getAttribute(Ego::Attribute::AGILITY) * 0.25f);
                        chr_make_text_billboard(powner->getCharacterID(), "Deadly Strike", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::blue(), 3, Billboard::Flags::All);
                        AudioSystem::get().playSound(powner->getPosition(), AudioSystem::get().getGlobalSound(GSND_CRITICAL_HIT));
                    }
                }
            }

            // handle vulnerabilities, double the damage
            if ( chr_has_vulnie(pdata->pchr->getCharacterID(), pdata->pprt->getSpawnerProfile()) )
            {
                // Double the damage
                modifiedDamage.base = ( modifiedDamage.base << 1 );
                modifiedDamage.rand = ( modifiedDamage.rand << 1 ) | 1;

                SET_BIT( pdata->pchr->ai.alert, ALERTIF_HITVULNERABLE );

                // Initialize for the billboard
                chr_make_text_billboard(pdata->pchr->getCharacterID(), "Super Effective!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::yellow(), 3, Billboard::Flags::All);
            }

            //Is it a critical hit?
            if(powner && powner->hasPerk(Ego::Perks::CRITICAL_HIT) && DamageType_isPhysical(pdata->pprt->damagetype)) {
                //0.5% chance per agility to deal max damage
                float critChance = powner->getAttribute(Ego::Attribute::AGILITY)*0.5f;

                //Lucky Perk increases critical hit chance by 10%!
                if(powner->hasPerk(Ego::Perks::LUCKY)) {
                    critChance += 10.0f;
                }

                if(Random::getPercent() <= critChance) {
                    modifiedDamage.base += modifiedDamage.rand;
                    modifiedDamage.rand = 0;
                    chr_make_text_billboard(powner->getCharacterID(), "Critical Hit!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::red(), 3, Billboard::Flags::All);
                    AudioSystem::get().playSound(powner->getPosition(), AudioSystem::get().getGlobalSound(GSND_CRITICAL_HIT));
                }
            }

            // Damage the character
            pdata->actual_damage = pdata->pchr->damage(direction, modifiedDamage, pdata->pprt->damagetype, 
                pdata->pprt->team, _currentModule->getObjectHandler()[pdata->pprt->owner_ref], pdata->ppip->damfx, false);
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
#if 0
bool do_chr_prt_collision_impulse( chr_prt_collision_data_t * pdata )
{
    // estimate the impulse on the particle

    bool did_something = false;

    if ( NULL == pdata ) return false;

    if ( !pdata->ppip->allowpush ) return false;

    // the impulse due to particle damage
    if ( pdata->is_impact && pdata->prt_damages_chr )
    {
        int left_over_damage;

        did_something = true;

        left_over_damage = 0;
        if ( std::abs( pdata->actual_damage ) < std::abs( pdata->max_damage ) )
        {
            left_over_damage = std::abs( pdata->max_damage ) - std::abs( pdata->actual_damage );
        }

        if ( 0 == pdata->max_damage )
        {
            pdata->block_factor = 0.0f;
        }
        else
        {
            pdata->block_factor = static_cast<float>(left_over_damage)
                                / static_cast<float>(std::abs(pdata->max_damage));
            pdata->block_factor = pdata->block_factor / ( 1.0f + pdata->block_factor );
        }

        if ( 0.0f == pdata->block_factor )
        {
            // the simple case (particle comes to a stop)
            pdata->vimpulse -= pdata->pprt->vel;
        }
        else if ( 0.0f != pdata->dot )
        {
            float sgn = SGN( pdata->dot );

            pdata->vimpulse += pdata->vdiff_perp * (-sgn * (1.0f + pdata->block_factor));
        }
    }

    // the "pressure" impulse due to overlap
    if ( pdata->int_min && pdata->depth_min > 0.0f && pdata->ichr != pdata->pprt->owner_ref )
    {
        // is the normal reversed?
		fvec3_t tmp_imp = pdata->nrm * pdata->depth_min;
        pdata->pimpulse += tmp_imp;

        did_something = true;
    }

    return true;
}
#endif

//--------------------------------------------------------------------------------------------
bool do_chr_prt_collision_bump( chr_prt_collision_data_t * pdata )
{
    bool prt_belongs_to_chr;
    bool prt_hates_chr, prt_attacks_chr, prt_hateonly;
    bool valid_onlydamagefriendly;
    bool valid_friendlyfire;
    bool valid_onlydamagehate;

    if ( NULL == pdata ) return false;

    const float maxDamage = std::abs(pdata->pprt->damage.base) + std::abs(pdata->pprt->damage.rand);

    // always allow valid reaffirmation
    if (( pdata->pchr->reaffirm_damagetype < DAMAGE_COUNT ) &&
        ( pdata->pprt->damagetype < DAMAGE_COUNT ) &&
        ( pdata->pchr->reaffirm_damagetype == pdata->pprt->damagetype ) &&
        ( maxDamage > 0) )
    {
        return true;
    }

    // if the particle was deflected, then it can't bump the character
    if ( pdata->pchr->isInvincible() || pdata->pprt->getAttachedObject().get() == pdata->pchr ) return false;

    //Only allow one collision per particle unless that particle is eternal
    if(!pdata->pprt->isEternal() && pdata->pprt->hasCollided(_currentModule->getObjectHandler()[pdata->pchr->getCharacterID()])) {
        return false;
    }

	prt_belongs_to_chr = TO_C_BOOL(pdata->pchr->getCharacterID() == pdata->pprt->owner_ref);

    if ( !prt_belongs_to_chr )
    {
        // no simple owner relationship. Check for something deeper.
        CHR_REF prt_owner = prt_get_iowner( pdata->pprt->getParticleID(), 0 );
        if ( _currentModule->getObjectHandler().exists( prt_owner ) )
        {
            CHR_REF chr_wielder = chr_get_lowest_attachment( pdata->pchr->getCharacterID(), true );
            CHR_REF prt_wielder = chr_get_lowest_attachment( prt_owner, true );

            if ( !_currentModule->getObjectHandler().exists( chr_wielder ) ) chr_wielder = pdata->pchr->getCharacterID();
            if ( !_currentModule->getObjectHandler().exists( prt_wielder ) ) prt_wielder = prt_owner;

			prt_belongs_to_chr = TO_C_BOOL(chr_wielder == prt_wielder);
        }
    }

    // does the particle team hate the character's team
    prt_hates_chr = team_hates_team( pdata->pprt->team, pdata->pchr->team );

    // Only bump into hated characters?
    prt_hateonly = pdata->pprt->getProfile()->hateonly;
    valid_onlydamagehate = TO_C_BOOL( prt_hates_chr && pdata->pprt->getProfile()->hateonly );

    // allow neutral particles to attack anything
	prt_attacks_chr = false;
    if(prt_hates_chr || ((Team::TEAM_NULL != pdata->pchr->team) && (Team::TEAM_NULL == pdata->pprt->team)) ) {
        prt_attacks_chr = (maxDamage > 0);
    }

    // this is the onlydamagefriendly condition from the particle search code
    valid_onlydamagefriendly = TO_C_BOOL(( pdata->ppip->onlydamagefriendly && pdata->pprt->team == pdata->pchr->team ) ||
                                         ( !pdata->ppip->onlydamagefriendly && prt_attacks_chr ) );

    // I guess "friendly fire" does not mean "self fire", which is a bit unfortunate.
    valid_friendlyfire = TO_C_BOOL(( pdata->ppip->friendlyfire && !prt_hates_chr && !prt_belongs_to_chr ) ||
                                   ( !pdata->ppip->friendlyfire && prt_attacks_chr ) );

    pdata->prt_bumps_chr =  TO_C_BOOL( valid_friendlyfire || valid_onlydamagefriendly || valid_onlydamagehate );

    return pdata->prt_bumps_chr;
}

//--------------------------------------------------------------------------------------------
bool do_chr_prt_collision_handle_bump( chr_prt_collision_data_t * pdata )
{
    if ( NULL == pdata || !pdata->prt_bumps_chr ) return false;

    if ( !pdata->prt_bumps_chr ) return false;

    // Catch on fire
    spawn_bump_particles( pdata->pchr->getCharacterID(), pdata->pprt->getParticleID() );

    // handle some special particle interactions
    if ( pdata->pprt->getProfile()->end_bump )
    {
        if (pdata->pprt->getProfile()->bump_money)
        {
            Object * pcollector = pdata->pchr;

            // Let mounts collect money for their riders
            if (pdata->pchr->isMount())
            {
                // if the mount's rider can't get money, the mount gets to keep the money!
                const std::shared_ptr<Object> &rider = pdata->pchr->getLeftHandItem();
                if (rider != nullptr && rider->cangrabmoney) {
                    pcollector = rider.get();
                }
            }

            if ( pcollector->cangrabmoney && pcollector->isAlive() && 0 == pcollector->damage_timer && pcollector->money < MAXMONEY )
            {
                pcollector->money += pdata->pprt->getProfile()->bump_money;
                pcollector->money = Ego::Math::constrain<int>(pcollector->money, 0, MAXMONEY);

                // the coin disappears when you pick it up
                pdata->terminate_particle = true;
            }
        }
        else
        {
            // Only hit one character, not several
            pdata->terminate_particle = true;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool do_chr_prt_collision_init( const CHR_REF ichr, const PRT_REF iprt, chr_prt_collision_data_t * pdata )
{
    if ( NULL == pdata ) return false;

    BLANK_STRUCT_PTR(pdata);

    if ( !ParticleHandler::get()[iprt] ) return false;
    pdata->iprt = iprt;
    pdata->pprt = ParticleHandler::get()[iprt];

    // make sure that it is on
    if ( !_currentModule->getObjectHandler().exists( ichr ) ) return false;
    pdata->ichr = ichr;
    pdata->pchr = _currentModule->getObjectHandler().get( ichr );

    pdata->ppip = pdata->pprt->getProfile();

    // estimate the maximum possible "damage" from this particle
    // other effects can magnify this number, like vulnerabilities
    // or DAMFX_* bits
    pdata->max_damage = std::abs( pdata->pprt->damage.base ) + std::abs( pdata->pprt->damage.rand );

    return true;
}

void do_chr_prt_collision_knockback(chr_prt_collision_data_t &pdata)
{
    /**
    * @brief
    *   ZF> New particle collision knockback algorithm (07.08.2015)
    **/

    //No knocback applicable?
    if(pdata.pprt->vel.length_abs() == 0.0f) {
        return;
    }

    //Target immune to knockback?
    if(pdata.pchr->phys.bumpdampen == 0.0f || CHR_INFINITE_WEIGHT == pdata.pchr->phys.weight) {
        return;
    }

    //Is particle allowed to cause knockback?
    if(!pdata.ppip->allowpush) {
        return;
    }

    //If the particle was magically deflected, then there is no knockback
    if (pdata.mana_paid) {
        return;
    }

    float knockbackFactor = 1.0f;

    //If we are attached to a Object then the attacker's Might can increase knockback
    std::shared_ptr<Object> attacker = pdata.pprt->getAttachedObject();
    if (attacker)
    {
        //If we are actually a weapon, use the weapon holder's strength
        if(attacker->isBeingHeld()) {
            attacker = _currentModule->getObjectHandler()[attacker->attachedto];
        }

        const float attackerMight = attacker->getAttribute(Ego::Attribute::MIGHT) - 10.0f;

        //Add 2% knockback per point of Might above 10
        if(attackerMight >= 0.0f) {
            knockbackFactor += attackerMight * 0.02f;
        }

        //Reduce knockback by 10% per point of Might below 10
        else {
            knockbackFactor += attackerMight * 0.1f;
        }
    }

    //Adjust knockback based on relative mass between particle and target
    if(pdata.pchr->phys.bumpdampen != 0.0f && CHR_INFINITE_WEIGHT != pdata.pchr->phys.weight) {
        float targetMass, particleMass;
        get_chr_mass(pdata.pchr, &targetMass);
        get_prt_mass(pdata.pprt.get(), pdata.pchr, &particleMass);
        knockbackFactor *= Ego::Math::constrain(particleMass / targetMass, 0.0f, 1.0f);
    }

    //Amount of knockback is affected by damage type
    switch(pdata.pprt->damagetype)
    {
        // very blunt type of attack, the maximum effect
        case DAMAGE_CRUSH:
            knockbackFactor *= 1.0f;
        break;

        // very focussed type of attack, the minimum effect
        case DAMAGE_POKE:
            knockbackFactor *= 0.5f;
        break;        

        // all other damage types are in the middle
        default:
            knockbackFactor *= Ego::Math::invSqrtTwo<float>();
        break;
    }

    //Apply knockback to the victim (limit between 0% and 300% knockback)
    pdata.pchr->phys.avel += pdata.pprt->vel * Ego::Math::constrain(knockbackFactor, 0.0f, 3.0f);
}

//--------------------------------------------------------------------------------------------
bool do_chr_prt_collision( CoNode_t * d )
{
    /// @author BB
    /// @details this funciton goes through all of the steps to handle character-particle
    ///               interactions. A basic interaction has been detected. This needs to be refined
    ///               and then handled. The function returns false if the basic interaction was wrong
    ///               or if the interaction had no effect.
    ///
    /// @note This function is a little more complicated than the character-character case because
    ///       of the friend-foe logic as well as the damage and other special effects that particles can do.

    bool retval = false;

    bool prt_deflected;

    chr_prt_collision_data_t cn_data;
    chr_prt_collision_data_t::init(&cn_data);
    bool intialized;

    // valid node?
    if ( NULL == d ) return false;

    if ( INVALID_CHR_REF != d->chra && INVALID_PRT_REF != d->prtb )
    {
        // character was first
        intialized = do_chr_prt_collision_init( d->chra, d->prtb, &cn_data );
    }
    else if ( INVALID_CHR_REF != d->chrb && INVALID_PRT_REF != d->prta )
    {
        // particle was first
        intialized = do_chr_prt_collision_init( d->chrb, d->prta, &cn_data );
    }
    else
    {
        // not a valid interaction
        intialized = false;

        // in here to keep the compiler from complaining
        chr_prt_collision_data_t::init( &cn_data );
    }

    if ( !intialized ) return false;

    // ignore dead characters
    if ( !cn_data.pchr->isAlive() ) return false;

    // skip objects that are inside inventories
    if ( cn_data.pchr->isInsideInventory() ) return false;

    // if the particle is attached to this character, ignore a "collision"
    if ( cn_data.pprt->getAttachedObject().get() ==  cn_data.pchr )
    {
        return false;
    }

    // is there any collision at all?
    if ( !do_chr_prt_collision_get_details( d, &cn_data ) )
    {
        return false;
    }
    else
    {
        // help classify impacts

        if ( cn_data.is_pressure )
        {
            // on the odd chance that we want to use the pressure
            // algorithm for an obvious collision....
            if ( d->tmin > 0.0f ) cn_data.is_impact = true;

        }

        if ( cn_data.is_collision )
        {
            cn_data.is_impact = true;
        }
    }

    // if there is no collision, no point in going farther
    if ( !cn_data.int_min && !cn_data.int_max /* && !cn_data.int_plat */ ) return false;

    // if the particle is not actually hitting the object, then limit the
    // interaction to 2d
    if ( cn_data.int_max && !cn_data.int_min )
    {
        // do not re-normalize this vector
        cn_data.nrm[kZ] = 0.0f;
    }

    // find the relative velocity
	cn_data.vdiff = cn_data.pchr->vel - cn_data.pprt->vel;

    // decompose the relative velocity parallel and perpendicular to the surface normal
    cn_data.dot = fvec3_decompose(cn_data.vdiff, cn_data.nrm, cn_data.vdiff_perp, cn_data.vdiff_para);

    // handle particle deflection
    prt_deflected = false;
    if ( cn_data.int_min || cn_data.int_max )
    {
        // determine whether the particle is deflected by the character
        prt_deflected = do_chr_prt_collision_deflect( &cn_data );
        if ( prt_deflected )
        {
            retval = true;
        }
    }

    // refine the logic for a particle to hit a character
    bool prt_can_hit_chr = do_chr_prt_collision_bump( &cn_data );

    // Torches and such are marked as invulnerable, so the particle is always deflected.
    // make a special case for reaffirmation
    if (( cn_data.int_min || cn_data.int_max ) && 0 == cn_data.pchr->damage_timer )
    {
        // Check reaffirmation of particles
        if ( cn_data.pchr->reaffirm_damagetype == cn_data.pprt->damagetype )
        {
            // This prevents items in shops from being burned
            if ( !cn_data.pchr->isshopitem )
            {
                if ( 0 != reaffirm_attached_particles( cn_data.ichr ) )
                {
                    retval = true;
                }
            }
        }
    }

    //Do they hit each other?
    if(prt_can_hit_chr && (cn_data.int_min || cn_data.int_max))
    {
        bool dodged = false;

        //Does the character have a dodge ability?
        if(cn_data.pchr->hasPerk(Ego::Perks::DODGE)) {
            float dodgeChance = cn_data.pchr->getAttribute(Ego::Attribute::AGILITY);

            //Masterful Dodge Perk gives flat +10% dodge chance
            if(cn_data.pchr->hasPerk(Ego::Perks::MASTERFUL_DODGE)) {
                dodgeChance += 10.0f;
            }

            //1% dodge chance per Agility
            if(Random::getPercent() <= dodgeChance) 
            {
                dodged = true;
            }
        }

        if(!dodged) {
            // do "damage" to the character
            if (!prt_deflected && 0 == cn_data.pchr->damage_timer )
            {
                // we can't even get to this point if the character is completely invulnerable (invictus)
                // or can't be damaged this round
                cn_data.prt_damages_chr = do_chr_prt_collision_damage( &cn_data );
                if ( cn_data.prt_damages_chr )
                {
                    //Remember the collision so that this doesn't happen again
                    cn_data.pprt->addCollision(_currentModule->getObjectHandler()[cn_data.pchr->getCharacterID()]);
                    retval = true;
                }
            }

    #if 0
            // calculate the impulse.
            if (( cn_data.int_min || cn_data.int_max ) && cn_data.ppip->allowpush )
            {
                do_chr_prt_collision_impulse( &cn_data );
            }

            // make the character and particle recoil from the collision
            if (cn_data.vimpulse.length_abs() > 0.0f ||
                cn_data.pimpulse.length_abs() > 0.0f)
            {
                if ( do_chr_prt_collision_recoil( &cn_data ) )
                {
                    retval = true;
                }
            }
    #endif

            //Cause knockback (Hold the Line perk makes Objects immune to knockback)
            if(!cn_data.pchr->hasPerk(Ego::Perks::HOLD_THE_LINE)) {
                do_chr_prt_collision_knockback(cn_data);
            }

        }

        //Attack was dodged!
        else {
            //Cannot collide again
            cn_data.pprt->addCollision(_currentModule->getObjectHandler()[cn_data.pchr->getCharacterID()]);

            //Play sound effect
            AudioSystem::get().playSound(cn_data.pchr->getPosition(), AudioSystem::get().getGlobalSound(GSND_DODGE));

            // Initialize for the billboard
            const float lifetime = 3;
            chr_make_text_billboard( cn_data.pchr->getCharacterID(), "Dodged!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f(1.0f, 0.6f, 0.0f, 1.0f), lifetime, Billboard::Flags::All);
        }


        // handle a couple of special cases
        if ( cn_data.prt_bumps_chr )
        {
            if ( do_chr_prt_collision_handle_bump( &cn_data ) )
            {
                retval = true;
            }
        }
    }

    // platform interaction. do this last, and only if there is no other interaction
    //if ( cn_data.int_plat && !cn_data.int_max && !cn_data.int_min )
    //{
    //    cn_data.int_plat = do_prt_platform_physics( &cn_data );
    //}

    // terminate the particle if needed
    if ( cn_data.terminate_particle )
    {
        cn_data.pprt->requestTerminate();
        retval = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

chr_prt_collision_data_t * chr_prt_collision_data_t::init( chr_prt_collision_data_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    //---- invalidate the object parameters
    ptr->ichr = INVALID_CHR_REF;
    ptr->pchr = NULL;

    ptr->iprt = INVALID_PRT_REF;
    ptr->pprt = NULL;
    ptr->ppip = NULL;

    //---- collision parameters

    // true collisions
    ptr->int_min = 0;
    ptr->depth_min = 0.0f;

    // hit-box collisions
    ptr->int_max = 0;
    ptr->depth_max = 0.0f;

    // platform interactions
    //ptr->int_plat = false;
    //ptr->plat_lerp = 0.0f;

    // basic parameters
    ptr->is_impact    = false;
    ptr->is_pressure  = false;
    ptr->is_collision = false;
    ptr->dot = 0.0f;
    ptr->nrm = fvec3_t(0, 0, 1);

    //---- collision modifications
    ptr->mana_paid = false;
    ptr->max_damage = ptr->actual_damage = 0;
	ptr->vdiff = fvec3_t::zero();
	ptr->vdiff_para = fvec3_t::zero();
	ptr->vdiff_perp = fvec3_t::zero();
    ptr->block_factor = 0.0f;

    //---- collision reaction
	//ptr->vimpulse = fvec3_t::zero();
	//ptr->pimpulse = fvec3_t::zero();
    ptr->terminate_particle = false;
    ptr->prt_bumps_chr = false;
    ptr->prt_damages_chr = false;

    return ptr;
}
