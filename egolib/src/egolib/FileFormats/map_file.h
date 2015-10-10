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

#pragma once

#include "egolib/typedef.h"
#include "egolib/_math.h"
#include "egolib/FileFormats/map_tile_dictionary.h"
#include "egolib/Math/Vector.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define CURRENT_MAP_VERSION_LETTER 'D'

// mesh constants
#define MAP_ID_BASE 0x4D617041 // The string... MapA

/// The maximum number of tiles in x direction.
constexpr uint32_t MAP_TILE_MAX_X = 1024;
/// The maximum number of tiles in y direction.
constexpr uint32_t MAP_TILE_MAX_Y = 1024;
/// The maximum number of tiles.
constexpr uint32_t MAP_TILE_MAX = MAP_TILE_MAX_X * MAP_TILE_MAX_Y;
/// The maximum number of vertices.
constexpr uint32_t MAP_VERTICES_MAX = MAP_TILE_MAX*MAP_FAN_VERTICES_MAX;

#   define TILE_BITS   7
#   define TILE_ISIZE (1<<TILE_BITS)
#   define TILE_MASK  (TILE_ISIZE - 1)
#   define TILE_FSIZE ((float)TILE_ISIZE)

// tile constants
#   define MAP_TILE_TYPE_MAX         256                     ///< Max number of tile images



#include "egolib/FileFormats/map_fx.hpp"

#   define VALID_MPD_TILE_RANGE(VAL)   ( ((size_t)(VAL)) < MAP_TILE_MAX )
#   define VALID_MPD_VERTEX_RANGE(VAL) ( ((size_t)(VAL)) < MAP_VERTICES_MAX )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The basic parameters needed to create an mpd
struct map_info_t
{
    /// The number of vertices.
	uint32_t vertexCount;
    /// The number of tiles in the x direction.
	uint32_t tileCountX;
    /// The number of tiles in the y direction.
	uint32_t tileCountY;

    /**
     * @brief
     *  Get the number of vertices.
     * @return
     *  the number of vertices
     */
    uint32_t getVertexCount() const
    {
        return vertexCount;
    }

    /**
     * @brief
     *  Get the number of tiles in the x direction.
     * @return
     *  the number of tiles in the x direction
     */
    uint32_t getTileCountX() const
    {
        return tileCountX;
    }

    /**
     * @brief
     *  Load creation parameters from a file.
     * @param file
     *  the source file
     */
    void load(vfs_FILE& file);

    /**
     * @brief
     *  Save creation parameters to a file.
     * @param file
     *  the target file
     */
    void save(vfs_FILE& file) const;

    /**
     * @brief
     *  Validate creation parameters.
     * @return
     *  @a true if the creation parameters are valid, @a false otherwise
     */
    bool validate() const;

    /**
     * @brief
     *  Reset this creation parameters to their default values.
     * @remark
     *  The default values are for an empty map.
     */
    void reset();

    /**
     * @brief
     *  Default constructor.
     * @remark
     *  The default values are for an empty map.
     */
    map_info_t();

    /**
     * @brief
     *  Copy constructor.
     * @param other
     *  the source of the copy operation
     */
    map_info_t(const map_info_t& other);

    /**
     * @brief
     *  Assignment operator.
     * @param other
     *  the source of the copy operation
     */
    map_info_t& operator=(const map_info_t& other);

};

//--------------------------------------------------------------------------------------------

/// The information for an mpd tile.
struct tile_info_t
{
    uint8_t   type;   ///< Tile type
    uint16_t  img;    ///< Get texture from this
    uint8_t   fx;     ///< Special effects flags
    uint8_t   twist;
};

//--------------------------------------------------------------------------------------------

/// The information for a single mpd vertex.
struct map_vertex_t
{
	Vector3f pos; ///< Vertex position.
    uint8_t a;      ///< Vertex base light.
};

//--------------------------------------------------------------------------------------------

/// A wrapper for the dynamically allocated memory in an mpd
struct map_mem_t
{
    std::vector<tile_info_t> tiles;
    std::vector<map_vertex_t> vertices;
    map_mem_t();
    map_mem_t(uint32_t tileCount,uint32_t vertexCount);
    virtual ~map_mem_t();
    void setInfo(uint32_t tileCount, uint32_t vertexCount);
};

//--------------------------------------------------------------------------------------------

/// The data describing a single mpd
struct map_t
{
public:
    map_info_t _info;
    map_mem_t _mem;
public:

    /**
     * @brief
     *  Create this map.
     * @param info
     *  the map info. Default is an empty map.
     */
    map_t(const map_info_t& info = map_info_t());

    /**
     * @brief
     *  Destruct this map.
     */
    virtual ~map_t();

    /**
     * @brief
     *  Set the map info.
     * @param info
     *  the map info. Default is an empty map.
     */
    bool setInfo(const map_info_t& info = map_info_t());

    /**
     * @brief
     *  Load a map from a file.
     * @param file
     *  the file to load the map from
     */
    bool load(vfs_FILE& file);

    /**
     * @brief
     *  Load a map from a file.
     * @param name
     *  the name to load the map from
     */
    bool load(const std::string& name);

    /**
     * @brief
     *  Save a map to a file.
     * @param file
     *  the file to save the map to
     */
    bool save(vfs_FILE& file) const;

    /**
     * @brief
     *  Save a map to a file.
     * @param name
     *  the name to save the map to
     */
    bool save(const std::string& name) const;

};

