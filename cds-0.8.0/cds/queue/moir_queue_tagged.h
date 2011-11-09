/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_QUEUE_MOIR_QUEUE_TAGGED_H
#define __CDS_QUEUE_MOIR_QUEUE_TAGGED_H

/*
    Editions:
        2008.10.02    Maxim.Khiszinsky    Refactoring
*/

#include <cds/queue/moir_queue.h>
// cds/queue/msqueue_tagged.h generates error if wide CAS is not supported by the CPU
#include <cds/queue/msqueue_tagged.h>

#ifdef CDS_DWORD_CAS_SUPPORTED
namespace cds {
    namespace queue {

        namespace details {
            /// A variation of Michael's queue implementation based on IBM tag scheme
            /**
            \par Source:
            [2000] Simon Doherty, Lindsay Groves, Victor Luchangco, Mark Moir
                "Formal Verification of a practical lock-free queue algorithm"

                This implementation is variation of Michael's lock-free queue algorithm based on IBM tag methodology.
                The target platform must support double word CAS primitive.

                \par Template parameters:
                    \li \p T        Type of data saved in queue's node
                    \li \p BACKOFF    The back-off strategy. It applies when the concurrent updates is encountered
                    \li \p FREELIST    The implementation of IBM free list

            */
            template <typename T,
                typename TRAITS = traits,
                class FREELIST= cds::gc::tagged::FreeList< T, typename TRAITS::backoff_strategy >
            >
            class moir_queue_tagged: public msqueue_tagged::queue_impl< T, TRAITS, FREELIST >
            {
                typedef msqueue_tagged::queue_impl< T, TRAITS, FREELIST >    base_class    ;    ///< Base class
                typedef typename base_class::tagged_ptr                tagged_ptr            ;    ///< Tagged pointer type

            public:
                typedef typename base_class::type_traits            type_traits ;    ///< type traits
            public:

                /// Dequeues tail node
                bool dequeue( T& dest )
                {
                    typename type_traits::backoff_strategy    bkoff ;

                    tagged_ptr head    ;
                    while ( true ) {
                        head = atomics::load<membar_acquire>( &base_class::getHeadRef())   ;
                        tagged_ptr next = atomics::load<membar_acquire>( &head.m_data->m_Next ) ;
                        if ( head == atomics::load<membar_acquire>( &base_class::getHeadRef() ) ) {
                            if ( next.data() == NULL )            // queue is empty
                                return false    ;
                            else {
                                dest = next.data()->m_data ;
                                if ( gc::tagged::cas_tagged<membar_release>( base_class::m_Head, head, next.data() )) {
                                    // Remove
                                    tagged_ptr tail = atomics::load<membar_acquire>( &base_class::getTailRef() ) ;
                                    if ( head.data() == tail.data() )
                                        gc::tagged::cas_tagged<membar_release>( base_class::m_Tail, tail, next.data() ) ;
                                    break ;
                                }
                            }
                        }
                        base_class::m_Stat.onDequeueRace()    ;
                        bkoff() ;
                    }

                    --base_class::m_ItemCounter            ;
                    base_class::freeNode( head.data() )    ;
                    return true    ;
                }

                bool pop( T& dest )                { return dequeue( dest ) ; }
            };
        }    // namespace details

        /// A variation of Michael & Scott lock-free queue based on tagged pointer technique
        /**
            The algorithm is a variation of Michael & Scott's queue algorithm. It overloads dequeue function.

            The FreeList implementation is taken from TRAITS::free_list_type. If TRAITS::free_list_type is void (default) then
            default free-list cds::gc::tagged::FreeList< T, TRAITS::backoff_strategy > is used

            \par Template parameters:
                \li \p T        Type of data saved in queue's node
                \li \p TRAITS    Traits class, see @ref cds::queue::traits
                \li \p ALLOCATOR Memory allocator class

            \par
                Interface: see \ref queue_common_interface.
        */
        template <typename T, class TRAITS, class ALLOCATOR >
        class MoirQueue< gc::tagged_gc, T, TRAITS, ALLOCATOR >:
            public queue::details::moir_queue_tagged< T,
                normalized_type_traits<TRAITS>,
                typename queue::details::msqueue_tagged::free_list_selector<T,
                    typename  normalized_type_traits<TRAITS>::backoff_strategy,
                    typename  normalized_type_traits<TRAITS>::free_list_type
                >::free_list_type
            >
        {
        public:
            typedef gc::tagged_gc    gc_schema    ;    ///< Garbage collection schema
        };


    }    // namespace queue
}    // namespace cds

#endif // #ifdef CDS_DWORD_CAS_SUPPORTED

#endif // #ifndef __CDS_QUEUE_MOIR_QUEUE_TAGGED_H
