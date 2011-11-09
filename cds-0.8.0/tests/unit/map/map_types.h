/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __UNIT_MAP_TYPES_H
#define __UNIT_MAP_TYPES_H

#include <cds/map/split_ordered_list.h>
#include <cds/map/michael_hash_map.h>
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

#include <cds/lock/spinlock.h>
#include "lock/nolock.h"
#ifdef WIN32
#    include "lock/win32_lock.h"
#endif
#include "ordlist/ordlist_types.h"

#include "map/std_map.h"
#include "map/std_hash_map.h"

namespace map {

    template <typename KEY, typename VALUE>
    struct MapTypes {

        // Split-Ordered list implementations
        typedef cds::map::SplitOrderedList< KEY, VALUE>                                                    SplitListDyn_Michael_HP     ;

        struct StaticMichaelSplitListTraits: public cds::map::split_list::type_traits {
            typedef cds::map::split_list::static_bucket_table< KEY >        bucket_table    ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, StaticMichaelSplitListTraits>                SplitListStatic_Michael_HP  ;

        struct DynamicMichaelSplitListHRCTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::michael_list_tag<cds::gc::hrc_gc>    bucket_type ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, DynamicMichaelSplitListHRCTraits>            SplitListDyn_Michael_HRC   ;

        struct StaticMichaelSplitListHRCTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::michael_list_tag<cds::gc::hrc_gc>    bucket_type ;
            typedef cds::map::split_list::static_bucket_table< unsigned int >        bucket_table    ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, StaticMichaelSplitListHRCTraits>            SplitListStatic_Michael_HRC ;

        struct DynamicMichaelSplitListPTBTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::michael_list_tag<cds::gc::ptb_gc>    bucket_type ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, DynamicMichaelSplitListPTBTraits>            SplitListDyn_Michael_PTB   ;

        struct StaticMichaelSplitListPTBTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::michael_list_tag<cds::gc::ptb_gc>    bucket_type ;
            typedef cds::map::split_list::static_bucket_table< unsigned int >        bucket_table    ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, StaticMichaelSplitListPTBTraits>            SplitListStatic_Michael_PTB ;

        struct DynamicMichaelSplitListNoGCTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::michael_list_tag<cds::gc::no_gc>    bucket_type ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, DynamicMichaelSplitListNoGCTraits>            SplitListDyn_Michael_NoGC   ;

        struct StaticMichaelSplitListNoGCTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::michael_list_tag<cds::gc::no_gc>    bucket_type ;
            typedef cds::map::split_list::static_bucket_table< unsigned int >        bucket_table    ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, StaticMichaelSplitListNoGCTraits>                SplitListStatic_Michael_NoGC ;

#ifdef CDS_DWORD_CAS_SUPPORTED
        struct DynamicMichaelSplitListTaggedTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::michael_list_tag<cds::gc::tagged_gc>    bucket_type ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, DynamicMichaelSplitListTaggedTraits>            SplitListDyn_Michael_Tagged   ;

        struct StaticMichaelSplitListTaggedTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::michael_list_tag<cds::gc::tagged_gc>    bucket_type ;
            typedef cds::map::split_list::static_bucket_table< unsigned int >        bucket_table    ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, StaticMichaelSplitListTaggedTraits>            SplitListStatic_Michael_Tagged ;

        //struct DynamicMichaelSplitListTaggedSharedTraits: public cds::map::split_list::type_traits {
        //    typedef ordlist::OrdlistTypes<int,int>::MichaelList_TaggedShared    bucket_type    ;
        //};
        //typedef cds::map::SplitOrderedList< KEY, VALUE, void, DynamicMichaelSplitListTaggedSharedTraits>    SplitListDyn_Michael_TaggedShared   ;

        //struct StaticMichaelSplitListTaggedSharedTraits: public cds::map::split_list::type_traits {
        //    typedef ordlist::OrdlistTypes<int,int>::MichaelList_TaggedShared    bucket_type    ;
        //    typedef cds::map::split_list::static_bucket_table< unsigned int >        bucket_table    ;
        //};
        //typedef cds::map::SplitOrderedList< KEY, VALUE, void, StaticMichaelSplitListTaggedTraits>            SplitListStatic_Michael_TaggedShared ;
#endif

