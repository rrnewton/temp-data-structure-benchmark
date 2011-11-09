/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ORDERED_LIST_TRIVIAL_FUNCTOR_H
#define __CDS_ORDERED_LIST_TRIVIAL_FUNCTOR_H

/*
    Editions:
        2011.01.13 khizmax  [0.7.2] Created
*/

//#include <cds/details/defs.h>

//@cond
namespace cds { namespace ordered_list { namespace details { 

    struct trivial_erase_functor 
    {
        template <typename VALUE_TYPE>
        void operator()( int&, VALUE_TYPE& ) {}
    };

}}} // namespace cds::ordered_list::details
//@endcond

#endif  // #ifndef __CDS_ORDERED_LIST_TRIVIAL_FUNCTOR_H
