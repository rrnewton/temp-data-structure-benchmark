/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_IMPL_BOUNDED_ARRAY_H
#define __CDS_IMPL_BOUNDED_ARRAY_H

/*
    Dynamic non-growing array

    Editions:
        2008.03.08    Maxim.Khiszinsky    Created
*/

#include <cds/details/allocator.h>
#include <vector>
#include <algorithm>    // std::fill
#include <assert.h>

namespace cds {
    /// Implementation details
    //@cond
    namespace details {

        /// Upper bounded dynamic array
        /** BoundedArray is dynamic allocated C-array of item of type T with the interface like STL.
            The max size (capacity) of array is defined at ctor time and cannot be changed during object's lifetime
            \par Template parameters
                \li \p T type of elements
                \li \p ALLOCATOR dynamic memory allocator class
        */
        template <typename T, class ALLOCATOR = CDS_DEFAULT_ALLOCATOR >
        class BoundedArray: private std::vector< T, typename ALLOCATOR::template rebind<T>::other >
        {
        public:
            typedef std::vector<T, typename ALLOCATOR::template rebind<T>::other>   vector_type     ;

            typedef typename vector_type::iterator          iterator        ;    ///< item iterator
            typedef typename vector_type::const_iterator    const_iterator    ;    ///< item const iterator

        public:
            /// Default ctor
            explicit BoundedArray(
                size_t nCapacity            ///< capacity
            )
            {
                vector_type::resize( nCapacity )  ;
                assert( size() == capacity() )  ;
            }

            /// Ctor with item's initialization
            BoundedArray(
                size_t nCapacity,            ///< capacity of array
                const T& init,                ///< initial value of any item
                size_t nInitCount = 0        ///< how many items will be initialized; 0 - all items
            )
            {
                assert( nInitCount <= nCapacity )   ;
                vector_type::resize( nCapacity )    ;
                assign( nInitCount ? nInitCount : vector_type::capacity(), init )  ;
                assert( size() == capacity() )      ;
            }

            const T& operator []( size_t nItem ) const      { return vector_type::operator[](nItem); }
            T&       operator []( size_t nItem )            { return vector_type::operator[](nItem); }
            size_t   size() const                           { return vector_type::size(); }
            size_t   capacity() const                       { return vector_type::capacity(); }
            void     assign( size_t nCount, const T& val )
            {
                assert( nCount <= size() )  ;
                std::fill( begin(), begin() + nCount, val ) ;
            }

            /// Returns sizeof(T)
            static size_t itemSize()
            {
                return sizeof(T);
            }

            /// Returns pointer to the first item in the array
            T * top()
            {
                return & vector_type::front()   ;
            }

            friend T * operator +( BoundedArray<T, ALLOCATOR>& arr, size_t i )
            {
                return &( arr[i] )  ;
            }

            /// Get begin iterator
            const_iterator  begin() const   { return vector_type::begin();  }
            iterator        begin()         { return vector_type::begin();  }

            /// Get end iterator
            const_iterator    end() const        { return vector_type::end(); }
            iterator        end()            { return vector_type::end(); }

            /// Get end iterator for \p nMax-th item
            const_iterator    end( size_t nMax )    const
            {
                assert( nMax <= vector_type::capacity())    ;
                return vector_type::begin() + nMax    ;
            }
            iterator        end( size_t nMax )
            {
                assert( nMax <= vector_type::capacity())    ;
                return vector_type::begin() + nMax    ;
            }
        };
    }    // namespace details
    //@endcond
}    // namespace cds

#endif    // #ifndef __CDS_IMPL_BOUNDED_ARRAY_H
