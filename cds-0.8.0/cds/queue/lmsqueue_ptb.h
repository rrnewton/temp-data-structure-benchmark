/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_QUEUE_LMSQUEUE_PTB_H
#define __CDS_QUEUE_LMSQUEUE_PTB_H

/*
    Editions:
        2010.10.14 0.7.0 Maxim Khiszinsky   Created
*/

#include <cds/queue/details/lmsqueue_hp_impl.h>
#include <cds/gc/hzp/hp_object.h>
#include <cds/gc/ptb/container.h>

namespace cds {
    namespace queue {

        //@cond none
        namespace details {
            template <
                typename T,
                class TRAITS,
                class ALLOCATOR
            >
            class lmsqueue_adapter<gc::ptb_gc, T, TRAITS, ALLOCATOR>
            {
            public:
                typedef gc::ptb_gc  gc_schema   ;
                typedef gc::ptb::Container<details::lmsqueue_hp::regular_node<T>, ALLOCATOR >    container_base  ;
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
                typedef gc::ptb::GuardArray<3>  guard_array ;
            };
        }
        //@endcond

        /// Ladan-Mozes & Shavit optimistic queue based on Pass-The-Buck (gc::ptb_gc) reclamation schema
        /**
            Ladan-Mozes & Shavit optimistic queue algorithm based on Hazard Pointer (gc::hzp_gc) reclamation schema
            \par Template parameters
                \li \p T    type of the data stored in the queue
                \li \p TRAITS class traits. The default is QueueTraits
                \li \p ALLOCATOR node allocator. The default is \ref CDS_DEFAULT_ALLOCATOR.

            \par Source
                \li [2004] Edya Ladan-Mozes, Nir Shavit "An Optimistic Approach to Lock-Free FIFO Queues"

            \par
                Common interface: see \ref queue_common_interface.
        */
        template <typename T, class TRAITS, class ALLOCATOR >
        class LMSQueue< gc::ptb_gc, T, TRAITS, ALLOCATOR >:
            public details::lmsqueue_implementation< details::lmsqueue_adapter<gc::ptb_gc, T, TRAITS, ALLOCATOR> >
        {} ;

    } // namespace queue
} // namespace cds

#endif // #ifndef __CDS_QUEUE_LMSQUEUE_HZP_H
