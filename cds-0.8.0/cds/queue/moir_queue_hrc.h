/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_QUEUE_MOIR_QUEUE_HRC_H
#define __CDS_QUEUE_MOIR_QUEUE_HRC_H

/*
Editions:
    2009.04.11    mx  Created
*/

#include <cds/queue/moir_queue.h>
#include <cds/queue/msqueue_hrc.h>

namespace cds { namespace queue {

    ///  A variation of Michael & Scott lock-free queue (Gidenstam's GC)
    /**
        Implementation of Moir et al lock-free queue based on Gidenstam's reference counting GC.
        The algorithm is a variation of Michael & Scott's queue algorithm. It overloads dequeue function.

    \par Source:
        \li [2000] Simon Doherty, Lindsay Groves, Victor Luchangco, Mark Moir
            "Formal Verification of a practical lock-free queue algorithm"
        \li [2002] Maged M.Michael "Safe memory reclamation for dynamic lock-freeobjects using atomic reads and writes"
        \li [2003] Maged M.Michael "Hazard Pointers: Safe memory reclamation for lock-free objects"

    \par Template parameters:
        \li \p T        Type of data saved in queue's node
        \li \p TRAITS    Traits class, see @ref cds::queue::traits
        \li \p ALLOCATOR Memory allocator class

    \par
        Common interface: see \ref queue_common_interface.
    */
    template <typename T, class TRAITS, class ALLOCATOR>
    class MoirQueue< gc::hrc_gc, T, TRAITS, ALLOCATOR >:
        public MSQueue< gc::hrc_gc, T, TRAITS, ALLOCATOR >
    {
        typedef MSQueue< gc::hrc_gc, T, TRAITS, ALLOCATOR >        base_class ;    ///< Base class

    public:
        typedef gc::hrc_gc                            gc_schema    ;    ///< Garbage collection schema
        typedef typename base_class::type_traits    type_traits    ;    ///< Queue type traits

    protected:
        typedef typename base_class::Node                Node                ;    ///< Node type

    public:
        /// Dequeues a value to \p dest.
        /**
            If queue is empty returns \a false, \p dest is unchanged.
            If queue is not empty returns \a true, \p dest contains the value dequeued
        */
        bool dequeue( T& dest )
        {
            typename type_traits::backoff_strategy backoff    ;
            gc::hrc::AutoHPArray<3> hpArr( base_class::getGC() )    ;

            Node * pNext    ;
            Node * h        ;
            while ( true ) {
                h = hpArr.getGC().derefLink( &base_class::getHeadRef(), hpArr[0] )    ;
                if ( base_class::m_pHead != h ) {
                    backoff()    ;
                    continue    ;
                }

                pNext = hpArr.getGC().derefLink( &(h->m_pNext), hpArr[1] ) ;
                if ( base_class::m_pHead != h )
                    continue    ;

                if ( pNext == NULL )
                    return false    ;    // queue is empty

                if ( hpArr.getGC().CASRef( &base_class::getHeadRef(), h, pNext )) {
                    Node * t = hpArr.getGC().derefLink( &base_class::getTailRef(), hpArr[2] ) ;
                    if ( h == t )
                        hpArr.getGC().CASRef( &base_class::getTailRef(), t, pNext ) ;
                    break ;
                }

                base_class::m_Stat.onDequeueRace()    ;
                backoff()    ;
            }

            dest = pNext->m_data     ;
            --base_class::m_ItemCounter            ;
            base_class::m_Stat.onDequeue()        ;
            hpArr.getGC().retireNode( h, hpArr[0] ) ;

            return true    ;
        }

        /// Synonym for \ref dequeue
        bool pop( T& data )                { return dequeue( data ); }
    };

} } // namespace cds::queue

#endif // #ifndef __CDS_QUEUE_MOIR_QUEUE_HRC_H
