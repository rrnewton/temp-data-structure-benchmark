/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include <cds/ordered_list/michael_list_nogc.h>
#include <cds/map/split_ordered_list.h>

#include "map/map_test_header.h"

namespace map {

    namespace {
        struct MapTraits: public cds::map::split_list::type_traits
        {
            typedef cds::ordered_list::michael_list_tag<cds::gc::no_gc>   bucket_type ;
        };
    }

    void MapTestHeaderNoGC::SplitList_Dynamic_Michael()
    {
        testWithItemCounter< cds::map::SplitOrderedList<int, int, void, MapTraits> >() ;
    }
}
