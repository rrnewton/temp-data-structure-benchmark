/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_QUEUE_MSQUEUE_PTB_H
#define __CDS_QUEUE_MSQUEUE_PTB_H

/*
    Michael & Scott lock-free queue algorithm based on Pass-the-Buck reclamation schema

    Editions:
        2010.08.30  Maxim.Khiszinsky    Created
*/

#include <cds/queue/details/msqueue_hp_impl.h>
#include <cds/gc/ptb/container.h>

namespace cds {
    namespace queue {

        //@cond
        namespace details {

            template <
                typename T,
                class TRAITS,
                class ALLOCATOR
            >
            class msqueue_adapter<gc::ptb_gc, T, TRAITS, ALLOCATOR >
            {
            public:
                typedef gc::ptb_gc  gc_schema   ;
                typedef gc::ptb::Container<details::msqueue_node<T>, ALLOCATOR >    container_base  ;
                typedef T   value_type  ;
                typedef typename container_base::Node   node_type   ;

                struct type_traits: public normalized_type_traits<TRAITS>
                {
                    typedef typename cds::details::void_selector<
                        typename normalized_type_traits<TRAITS>::original::node_deleter_functor,
                        typename container_base::deferral_node_deleter
                    >::type       node_deleter_functor   ;
                };

                typedef gc::ptb::Guard          guard   ;
                typedef gc::ptb::GuardArray<2>  guard_array ;
            };
        }   // namespace details
        //@endcond

        /// Michael's Pass-the-Buck queue
        /**
            \par Source:
                \li [1998] Maged Michael, Michael Scott "Simple, fast, and practical non-blocking and blocking
                            concurrent queue algorithms"

            \par Template parameters:
                \li \p T        Type of data stored in queue's node
                \li \p TRAITS    Traits class, see cds::queue::traits
                \li \p ALLOCATOR Memory allocator template

            \par
                Interface: see \ref queue_common_interface.
        */
        template <typename T, class TRAITS, class ALLOCATOR >
        class MSQueue< gc::ptb_gc, T, TRAITS, ALLOCATOR >
            : public details::msqueue_implementation< details::msqueue_adapter< gc::ptb_gc, T, TRAITS, ALLOCATOR > >
        {};

    } // namespace queue
} // namespace cds

#endif    // #ifndef __CDS_QUEUE_MSQUEUE_PTB_H
