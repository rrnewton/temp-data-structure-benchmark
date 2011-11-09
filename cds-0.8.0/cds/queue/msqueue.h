/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_QUEUE_MSQUEUE_H
#define __CDS_QUEUE_MSQUEUE_H

/*
    Michael & Scott lock-free queue algorithm.

    Editions:
        2008.10.02    Maxim.Khiszinsky    Created
*/

#include <cds/queue/details/queue_base.h>
#include <cds/details/allocator.h>

namespace cds {
    namespace queue {

        namespace details {
            /// Michael's queue node. T - type stored in queue
            template <typename T>
            struct msqueue_node {
                atomic<msqueue_node *>  m_pNext ;   ///< Next node
                T                        m_data    ;    ///< Node's data

                //@cond
                msqueue_node( const T& data )
                    : m_pNext( NULL )
                    , m_data( data )
                {}

                msqueue_node()
                    :m_pNext(NULL)
                {}

                msqueue_node *    next(memory_order order)
                {
                    return m_pNext.load(order);
                }
                template <typename ORDER>
                msqueue_node *    next()
                {
                    return m_pNext.template load<ORDER>();
                }
                T&                data()
                {
                    return m_data;
                }
                //@endcond
            };
        }    // namespace details

        /// Michael & Scott queue
        /*
            Declaration of Michael & Scott queue algorithm.
            \par Template parameters
                \li \p GC safe memory reclamation schema (garbage collector)
                \li \p T type of item stored in queue
                \li \p TRAITS class traits. The default is traits
                \li \p ALLOCATOR node allocator. The default is \ref CDS_DEFAULT_ALLOCATOR

            There are specialization for each appropriate reclamation schema \p GC.
        */
        template < typename GC,
            typename T,
            typename TRAITS = traits,
            class ALLOCATOR = CDS_DEFAULT_ALLOCATOR
        >
        class MSQueue    ;


    }    // namespace queue
}    // namespace cds

#endif // #ifndef __CDS_QUEUE_MSQUEUE_H
