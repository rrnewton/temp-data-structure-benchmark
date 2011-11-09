/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_HRC_SCHEMA_RETIRED_H
#define __CDS_GC_HRC_SCHEMA_RETIRED_H

#include <cds/gc/hrc/details/hrc_fwd.h>

#include <cds/details/bounded_array.h>

namespace cds { namespace gc { namespace hrc {
    namespace details {

        /// Retired node descriptor
        struct retired_node {
            atomic<ContainerNode *>     m_pNode        ;    ///< node to destroy
            //ContainerNode * volatile    m_pNode        ;    ///< node to destroy
            size_t                      m_nNextFree    ;    ///< Next free item in retired array
            signed_ref_counter          m_nClaim       ;    ///< Access to reclaimed node
            atomic<bool>                m_bDone        ;    ///< the record is in work (concurrent access flag)

            /// Default ctor
            CDS_CONSTEXPR retired_node()
                : m_pNode( NULL ),
                m_bDone( false )
            {}

            /// Assignment ctor
            retired_node(
                ContainerNode * pNode   ///< Node to retire
                )
                : m_pNode( pNode ),
                m_bDone( false )
            {}

            /// Compares two \ref retired_node
            static bool Less( const retired_node& p1, const retired_node& p2 )
            {
                return p1.m_pNode.load<membar_relaxed>() < p2.m_pNode.load<membar_relaxed>() ;
            }

            /// Assignment operator
            retired_node& operator =( ContainerNode * pNode )
            {
                m_bDone.store<membar_relaxed>( false )  ;
                m_nClaim = 0    ;
                m_pNode.store<membar_release>( pNode )  ;
                CDS_COMPILER_RW_BARRIER    ;
                return *this    ;
            }
        };

        /// Compare two retired node
        /**
            This comparison operator is needed for sorting pointers on
            deallocation step
        */
        static inline bool operator <( const retired_node& p1, const retired_node& p2 )
        {
            return retired_node::Less( p1, p2 ) ;
        }

        /// Array of ready for destroying pointers
        /**
            The array object is belonged to one thread: only owner thread may write to this array,
            all other thread can read one.
        */
        class retired_vector {
            typedef cds::details::BoundedArray<retired_node>        vector_type     ;   ///< type of vector of retired pointer

            //@cond
            static const size_t m_nEndFreeList = size_t(0) -  1 ;    ///< End of free list
            //@endcond
            size_t            m_nFreeList    ;    ///< Index of first free item in m_arr
            vector_type        m_arr    ;    ///< Array of retired pointers (implicit \ref CDS_DEFAULT_ALLOCATOR dependence)

        public:
            /// Iterator over retired pointer vector
            typedef vector_type::iterator                       iterator        ;
            /// Const iterator type
            typedef vector_type::const_iterator                 const_iterator  ;

        public:
            /// Cror
            retired_vector( const GarbageCollector& mgr )    ;    // inline
            ~retired_vector()
            {}

            /// Capacity (max available size) of array
            size_t capacity() const        { return m_arr.capacity(); }

            /// Returns count of retired node in array. This function is intended for debug purposes only
            size_t getRetiredNodeCount() const
            {
                size_t nCount = 0   ;
                const size_t nCapacity = capacity() ;
                for ( size_t i = 0; i < nCapacity; ++i ) {
                    if ( m_arr[i].m_pNode.load<membar_relaxed>() != NULL )
                        ++nCount    ;
                }
                return nCount   ;
            }

            /// Push new item to the array
            void push( ContainerNode * p )
            {
                assert( !isFull()) ;

                size_t n = m_nFreeList    ;
                assert( m_arr[n].m_nClaim == 0 )    ;
                assert( m_arr[n].m_pNode.load<membar_relaxed>() == NULL )  ;
                m_nFreeList = m_arr[n].m_nNextFree    ;
                CDS_DEBUG_DO( m_arr[n].m_nNextFree = m_nEndFreeList ; )
                m_arr[n] = p        ;
            }

            /// Pop item \p n from array
            void pop( size_t n )
            {
                assert( n < capacity() )    ;
                m_arr[n].m_pNode.store<membar_relaxed>( NULL )        ;
                m_arr[n].m_nNextFree = m_nFreeList    ;
                m_nFreeList = n                ;
            }

            /// Check if array is full
            bool isFull() const
            {
                return m_nFreeList == m_nEndFreeList    ;
            }

            /// Get item by index \p i. It is not error if \p i >= @ref capacity()
            retired_node& operator []( size_t i )
            {
                assert( i < capacity() )    ;
                return m_arr[i]    ;
            }

            /// Returns a random-access iterator to the first element in the retired pointer vector
            /**
                If the vector is empty, end() == begin().
            */
            iterator        begin()         { return m_arr.begin(); }
            /// Const version of begin()
            const_iterator  begin() const   { return m_arr.begin(); }

            /// A random-access iterator to the end of the vector object.
            /**
                If the vector is empty, end() == begin().
            */
            iterator        end()           { return m_arr.end(); }
            /// Const version of end()
            const_iterator  end() const     { return m_arr.end(); }
        };

    }    // namespace details
}}}    // namespace cds::gc::hrc

#endif // #ifndef __CDS_GC_HRC_SCHEMA_RETIRED_H
