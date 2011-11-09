/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_QUEUE_MOIR_QUEUE_HP_IMPL_H
#define __CDS_QUEUE_MOIR_QUEUE_HP_IMPL_H

/*
    Editions:
        2010.10.13    Maxim.Khiszinsky    Created, moved from moir_queue_hzp.h
*/

#include <cds/queue/moir_queue.h>

//@cond
namespace cds { namespace queue { namespace details {

    template <class BASE>
    class moir_queue_implementation: public BASE
    {
        typedef BASE    base_class  ;
    public:
        typedef typename base_class::gc_schema     gc_schema   ;
        typedef typename base_class::value_type    value_type  ;
        typedef typename base_class::type_traits   type_traits ;

    protected:
        typedef typename base_class::node_type     node_type       ;
        typedef typename base_class::guard         guard           ;
        typedef typename base_class::guard_array   guard_array     ;

    public:
        /// Dequeues a value to \p dest.
        /**
            If queue is empty returns \a false, \p dest is unchanged.
            If queue is not empty returns \a true, \p dest contains the value dequeued
        */
        bool dequeue( value_type& dest )
        {
            typename type_traits::backoff_strategy bkoff    ;
            typename guard_array::template rebind<2>::other   hpArr( base_class::getGC() )    ;

            node_type * pNext    ;
            node_type * h        ;
            while ( true ) {
                h = base_class::m_pHead.template load<membar_relaxed>()  ;
                hpArr[0] = h    ;
                if ( base_class::m_pHead.template load<membar_acquire>() != h ) {
                    bkoff()    ;
                    continue    ;
                }

                pNext = h->m_pNext.template load<membar_acquire>()   ;
                hpArr[1] = pNext    ;
                if ( base_class::m_pHead.template load<membar_acquire>() != h )
                    continue    ;

                if ( pNext == NULL )
                    return false    ;    // queue is empty

                if ( base_class::m_pHead.template cas<membar_release>( h, pNext )) {
                    node_type * t = base_class::m_pTail.template load<membar_acquire>()    ;
                    if ( h == t )
                        base_class::m_pTail.template cas<membar_release>( t, pNext ) ;
                    break ;
                }

                base_class::m_Stat.onDequeueRace()    ;
                bkoff()    ;
            }

            dest = pNext->m_data     ;
            --base_class::m_ItemCounter            ;
            base_class::m_Stat.onDequeue()        ;
            hpArr.getGC().retirePtr( h, type_traits::node_deleter_functor::free ) ;
            return true    ;
        }

        /// Synonym for \ref dequeue
        bool pop( value_type& data )
        {
            return dequeue( data );
        }
    };

}}} // cds::queue::details
//@endcond

#endif //  #ifndef __CDS_QUEUE_MOIR_QUEUE_HP_IMPL_H
