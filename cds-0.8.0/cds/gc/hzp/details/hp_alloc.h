/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_HZP_DETAILS_HP_ALLOC_H
#define __CDS_GC_HZP_DETAILS_HP_ALLOC_H

#include <cds/details/bounded_array.h>
#include <cds/atomic.h>
#include <cds/gc/hzp/details/hp_fwd.h>
#include <cds/gc/hzp/details/hp_type.h>

namespace cds {
    namespace gc { namespace hzp {
    /// Hazard Pointer schema implementation details
    namespace details {

        /// Hazard pointer guard
        /**
            It is unsafe to use this class directly.
            Instead of it the AutoHPGuard class should be used.

            Template parameter:
                \li HPTYPE - type of hazard pointer. It is \ref HazardPtr for Michael's Hazard Pointer reclamation schema
        */
        template <typename HPTYPE>
        class HPGuardT
        {
            HPTYPE *        m_hzp        ;    ///< Hazard pointer allocated by HPAllocator

            template < typename HPTYPE2, class ALLOCATOR > friend class HPAllocator    ;

        public:
            CDS_CONSTEXPR HPGuardT()
#        ifdef CDS_DEBUG
                : m_hzp( NULL )
#        endif
            {}
            ~HPGuardT()
            {}

            /// Sets HP value. Guards pointer \p p from reclamation.
            template <typename T>
            T * operator =( T * p )
            {
                // We use atomic store with explicit memory order because other threads may read this hazard pointer concurrently
                atomics::store<membar_release>( m_hzp, reinterpret_cast<HPTYPE>(p) )    ;
                return p    ;
            }

            /// Returns current HP
            operator HPTYPE()    { return *m_hzp; }

            /// Returns reference to Hazard Pointer
            HPTYPE& getHPRef() { return *m_hzp; }

            /// Clears HP
            void clear()
            {
                *m_hzp = NULL;  // memory order is not necessary here (by default, relaxed)
                //CDS_COMPILER_RW_BARRIER ;
            }
        };

        /// Specialization of HPGuardT for HazardPtr type
        typedef HPGuardT<HazardPtr> HPGuard    ;

        /// Array of hazard pointers.
        /**
            Array of hazard-pointer. Placing a pointer into this array guards the pointer against reclamation.
            Template parameter \p COUNT defines the size of hazard pointer array. \p COUNT parameter should not exceed
            GarbageCollector::getHazardPointerCount().

            It is unsafe to use this class directly. Instead, the AutoHPArray should be used.

            While creating the object of HPArray class an array of size \p COUNT of hazard pointers is reserved by
            the HP Manager of current thread. The object's destructor cleans all of reserved hazard pointer and
            returns reserved HP to the HP pool of ThreadGC.

            Usually, it is not necessary to create an object of this class. The object of class ThreadGC contains
            the HPArray object and implements interface for HP setting and freeing.

            Template parameter:
                \li HPTYPE - type of hazard pointer. It is HazardPtr usually
                \li COUNT - capacity of array

        */
        template <typename HPTYPE, size_t COUNT>
        class HPArrayT
        {
            HPTYPE *    m_arr               ;   ///< Hazard pointer array of size = COUNT

            template < typename HPTYPE2, class ALLOCATOR > friend class HPAllocator    ;

        public:
            /// Constructs uninitialized array.
            CDS_CONSTEXPR HPArrayT()
            {}

            /// Destructs object
            ~HPArrayT() {}

            /// Returns max count of hazard pointer for this array
            size_t        capacity() const    { return COUNT;        }

            /// Set hazard pointer \p nIndex. 0 <= \p nIndex < \p COUNT
            void set( size_t nIndex, HPTYPE hzPtr )
            {
                assert( nIndex < COUNT )    ;
                atomics::store<membar_release>( m_arr + nIndex, hzPtr )   ;
            }

            /// Returns reference to hazard pointer of index \p nIndex (0 <= \p nIndex < \p COUNT)
            HPTYPE& operator []( size_t nIndex )
            {
                assert( nIndex < COUNT )    ;
                return *( m_arr + nIndex )    ;
            }

            /// Clears (sets to NULL) hazard pointer \p nIndex
            void clear( size_t nIndex )
            {
                assert( nIndex < COUNT )        ;
                m_arr[ nIndex ] = NullHazardPtr    ;   // memory order is not necessary here (by default, relaxed)
                //CDS_COMPILER_RW_BARRIER         ;
            }
        };