        /*
        struct DynamicHarrisSplitListTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::harris_list_tag<cds::gc::hzp_gc>    bucket_type ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, DynamicHarrisSplitListTraits>                    SplitListDyn_Harris_HP        ;

        struct DynamicHarrisSplitListHRCTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::harris_list_tag<cds::gc::hrc_gc>    bucket_type ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, DynamicHarrisSplitListHRCTraits>                SplitListDyn_Harris_HRC       ;

        struct StaticHarrisSplitListTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::harris_list_tag<cds::gc::hzp_gc>    bucket_type ;
            typedef cds::map::split_list::static_bucket_table< unsigned int >        bucket_table    ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, StaticHarrisSplitListTraits>                    SplitListStatic_Harris_HP     ;

        struct StaticHarrisSplitListHRCTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::harris_list_tag<cds::gc::hrc_gc>    bucket_type ;
            typedef cds::map::split_list::static_bucket_table< unsigned int >        bucket_table    ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, StaticHarrisSplitListHRCTraits>                SplitListStatic_Harris_HRC    ;
        */


        struct DynamicLazySplitListTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::lazy_list_tag<cds::gc::hzp_gc>    bucket_type ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, DynamicLazySplitListTraits>                    SplitListDyn_Lazy_HP        ;

        struct StaticLazySplitListTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::lazy_list_tag<cds::gc::hzp_gc>    bucket_type ;
            typedef cds::map::split_list::static_bucket_table< unsigned int >        bucket_table    ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, StaticLazySplitListTraits>                    SplitListStatic_Lazy_HP     ;

        struct DynamicLazySplitListHRCTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::lazy_list_tag<cds::gc::hrc_gc>    bucket_type ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, DynamicLazySplitListHRCTraits>                SplitListDyn_Lazy_HRC       ;

        struct StaticLazySplitListHRCTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::lazy_list_tag<cds::gc::hrc_gc>    bucket_type ;
            typedef cds::map::split_list::static_bucket_table< unsigned int >        bucket_table    ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, StaticLazySplitListHRCTraits>                 SplitListStatic_Lazy_HRC    ;

        struct DynamicLazySplitListPTBTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::lazy_list_tag<cds::gc::ptb_gc>    bucket_type ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, DynamicLazySplitListPTBTraits>                SplitListDyn_Lazy_PTB       ;

        struct StaticLazySplitListPTBTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::lazy_list_tag<cds::gc::ptb_gc>    bucket_type ;
            typedef cds::map::split_list::static_bucket_table< unsigned int >        bucket_table    ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, StaticLazySplitListPTBTraits>                 SplitListStatic_Lazy_PTB    ;

        struct DynamicLazySplitListNoGCTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::lazy_list_tag<cds::gc::no_gc>    bucket_type ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, DynamicLazySplitListNoGCTraits>                SplitListDyn_Lazy_NoGC      ;

        struct StaticLazySplitListNoGCTraits: public cds::map::split_list::type_traits {
            typedef cds::ordered_list::lazy_list_tag<cds::gc::no_gc>    bucket_type ;
            typedef cds::map::split_list::static_bucket_table< unsigned int >        bucket_table    ;
        };
        typedef cds::map::SplitOrderedList< KEY, VALUE, void, StaticLazySplitListNoGCTraits>                SplitListStatic_Lazy_NoGC   ;

        // Michael Hash Map
        typedef cds::map::MichaelHashMap< KEY, VALUE >                                              MichaelHashMap_Michael_HP   ;

        struct MichaelHashMichaelHRCTraits: public cds::map::type_traits {
            typedef cds::ordered_list::MichaelList<cds::gc::hrc_gc, int, int>    bucket_type    ;
        };
        typedef cds::map::MichaelHashMap< KEY, VALUE, void, MichaelHashMichaelHRCTraits >           MichaelHashMap_Michael_HRC  ;

        struct MichaelHashMichaelPTBTraits: public cds::map::type_traits {
            typedef cds::ordered_list::MichaelList<cds::gc::ptb_gc, int, int>    bucket_type    ;
        };
        typedef cds::map::MichaelHashMap< KEY, VALUE, void, MichaelHashMichaelPTBTraits >           MichaelHashMap_Michael_PTB  ;

