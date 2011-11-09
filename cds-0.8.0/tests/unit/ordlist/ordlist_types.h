/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __UNIT_ORDLIST_TYPES_H
#define __UNIT_ORDLIST_TYPES_H

#include <cds/ordered_list/michael_list_hzp.h>
#include <cds/ordered_list/michael_list_hrc.h>
#include <cds/ordered_list/michael_list_ptb.h>
#include <cds/ordered_list/michael_list_nogc.h>
#ifdef CDS_DWORD_CAS_SUPPORTED
#   include <cds/ordered_list/michael_list_tagged.h>
#endif
#include <cds/ordered_list/lazy_list_hzp.h>
#include <cds/ordered_list/lazy_list_hrc.h>
#include <cds/ordered_list/lazy_list_ptb.h>
#include <cds/ordered_list/lazy_list_nogc.h>

namespace ordlist {
    template <typename KEY, typename VALUE>
    struct OrdlistTypes {
        typedef cds::ordered_list::MichaelList<
            cds::gc::hzp_gc
            , KEY
            , VALUE
        >    MichaelList_HP    ;

        typedef cds::ordered_list::MichaelList<
            cds::gc::hrc_gc
            , KEY
            , VALUE
        >    MichaelList_HRC    ;

        typedef cds::ordered_list::MichaelList<
            cds::gc::ptb_gc
            , KEY
            , VALUE
        >    MichaelList_PTB    ;

        typedef cds::ordered_list::MichaelList<
            cds::gc::no_gc
            , KEY
            , VALUE
        >    MichaelList_NoGC    ;

#ifdef CDS_DWORD_CAS_SUPPORTED
        typedef cds::ordered_list::MichaelList<
            cds::gc::tagged_gc
            , KEY
            , VALUE
        >    MichaelList_Tagged    ;

        struct MichaelListTaggedSharedFreeListTraits: public cds::ordered_list::type_traits
        {
            typedef cds::gc::tagged::FreeList< int >&    free_list_type   ;
        };
        typedef cds::ordered_list::MichaelList<
            cds::gc::tagged_gc
            , KEY
            , VALUE
            , cds::map::pair_traits< KEY, VALUE >
            , MichaelListTaggedSharedFreeListTraits
        >    MichaelList_TaggedShared    ;
#endif

        typedef cds::ordered_list::LazyList<
            cds::gc::hzp_gc
            , KEY
            , VALUE
        > LazyList_HP   ;

        typedef cds::ordered_list::LazyList<
            cds::gc::hrc_gc
            , KEY
            , VALUE
        > LazyList_HRC   ;

        typedef cds::ordered_list::LazyList<
            cds::gc::ptb_gc
            , KEY
            , VALUE
        > LazyList_PTB   ;

        typedef cds::ordered_list::LazyList<
            cds::gc::no_gc
            , KEY
            , VALUE
        > LazyList_NoGC ;

        /*
        typedef cds::ordered_list::HarrisList<
            cds::gc::hzp_gc
            , KEY
            , VALUE
        >    HarrisList_HP    ;

        typedef cds::ordered_list::HarrisList<
            cds::gc::hrc_gc
            , KEY
            , VALUE
        >    HarrisList_HRC    ;
        */

    };

}   // namespace ordlist


#endif  // #ifndef __UNIT_ORDLIST_TYPES_H
