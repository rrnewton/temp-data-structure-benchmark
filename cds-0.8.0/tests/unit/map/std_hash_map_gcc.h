/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __UNIT_STD_HASH_MAP_GCC_H
#define __UNIT_STD_HASH_MAP_GCC_H

#if defined(__GXX_EXPERIMENTAL_CXX0X)
    //  GCC 4.3 and above
#   include <unordered_map>
    namespace gcc_unordered_map = std ;
#elif __GNUC__ == 4
#   include <tr1/unordered_map>
    namespace gcc_unordered_map = std::tr1 ;
#else
#   error "unordered_map is defined for GCC 4 only"
#endif

#if __GNUC__ == 4 && __GNUC_MINOR__ <= 1
#   define CDS_GCC_ALLOCATOR(A)     typename A::template rebind< std::pair<const KEY, VALUE> >::other
#else
#   define CDS_GCC_ALLOCATOR(A)     A
#endif

#include <cds/ref.h>

namespace map {

    template <typename KEY, typename VALUE, typename LOCK, class ALLOCATOR = CDS_DEFAULT_ALLOCATOR>
    class StdHashMap
        : public gcc_unordered_map::unordered_map<
            KEY, VALUE
            , gcc_unordered_map::hash<KEY>
            , std::equal_to<KEY>
            , CDS_GCC_ALLOCATOR( ALLOCATOR )
        >
    {
    public:
        LOCK m_lock    ;
        typedef cds::lock::Auto<LOCK> AutoLock ;
        typedef gcc_unordered_map::unordered_map<
            KEY, VALUE
            , gcc_unordered_map::hash<KEY>
            , std::equal_to<KEY>
            , CDS_GCC_ALLOCATOR(ALLOCATOR)
        >   base_class ;
    public:
        typedef typename base_class::mapped_type value_type ;

        StdHashMap( size_t nMapSize, size_t nLoadFactor )
        {}

        bool find( const KEY& key )
        {
            AutoLock al( m_lock )    ;
            return base_class::find( key ) != base_class::end() ;
        }

        bool insert( const KEY& key, const VALUE& val )
        {
            AutoLock al( m_lock )    ;
            return base_class::insert( typename base_class::value_type(key, val)).second ;
        }

        template <typename T, typename FUNC>
        bool insert( const KEY& key, const T& val, FUNC func )
        {
            AutoLock al( m_lock )    ;
            std::pair<typename base_class::iterator, bool> pRet = base_class::insert( typename base_class::value_type(key, VALUE() )) ;
            if ( pRet.second ) {
                cds::unref(func)( pRet.first->second, val ) ;
                return true ;
            }
            return false;
        }

        template <typename T, typename FUNC>
        std::pair<bool, bool> ensure( const KEY& key, const T& val, FUNC func )
        {
            AutoLock al( m_lock )   ;
            std::pair<typename base_class::iterator, bool> pRet = base_class::insert( typename base_class::value_type( key, VALUE() )) ;
            if ( pRet.second ) {
                cds::unref(func)( pRet.first->second, val, true ) ;
                return std::make_pair( true, true ) ;
            }
            else {
                cds::unref(func)( pRet.first->second, val, false ) ;
                return std::make_pair( true, false ) ;
            }            
        }

        bool erase( const KEY& key )
        {
            AutoLock al( m_lock )   ;
            return base_class::erase( key ) != 0 ;
        }

        template <typename T, typename FUNC>
        bool erase( const KEY& key, T& dest, FUNC func )
        {
            AutoLock al( m_lock )   ;
            typename base_class::iterator it = base_class::find( key ) ;
            if ( it != base_class::end() ) {
                cds::unref(func)( dest, it->second )   ;
                return base_class::erase( key ) != 0    ;
            }
            return false ;
        }

        std::ostream& dump( std::ostream& stm ) { return stm; }
    };
}   // namespace map

#undef CDS_GCC_ALLOCATOR

#endif  // #ifndef __UNIT_STD_HASH_MAP_GCC_H