        struct MichaelHashMichaelHPTraits: public cds::map::type_traits {
            typedef cds::ordered_list::MichaelList<cds::gc::hzp_gc, int, int>    bucket_type    ;
        };
        typedef cds::map::MichaelHashMap< KEY, VALUE, void, MichaelHashMichaelHPTraits >            MichaelHashMap_Michael_NoGC ;
#ifdef CDS_DWORD_CAS_SUPPORTED
        struct MichaelHashMichaelTaggedTraits: public cds::map::type_traits {
            typedef cds::ordered_list::MichaelList<cds::gc::tagged_gc, int, int>    bucket_type    ;
        };
        typedef cds::map::MichaelHashMap< KEY, VALUE, void, MichaelHashMichaelTaggedTraits >            MichaelHashMap_Michael_Tagged ;

        struct MichaelHashMichaelTaggedSharedTraits: public cds::map::type_traits {
            typedef ordlist::OrdlistTypes<int,int>::MichaelList_TaggedShared    bucket_type    ;
        };
        typedef cds::map::MichaelHashMap< KEY, VALUE, void, MichaelHashMichaelTaggedSharedTraits >   MichaelHashMap_Michael_TaggedShared ;
#endif

        /*
        struct MichaelHashHarrisHPTraits: public cds::map::type_traits {
            typedef cds::ordered_list::HarrisList<cds::gc::hzp_gc, unsigned int, unsigned int>    bucket_type    ;
        };
        typedef cds::map::MichaelHashMap< KEY, VALUE, void, MichaelHashHarrisHPTraits >             MichaelHashMap_Harris_HP    ;

        struct MichaelHashHarrisHRCTraits: public cds::map::type_traits {
            typedef cds::ordered_list::HarrisList<cds::gc::hrc_gc, unsigned int, unsigned int>    bucket_type    ;
        };
        typedef cds::map::MichaelHashMap< KEY, VALUE, void, MichaelHashHarrisHRCTraits >            MichaelHashMap_Harris_HRC    ;
        */

        struct MichaelHashLazyHPTraits: public cds::map::type_traits {
            typedef cds::ordered_list::LazyList<cds::gc::hzp_gc, unsigned int, unsigned int>    bucket_type    ;
        };
        typedef cds::map::MichaelHashMap< KEY, VALUE, void, MichaelHashLazyHPTraits >               MichaelHashMap_Lazy_HP      ;

        struct MichaelHashLazyHRCTraits: public cds::map::type_traits {
            typedef cds::ordered_list::LazyList<cds::gc::hrc_gc, unsigned int, unsigned int>    bucket_type    ;
        };
        typedef cds::map::MichaelHashMap< KEY, VALUE, void, MichaelHashLazyHRCTraits >              MichaelHashMap_Lazy_HRC     ;

        struct MichaelHashLazyPTBTraits: public cds::map::type_traits {
            typedef cds::ordered_list::LazyList<cds::gc::ptb_gc, unsigned int, unsigned int>    bucket_type    ;
        };
        typedef cds::map::MichaelHashMap< KEY, VALUE, void, MichaelHashLazyPTBTraits >              MichaelHashMap_Lazy_PTB     ;

        struct MichaelHashLazyNoGCTraits: public cds::map::type_traits {
            typedef cds::ordered_list::LazyList<cds::gc::no_gc, int, int>    bucket_type    ;
        };
        typedef cds::map::MichaelHashMap< KEY, VALUE, void, MichaelHashLazyNoGCTraits >        MichaelHashMap_Lazy_NoGC    ;


        // Standard implementations
        typedef StdMap< KEY, VALUE, cds::SpinLock >            StdMap_Spin        ;
        typedef StdMap< KEY, VALUE, lock::NoLock>            StdMap_NoLock    ;

        typedef StdHashMap< KEY, VALUE, cds::SpinLock >        StdHashMap_Spin    ;
        typedef StdHashMap< KEY, VALUE, lock::NoLock >        StdHashMap_NoLock    ;

#ifdef WIN32
        typedef StdMap< KEY, VALUE, lock::win::CS >            StdMap_WinCS        ;
        typedef StdHashMap< KEY, VALUE, lock::win::CS >        StdHashMap_WinCS    ;
#endif
    } ;
}

#endif  // #ifndef __UNIT_MAP_TYPES_H
