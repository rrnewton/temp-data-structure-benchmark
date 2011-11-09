/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_QUEUE_MOIR_QUEUE_HZP_H
#define __CDS_QUEUE_MOIR_QUEUE_HZP_H

/*
    Editions:
        2010.10.13    Maxim.Khiszinsky  The implementation is generalized and is moved to details/moir_queue_hp_impl.h
        2008.10.02    Maxim.Khiszinsky  Refactoring
*/

#include <cds/queue/details/moir_queue_hp_impl.h>
#include <cds/queue/msqueue_hzp.h>

namespace cds {
    namespace queue {

        /// A variation of Michael & Scott lock-free queue based on Hazard Pointer GC
        /**
        The algorithm is a variation of Michael & Scott's queue algorithm. It overloads dequeue function.

        \par Source:
            \li [2000] Simon Doherty, Lindsay Groves, Victor Luchangco, Mark Moir
                "Formal Verification of a practical lock-free queue algorithm"
            \li [2002] Maged M.Michael "Safe memory reclamation for dynamic lock-freeobjects using atomic reads and writes"
            \li [2003] Maged M.Michael "Hazard Pointers: Safe memory reclamation for lock-free objects"

        \par Template parameters:
            \li \p T        Type of data saved in queue's node
            \li \p TRAITS    Traits class (see @ref cds::queue::traits)
            \li \p ALLOCATOR Memory allocator class

        \par
            Interface: see \ref queue_common_interface.
        */
        template <typename T, class TRAITS, class ALLOCATOR>
        class MoirQueue< gc::hzp_gc, T, TRAITS, ALLOCATOR >:
            public details::moir_queue_implementation< MSQueue< gc::hzp_gc, T, TRAITS, ALLOCATOR > >
        {};

        //template <typename T, class TRAITS, class ALLOCATOR>
        //class MoirQueue< gc::hzp_gc, T, TRAITS, ALLOCATOR >:
        //    public MSQueue< gc::hzp_gc, T, TRAITS, ALLOCATOR >
        //{
        //    typedef MSQueue< gc::hzp_gc, T, TRAITS, ALLOCATOR >        base_class ;    ///< Base class

        //public:
        //    typedef typename base_class::gc_schema            gc_schema    ;    ///< Garbage collection schema
        //    typedef typename base_class::type_traits        type_traits    ;    ///< Queue type traits

        //protected:
        //    typedef typename base_class::node_type            node_type   ;    ///< Node type

        //public:
        //    /// Dequeues a value to \p dest.
        //    /**
        //        If queue is empty returns \a false, \p dest is unchanged.
        //        If queue is not empty returns \a true, \p dest contains the value dequeued
        //    */
        //    bool dequeue( T& dest )
        //    {
        //        typename type_traits::backoff_strategy backoff    ;
        //        gc::hzp::AutoHPArray<2> hpArr( base_class::getGC() )    ;
        //        //THazardMgr hzmgr( base_class::getHazardMgr() )    ;

        //        node_type * pNext    ;
        //        node_type * h        ;
        //        while ( true ) {
        //            h = base_class::m_pHead.template load<membar_relaxed>()  ;
        //            hpArr[0] = h    ;
        //            if ( base_class::m_pHead.template load<membar_acquire>() != h ) {
        //                backoff()    ;
        //                continue    ;
        //            }

        //            pNext = h->m_pNext.template load<membar_acquire>()   ;
        //            hpArr[1] = pNext    ;
        //            if ( base_class::m_pHead.template load<membar_acquire>() != h )
        //                continue    ;

        //            if ( pNext == NULL )
        //                return false    ;    // queue is empty

        //            //if ( CAS( &base_class::getHeadRef(), h, pNext )) {
        //            if ( base_class::getHeadRef().template cas<membar_release>( h, pNext )) {
        //                node_type * t = base_class::m_pTail.template load<membar_acquire>()    ;
        //                if ( h == t )
        //                    base_class::getTailRef().template cas<membar_release>( t, pNext ) ;
        //                    //CAS( &base_class::getTailRef(), t, pNext ) ;
        //                break ;
        //            }

        //            base_class::m_Stat.onDequeueRace()    ;
        //            backoff()    ;
        //        }

        //        dest = pNext->m_data     ;
        //        --base_class::m_ItemCounter            ;
        //        base_class::m_Stat.onDequeue()        ;
        //        hpArr.getGC().retirePtr( h, base_class::deferral_node_deleter::free ) ;
        //        return true    ;
        //    }

        //    /// Synonym for \ref dequeue
        //    bool pop( T& data )                { return dequeue( data ); }
        //};

    } // namespace queue
} // namespace cds

#endif // #ifndef __CDS_QUEUE_MOIR_QUEUE_HZP_H