        /// Specialization of HPArrayT class for HazardPtr type
        template <size_t COUNT>
        class HPArray: public HPArrayT<HazardPtr, COUNT>
        {};

        /// Allocator of hazard pointers for the thread
        /**
            The max size (capacity) of array is defined at ctor time and cannot be changed during object's lifetime
            The array contains free item list that manages allocation and deallocation of array's free items
            (i.e. unused hazard pointers)

            Template parameters:
                \li HPTYPE - type of hazard pointer (HazardPtr usually)
                \li ALLOCATOR - memory allocator class, default is \ref CDS_DEFAULT_ALLOCATOR

            This helper class should not be used directly.
        */
        template < typename HPTYPE, class ALLOCATOR = CDS_DEFAULT_ALLOCATOR >
        class HPAllocator
        {
            typedef cds::details::BoundedArray< HPTYPE, ALLOCATOR >    THazardArray    ;    ///< Hazard pointer array type

            THazardArray                    m_arrHazardPtr  ;   ///< Array of hazard pointers
            unsigned int                    m_nTop          ;   ///< The top of stack

        public:
            /// Default ctor
            explicit HPAllocator(
                size_t nCapacity            ///< capacity
                )
                : m_arrHazardPtr( nCapacity )
            {
                makeFreeStack()    ;
                assert( m_arrHazardPtr.size() == capacity() )  ;
            }

            ~HPAllocator()
            {
                assert( size() == capacity() ) ;
            }

            /// Get capacity of array
            size_t capacity() const        { return m_arrHazardPtr.capacity()    ; }

            /// Get size of array. The size should be equal to the capacity of array
            size_t size() const            { return m_arrHazardPtr.size()    ; }

            /// Checks if all items are allocated
            bool isFull() const            { return m_nTop == 0; }

            /// Allocates hazard pointer
            void alloc( HPGuardT<HPTYPE>& hp )
            {
                assert( m_nTop > 0 )    ;
                --m_nTop    ;
                hp.m_hzp = m_arrHazardPtr + m_nTop  ;
                assert( m_arrHazardPtr.size() == capacity() )   ;
            }

            /// Frees previously allocated hazard pointer
            void free( HPGuardT<HPTYPE>& /*hp*/ )
            {
                assert( m_nTop < capacity() )   ;
                m_arrHazardPtr[ m_nTop ] = HPTYPE(0)    ;
                ++m_nTop ;
                CDS_COMPILER_RW_BARRIER ;
                assert( m_arrHazardPtr.size() == capacity() )   ;
            }

            /// Allocates hazard pointers array
            /**
                Allocates \p COUNT hazard pointers from array \p m_arrHazardPtr
                Returns initialized object \p arr
            */
            template <size_t COUNT>
            void alloc( HPArrayT<HPTYPE, COUNT>& arr )
            {
                assert( m_nTop >= COUNT )   ;
                m_nTop -= COUNT ;
                arr.m_arr = m_arrHazardPtr.top() + m_nTop   ;
                assert( m_arrHazardPtr.size() == capacity() )   ;
            }

            /// Frees hazard pointer array
            /**
                Frees the array of hazard pointers allocated by previous call \p this->alloc.
            */
            template <size_t COUNT>
            void free( const HPArrayT<HPTYPE, COUNT>& arr )
            {
                assert( m_nTop + COUNT <= capacity())   ;
                std::fill( m_arrHazardPtr.begin() + m_nTop, m_arrHazardPtr.begin() + m_nTop + COUNT, HPTYPE(0) ) ;
                m_nTop += COUNT ;
                assert( m_arrHazardPtr.size() == capacity() )   ;
            }

            /// Makes all HP free
            void clear()
            {
                makeFreeStack()    ;
                assert( m_arrHazardPtr.size() == capacity() )  ;
            }

            /// Returns to i-th hazard pointer
            HPTYPE operator []( size_t i )
            {
                assert( m_arrHazardPtr.size() == capacity() )  ;
                assert( i < capacity() )    ;
                return m_arrHazardPtr[i]    ;
            }

        private:
            //@cond
            void makeFreeStack()
            {
                assert( m_arrHazardPtr.size() == capacity() )   ;
                m_arrHazardPtr.assign( capacity(), HPTYPE(0) )   ;
                m_nTop = (unsigned int) capacity() ;
            }
            //@endcond
        };


    }}} // namespace gc::hzp::details
}   // namespace cds

#endif // #ifndef __CDS_GC_HZP_DETAILS_HP_ALLOC_H
