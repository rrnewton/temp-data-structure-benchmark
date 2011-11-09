/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include <cds/map/michael_hash_map.h>

#ifdef CDS_DWORD_CAS_SUPPORTED
#   include <cds/ordered_list/michael_list_tagged.h>
#endif

#include "map/map_test_header.h"

//
// TestCase class
//
namespace map {
#ifdef CDS_DWORD_CAS_SUPPORTED
    namespace {
        struct MapTraits: public cds::map::type_traits
        {
            typedef cds::ordered_list::MichaelList<    cds::gc::tagged_gc,
                int,
                int,
                cds::map::pair_traits<int, int>,
                void
            >           bucket_type ;
        };
    }
#endif

    void MapTestHeader::MichaelHash_Michael_tagged()
    {
#ifdef CDS_DWORD_CAS_SUPPORTED
        testWithItemCounter< cds::map::MichaelHashMap<int, int, void, MapTraits> >() ;
#else
        CPPUNIT_MSG( "This CPU does not support wide CAS primitive, test skipped" ) ;
#endif
    }
}

