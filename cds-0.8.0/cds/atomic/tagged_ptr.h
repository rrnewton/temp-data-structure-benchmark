/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ATOMIC_TAGGED_PTR_H
#define __CDS_ATOMIC_TAGGED_PTR_H

#include <cds/atomic.h>

#if CDS_BUILD_BITS == 32
#   define CDS_TAGGED_ALIGNMENT       8
#elif CDS_BUILD_BITS == 64
#   define CDS_TAGGED_ALIGNMENT       16
#else
#   error "Unsupported value of CDS_BUILD_BITS macro"
#endif
#define CDS_TAGGED_ALIGN_ATTRIBUTE    CDS_TYPE_ALIGNMENT(CDS_TAGGED_ALIGNMENT)

#ifdef CDS_DWORD_CAS_SUPPORTED

namespace cds { namespace atomics {

    /// Tagged pointer
    template <typename T>
    class CDS_TAGGED_ALIGN_ATTRIBUTE tagged_ptr {
    public:
        typedef T *             pointer ;   ///< pointer type

    private:
        T * volatile            m_ptr   ;   ///< pointer
        uptr_atomic_t volatile  m_tag   ;   ///< version number (tag)

    public:
        CDS_CONSTEXPR tagged_ptr()
            : m_ptr(NULL)
            , m_tag(0)
        {}

        explicit tagged_ptr( T * p, uptr_atomic_t nTag = 0 )
            : m_ptr( p )
            , m_tag( nTag )
        {}

        tagged_ptr( const tagged_ptr<T>& src )
            : m_ptr( src.m_ptr )
            , m_tag( src.m_tag )
        {}

        T * operator ->()
        {
            assert( m_ptr != NULL ) ;
            return m_ptr    ;
        }

        /// Returns pointer
        T * ptr()
        {
            return const_cast<T *>( m_ptr )    ;
        }

        const T * ptr() const
        {
            return const_cast<T *>( m_ptr )    ;
        }

        bool isNull() const
        {
            return m_ptr == NULL ;
        }

        bool cas( const tagged_ptr<T>& expected, T * desired, memory_order success_order, memory_order failure_order )
        {
            CDS_STATIC_ASSERT( sizeof( tagged_ptr<T> ) == 2 * sizeof( void * ) )    ;
            assert( cds::details::is_aligned<CDS_TAGGED_ALIGNMENT>( this ) )   ;

            tagged_ptr<T> newVal( desired, m_tag + 1 )    ;
            return atomics::cas( this, expected, newVal, success_order, failure_order )  ;
        }

        template <typename SUCCESS_ORDER>
        bool cas( const tagged_ptr<T>& expected, T * desired )
        {
            CDS_STATIC_ASSERT( sizeof( tagged_ptr<T> ) == 2 * sizeof( void * ) )    ;
            assert( cds::details::is_aligned<CDS_TAGGED_ALIGNMENT>( this ) )   ;

            tagged_ptr<T> newVal( desired, m_tag + 1 )    ;
            return atomics::cas<SUCCESS_ORDER>( this, expected, newVal )  ;
        }

        T * vcas( const tagged_ptr<T>& expected, T * desired, memory_order success_order, memory_order failure_order )
        {
            CDS_STATIC_ASSERT( sizeof( tagged_ptr<T> ) == 2 * sizeof( void * ) )    ;
            assert( cds::details::is_aligned<CDS_TAGGED_ALIGNMENT>( this ) )   ;

            tagged_ptr<T> newVal( desired, m_tag + 1 )    ;
            return const_cast<T *>( atomics::vcas( this, expected, newVal, success_order, failure_order ).m_ptr ) ;
        }

        template <typename SUCCESS_ORDER>
        T * vcas( const tagged_ptr<T>& expected, T * desired )
        {
            CDS_STATIC_ASSERT( sizeof( tagged_ptr<T> ) == 2 * sizeof( void * ) )    ;
            assert( cds::details::is_aligned<CDS_TAGGED_ALIGNMENT>( this ) )   ;

            tagged_ptr<T> newVal( desired, m_tag + 1 )    ;
            return const_cast<T *>( atomics::vcas<SUCCESS_ORDER>( this, expected, newVal ).m_ptr )  ;
        }
    };

}}  // namespace cds::atomics

#endif  // #ifdef CDS_DWORD_CAS_SUPPORTED

#endif  // __CDS_ATOMIC_TAGGED_PTR_H