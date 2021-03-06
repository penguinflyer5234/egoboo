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

/// @file    egolib/hash.h
/// @details Implementation of the "efficient" hash node storage.

#pragma once

#include "egolib/typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

	/**
	 * @brief
	 *	A node in a hash list.
	 */
    struct hash_node_t
    {
        hash_node_t *next; /// @todo Rename to _next.
        void *data; /// @todo Rename to _data.
        hash_node_t() :
            next(nullptr), data(nullptr)
        {
        }
        /// @todo Rename ArgData to data (after renaming data to _data).
        hash_node_t(void *ArgData) :
            next(nullptr), data(ArgData)
        {
        }
        static hash_node_t *ctor(hash_node_t *self, void *data)
        {
            if (!self)
            {
                return nullptr;
            }
            self->next = nullptr;
            self->data = data;
            return self;
        }
        static void dtor(hash_node_t *self)
        {
            EGOBOO_ASSERT(nullptr != self);
            self->data = nullptr;
        }
    };

    hash_node_t *hash_node_insert_after( hash_node_t lst[], hash_node_t * n );
    hash_node_t *hash_node_insert_before( hash_node_t lst[], hash_node_t * n );
    hash_node_t *hash_node_remove_after( hash_node_t lst[] );
    hash_node_t *hash_node_remove( hash_node_t lst[] );

//--------------------------------------------------------------------------------------------
    struct hash_list_t
    {
		/**
		 * @brief
		 *	The capacity of this hash list i.e. the number of elements in the array pointed to by @a sublist.
		 */
		size_t capacity;
		/**
		 * @brief
		 *	The number of entries in each bucket.
		 */
        int *subcount;
		/**
		 * @brief
		 *	A pointer to an array of @a capacity buckets.
		 */
        hash_node_t **sublist;
		/**
		 * @brief
		 *	Construct this hash list.
		 * @param initialCapacity
		 *	the initial capacity of the hash list
		 */
		hash_list_t(size_t initialCapacity) :
			capacity(initialCapacity),
			subcount(nullptr),
			sublist(nullptr)
		{
			subcount = EGOBOO_NEW_ARY(int, initialCapacity);
			if (!subcount)
			{
				throw std::bad_alloc();
			}
			sublist = EGOBOO_NEW_ARY(hash_node_t *, initialCapacity);
			if (!sublist)
			{
				EGOBOO_DELETE(subcount);
				subcount = nullptr;
				throw std::bad_alloc();
			}
			else
			{
				for (size_t i = 0, n = initialCapacity; i < n; ++i)
				{
					subcount[i] = 0;
					sublist[i] = nullptr;
				}
			}
		}
		/**
		 * @brief
		 *	Destruct this hash list.
		 */
		virtual ~hash_list_t()
		{
			EGOBOO_DELETE_ARY(subcount);
			subcount = nullptr;
			EGOBOO_DELETE_ARY(sublist);
			sublist = nullptr;
			capacity = 0;
		}


	    //Disable copying class
	    hash_list_t(const hash_list_t& copy) = delete;
	    hash_list_t& operator=(const hash_list_t&) = delete;

		/**
		 * @brief
		 *	Remove all entries from this hash list.
		 */
		void clear()
		{
			for (size_t i = 0, n = capacity; i < n; ++i)
			{
				subcount[i] = 0;
				sublist[i] = nullptr;
			}

		}
		/**
		 * @brief
		 *	Get the size of this hash list.
		 * @return
		 *	the size of this hash list
		 */
		size_t getSize() const
		{
			size_t size = 0;
			for (size_t i = 0, n = capacity; i < n; ++i)
			{
				size += subcount[i];
			}
			return size;
		}
        /**
         * @brief
         *	Get the number of elements in a bucket of a hash list.
         * @param self
         *	the hash list
         * @param index
         *	the bucket index
         * @return
         *	the number of elements in the bucket of the specified index
         */
        static size_t get_count(hash_list_t *self, size_t index)
        {
            if (!self || !self->subcount)
            {
                return 0;
            }
            return self->subcount[index];
        }
		/**
		 * @brief
		 *	Get the capacity of this hash list.
		 * @return
		 *	the capacity of this hash list
		 */
		size_t getCapacity() const
		{
			return capacity;
		}

    };

    hash_node_t *hash_list_get_node(hash_list_t *self, size_t index);
    bool hash_list_set_count(hash_list_t *self, size_t index, size_t count);
    bool hash_list_set_node(hash_list_t *self, size_t index, hash_node_t *node);

//--------------------------------------------------------------------------------------------

/// An iterator element for traversing the hash_list_t
    struct hash_list_iterator_t
    {
        int hash;
        hash_node_t * pnode;
		hash_list_iterator_t *ctor();
    };

    void *hash_list_iterator_ptr(hash_list_iterator_t * it);
    bool hash_list_iterator_set_begin( hash_list_iterator_t * it, hash_list_t * hlst );
    bool hash_list_iterator_done( hash_list_iterator_t * it, hash_list_t * hlst );
    bool hash_list_iterator_next( hash_list_iterator_t * it, hash_list_t * hlst );