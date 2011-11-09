/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ORDERED_LIST_LAZY_LIST_H
#define __CDS_ORDERED_LIST_LAZY_LIST_H

#include <cds/map/_base.h>
#include <cds/ordered_list/list_base.h>
#include <cds/ordered_list/details/lazy_list_node.h>
#include <cds/lock/spinlock.h>

namespace cds {
    namespace ordered_list {

        //@cond
        namespace details { namespace lazy_list {

            ///< Lock type selector chooses default implementation of node's locker if input lock type is void.
            template <typename LOCK>
            struct lock_type {
                typedef LOCK                type   ;    ///< Lock type
            };

            template <>
            struct lock_type<void> {
                typedef cds::lock::Spin     type   ;
            };
        }}    // namespace details::lazy_list
        //@endcond


        /// Lazy list
        /**
            Declaration of ordered Lazy list

            \par Template parameters
                \li \p GC        lock-free reclamation schema (garbage collector)
                \li \p KEY        type of key stored in list
                \li \p VALUE    type of value stored in list
                \li \p PAIR_TRAITS (key, value) pair traits. The default is cds::map::pair_traits <\p KEY, \p VALUE>
                \li \p TRAITS    list traits. The default is cds::ordered_list::type_traits
                \li \p ALLOCATOR node allocator. The default is \ref CDS_DEFAULT_ALLOCATOR

            There are specializations for each appropriate reclamation schema \p GC.

            \par Source
                [2005] Steve Heller, Maurice Herlihy, Victor Luchangco, Mark Moir, William N. Scherer III, and Nir Shavit
                "A Lazy Concurrent List-Based Set Algorithm"

            \par
                Common interface: see \ref ordered_list_common_interface.
        */
        template < class GC,
            typename KEY,
            typename VALUE,
            typename PAIR_TRAITS = map::pair_traits< KEY, VALUE >,
            typename TRAITS = type_traits,
            class ALLOCATOR = CDS_DEFAULT_ALLOCATOR
        >
        class LazyList    ;

        /// Selector for implementation of lazy ordered list
        template <class GC>
        struct lazy_list_tag
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
            class LazyList    ;

        }   // namespace split_list
        //@endcond


    }    // namespace ordered_list
}    // namespace cds

#endif // #ifndef __CDS_ORDERED_LIST_MICHAEL_LIST_H
