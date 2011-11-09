/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ORDERED_LIST_MICHAEL_LIST_H
#define __CDS_ORDERED_LIST_MICHAEL_LIST_H

#include <cds/map/_base.h>
#include <cds/ordered_list/list_base.h>
#include <cds/ordered_list/details/michael_list_node.h>

namespace cds {
    namespace ordered_list {
        /// Michael ordered list
        /**
            Declaration of Michael's ordered list

            \par Template parameters
                \li \p GC        lock-free reclamation schema (garbage collector)
                \li \p KEY        type of key stored in list
                \li \p VALUE    type of value stored in list
                \li \p PAIR_TRAITS (key, value) pair traits. The default is cds::map::pair_traits <\p KEY, \p VALUE>
                \li \p TRAITS    list traits. The default is cds::ordered_list::type_traits
                \li \p ALLOCATOR node allocator. The default is \ref CDS_DEFAULT_ALLOCATOR

            There are specializations for each appropriate reclamation schema \p GC.

            \par Source
                [2002] Maged Michael "High performance dynamic lock-free hash tables and list-based sets"

            \par
                Common interface: see \ref ordered_list_common_interface.
        */
        template < class GC,
            typename KEY,
            typename VALUE,
            typename PAIR_TRAITS = cds::map::pair_traits< KEY, VALUE >,
            typename TRAITS = type_traits,
            class ALLOCATOR = CDS_DEFAULT_ALLOCATOR
        >
        class MichaelList   ;

        /// Selector for implementation of Michael's ordered list
        template <class GC>
        struct michael_list_tag
        {
            typedef GC  gc_schema   ;   ///< Garbage collector schema used by implementation
        };

        //@cond
        namespace split_list {

            template < class GC,
                typename REGULAR_KEY,
                typename DUMMY_KEY,
                typename VALUE,
                typename PAIR_TRAITS = cds::map::pair_traits< REGULAR_KEY, VALUE >,
                typename TRAITS = type_traits,
                class ALLOCATOR = CDS_DEFAULT_ALLOCATOR
            >
            class MichaelList    ;

        }   // namespace split_list
        //@endcond

    }    // namespace ordered_list
}    // namespace cds

#endif // #ifndef __CDS_ORDERED_LIST_MICHAEL_LIST_H
