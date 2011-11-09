/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_REF_H
#define __CDS_REF_H

#include <cds/details/defs.h>
#include <boost/ref.hpp>

namespace cds {

    using boost::ref    ;

    //@cond none
    namespace details {
        template <typename T> 
        struct unwrap_reference 
        {
            typedef T type  ;
            static T apply(T f)
            {
                return f ;
            }
        };

        template <typename T>
        struct unwrap_reference<T&>
        {
            typedef T& type ;
            static T& apply( T& f )
            {
                return f ;
            }
        };

        template <typename T>
        struct unwrap_reference<boost::reference_wrapper<T> >
        {
            typedef T& type ;
            static T& apply( boost::reference_wrapper<T> f )
            {
                return f.get() ;
            }
        };
    }
    //@endcond

    //@cond
    template <typename T>
    static inline typename details::unwrap_reference<T>::type unref( T f )
    {
        return details::unwrap_reference<T>::apply(f)   ;
    }

    //@endcond
}

#endif // #ifndef __CDS_REF_H