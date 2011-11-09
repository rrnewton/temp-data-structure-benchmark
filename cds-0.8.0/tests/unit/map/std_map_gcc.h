/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __UNIT_STD_MAP_GCC_H
#define __UNIT_STD_MAP_GCC_H

#include <map>
#include <cds/ref.h>

#if __GNUC__ * 100 + __GNUC_MINOR__ < 401
#   define CDS_GCC_ALLOCATOR(A)     typename A::template rebind< std::pair<const KEY, VALUE> >::other
#else
#   define CDS_GCC_ALLOCATOR(A)     A
#endif

namespace map {

    template <typename KEY, typename VALUE, typename LOCK, class ALLOCATOR = CDS_DEFAULT_ALLOCATOR>
    class StdMap: public std::map<KEY, VALUE, std::less<KEY>, CDS_GCC_ALLOCATOR(ALLOCATOR)>
    {
        LOCK m_lock    ;
        typedef cds::lock::Auto<LOCK> AutoLock ;
        typedef std::map<KEY, VALUE, std::less<KEY>, CDS_GCC_ALLOCATOR(ALLOCATOR)> base_class ;
    public:
        typedef typename base_class::mapped_type value_type ;

        StdMap( size_t nMapSize, size_t nLoadFactor )
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
            std::pair<typename base_class::iterator, bool> pRet = base_class::insert( typename base_class::value_type(key, VALUE() )) ;
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

#endif  // #ifndef __UNIT_STD_MAP_GCC_H
