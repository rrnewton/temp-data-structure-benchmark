/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include <cds/map/split_ordered_list.h>
#include <cds/ordered_list/michael_list_hrc.h>

#include "map/map_test_header.h"

//#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
//    using namespace std;
//#endif

//
// TestCase class
//
namespace map {
    namespace {
        struct MapTraits: public cds::map::split_list::type_traits
        {
            typedef cds::map::split_list::static_bucket_table<int>          bucket_table     ;
            typedef cds::ordered_list::michael_list_tag<cds::gc::hrc_gc>    bucket_type      ;
        };
    }

    void MapTestHeader::SplitList_Static_Michael_hrc()
    {
        testWithItemCounter< cds::map::SplitOrderedList<int, int, void, MapTraits> >() ;
    }
}
