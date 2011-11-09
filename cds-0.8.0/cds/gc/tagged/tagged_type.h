/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_TAGGED_GC_TAGGED_TYPE_H
#define __CDS_GC_TAGGED_GC_TAGGED_TYPE_H

/*
    Editions:
        2011.02.19  Maxim.Khiszinsky    Moved from cds/gc/tagged/gc.h to separated header
*/

#include <cds/atomic/tagged_ptr.h>

#ifdef CDS_DWORD_CAS_SUPPORTED
namespace cds { namespace gc { namespace tagged {

    /// Tag type
    typedef uptr_atomic_t        ABA_tag    ;

    /// Tagged data
    template <typename T>
    struct CDS_TAGGED_ALIGN_ATTRIBUTE tagged_type 
    {
        T volatile m_data           ;   ///< Data. Condition: sizeof(data) == sizeof(uptr_atomic_t)
        ABA_tag volatile m_nTag     ;   ///< ABA-prevention tag. It might not be decreased during lifetime.

        /// Default ctor. Initializes data to \p NULL and the tag to 0.
        CDS_CONSTEXPR tagged_type()
            : m_data( 0 )
            , m_nTag(0)
        {
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(uptr_atomic_t) )    ;
            CDS_STATIC_ASSERT(sizeof(tagged_type<T>) == 2 * sizeof(uptr_atomic_t)) ;
        }

        /// Constructor
        tagged_type(
            T data,         ///< data
            ABA_tag nTag    ///< Tag value
            )
            : m_data( data )
            , m_nTag( nTag )
        {
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(uptr_atomic_t) )    ;
            CDS_STATIC_ASSERT(sizeof(tagged_type<T>) == 2 * sizeof(uptr_atomic_t)) ;
        }

        /// Copy constructor
        tagged_type( const tagged_type<T>& v )
            : m_data( v.m_data )
            , m_nTag( v.m_nTag )
        {}

        /// Assignment operator.
        tagged_type<T>&    operator =( const tagged_type<T>& src )
        {
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(uptr_atomic_t) )    ;
            CDS_STATIC_ASSERT(sizeof(tagged_type<T>) == 2 * sizeof(uptr_atomic_t)) ;

            m_data = src.m_data    ;
            m_nTag = src.m_nTag    ;
            return *this        ;
        }

        /// Get value of tagged_type
        T  data()               { return const_cast<T>( m_data ); }     // drop volatile
        /// Get value of tagged_type
        T  data() const         { return const_cast<T>( m_data ); }     // drop volatile
        /// Get reference to tagged_type's value
        T& ref()                { return const_cast<T&>( m_data ); }    // drop volatile

        /// Stores \p val into tagged_type and increments its tag
        template <typename ORDER>
        void set( T val )
        {
            atomics::store<ORDER>( this, tagged_type(val, m_nTag + 1) ) ;
        }

        /// Tagged data equality
        /**
            Tagged value is equal iff its value AND its tag are equal.
        */
        friend inline bool operator ==( const tagged_type<T>& r1, const tagged_type<T>& r2 )
        {
            return r1.m_data == r2.m_data && r1.m_nTag == r2.m_nTag    ;
        }

        //@cond
        friend inline bool operator !=( const tagged_type<T>& r1, const tagged_type<T>& r2 )
        {
            return !( r1 == r2 ) ;
        }
        //@endcond
    }  ;

    /// CAS specialization for \ref tagged_type<T>
    template <typename T>
    static inline bool cas_tagged( 
        tagged_type<T> volatile & dest,     ///< Destination. Success CAS increments tag value
        const tagged_type<T>& curVal,       ///< Current value of \p dest
        T dataNew,                          ///< New value of \p dest
        memory_order success_order,         ///< memory order constraint for success CAS
        memory_order failure_order          ///< memory order constraint for failed CAS
    )
    {
        CDS_STATIC_ASSERT( sizeof( tagged_type<T> ) == 2 * sizeof( void * ) )    ;
        assert( cds::details::is_aligned<CDS_TAGGED_ALIGNMENT>( &dest ) )   ;

        tagged_type<T> newVal( dataNew, curVal.m_nTag + 1 )    ;
        return atomics::cas( &dest, curVal, newVal, success_order, failure_order )  ;
    }

    /// CAS specialization for \ref tagged_type<T>
    /**
        Template arguments:
            \li \p ORDER memory order constraint for success CAS
            \li \p T type of value stored in \p dest
    */
    template <typename ORDER, typename T>
    static inline bool cas_tagged(  
        tagged_type<T> volatile & dest,     ///< Destination. Success CAS increments tag value
        const tagged_type<T>& curVal,       ///< Current value of \p dest
        T dataNew                           ///< New value of \p dest
    )
    {
        CDS_STATIC_ASSERT( sizeof( tagged_type<T> ) == 2 * sizeof( void * ) )    ;
        assert( cds::details::is_aligned<CDS_TAGGED_ALIGNMENT>( &dest ) )   ;

        tagged_type<T> newVal( dataNew, curVal.m_nTag + 1 )    ;
        return atomics::cas<ORDER>( &dest, curVal, newVal )  ;
    }

}}} // namespace cds::gc::tagged
#endif  // #ifdef CDS_DWORD_CAS_SUPPORTED

#endif // #ifndef __CDS_GC_TAGGED_GC_TAGGED_TYPE_H
