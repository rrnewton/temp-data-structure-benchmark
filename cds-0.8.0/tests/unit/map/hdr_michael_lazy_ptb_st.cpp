/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include <cds/ordered_list/lazy_list_ptb.h>
#include <cds/map/michael_hash_map.h>

#include "map/map_test_header.h"

//#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
//    using namespace std;
//#endif

//
// TestCase class
//
namespace map {
    namespace {
        struct MapTraits: public cds::map::type_traits
        {
            typedef cds::ordered_list::LazyList< cds::gc::ptb_gc,
                char,
                long,
                cds::map::pair_traits<char, long>,
                void
            >           bucket_type ;
        };
    }

    void MapTestHeader::MichaelHash_Lazy_ptb()
    {
        testWithItemCounter< cds::map::MichaelHashMap<int, int, void, MapTraits> >() ;
    }
}
