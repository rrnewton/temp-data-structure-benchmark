/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_QUEUE_DETAILS_QUEUE_BASE_H
#define __CDS_QUEUE_DETAILS_QUEUE_BASE_H

/*
    Editions:
        2008.10.02    Maxim Khiszinsky    Refactoring
*/

#include <boost/iterator.hpp>

#include <cds/concept.h>
#include <cds/atomic.h>
#include <cds/backoff_strategy.h>
#include <cds/details/void_selector.h>


/** \page queue_common_interface Queue common interface

    Any queue implementation in CDS library supports the following interface:

    \par Stable interface

\member{enqueue}
\code
    bool enqueue (const T &data)
\endcode
        Places new item with \a data into the queue
        \return \c true if operation success, \c false otherwise

\member{dequeue}
\code
    bool dequeue (T &data)
\endcode
        Dequeues a value from the queue to \a dest.
        \return \c true if operation success, \c false otherwise (i.e. the queue is empty)

\member{push}
\code
    bool push (const T &data)
\endcode
        Synonym for the \c enqueue function

\member{pop}
\code
    bool pop (const T &data)
\endcode
        Synonym for the \c dequeue function

\member{empty}
\code
    bool empty()
\endcode
        Checks if the queue is empty

\member{clear}
\code
    void clear ()
\endcode
        Clears the queue in lock-free fashion.

\member{size}
\code
    size_t size () const
\endcode
    Returns count of items in the queue. The value returned depends on implementation of the queue. For many
    implementation the item counting facility is optional and is defined by queue's template parameter TRAITS.
    Therefore, for the item counting it may be defined dummy object (like cds::atomics::empty_item_counter)
    that do nothing (it is default behavior for queue's TRAITS template parameter); in this case \c size() method always returns 0.
    In any cases, \c size()==0 is not equal to \c empty()==true. The \c empty method is the more reliable way
    to check if the queue is empty.

*/


namespace cds {

/// Various queue implementations
/**
    The common interface of queue implementation see \ref queue_common_interface.
*/
namespace queue {

    /// Queue internal statistics. May be used for debugging or profiling
    struct queue_statistics {
        atomics::event_counter    m_EnqueueCount    ;    ///< Enqueue call count
        atomics::event_counter    m_DequeueCount    ;    ///< Dequeue call count
        atomics::event_counter    m_EnqueueRace    ;    ///< Count of enqueue race conditions encountered
        atomics::event_counter    m_DequeueRace    ;    ///< Count of dequeue race conditions encountered
        atomics::event_counter    m_AdvanceTailError;    ///< Count of "advance tail failed" events
        atomics::event_counter    m_BadTail        ;    ///< Count of events "Tail is not pointed to the last item in the queue"

        /// Register enqueue call
        void onEnqueue()                { ++m_EnqueueCount; }
        /// Register dequeue call
        void onDequeue()                { ++m_DequeueCount; }
        /// Register enqueue race event
        void onEnqueueRace()            { ++m_EnqueueRace; }
        /// Register dequeue race event
        void onDequeueRace()            { ++m_DequeueRace; }
        /// Register "advance tail failed" event
        void onAdvanceTailFailed()        { ++m_AdvanceTailError; }
        /// Register event "Tail is not pointed to last item in the queue"
        void onBadTail()                { ++m_BadTail; }
    };

    /// Empty queue statistics - does nothing. Support interface like as queue_statistics
    struct dummy_statistics {
        //@cond
        void onEnqueue()                {}
        void onDequeue()                {}
        void onEnqueueRace()            {}
        void onDequeueRace()            {}
        void onAdvanceTailFailed()        {}
        void onBadTail()                {}
        //@endcond
    };

    /// Traits struct defines types for queue implementations
    struct traits
    {
        typedef cds::backoff::empty backoff_strategy    ;    ///< Backoff schema type
        typedef atomics::empty_item_counter    item_counter_type    ;    ///< Item counter implementation. Default is atomics::empty_item_counter
        typedef dummy_statistics    statistics            ;    ///< Internal statistics implementation, default is empty (no statistics)
        typedef void                free_list_type        ;    ///< FreeList implementation (only for queues based on gc::tagged memory reclamation schema)

        /// Node deletion function
        /**
            This functor is used internally for some implementations of queue. If a queue may store
            the nodes of different types then for those queue the node deletion function must be provided.

            If it is \p void then use default GC-related node deletion function
        */
        typedef void                node_deleter_functor   ;
    };

    //@cond
    /// Normalized queue traits
    /**
        This class is for internal use only.
    */
    template <typename TRAITS>
    struct normalized_type_traits: public cds::details::void_selector<TRAITS, traits>::type
    {
        typedef typename cds::details::void_selector<TRAITS, traits>::type  original    ;   ///< Original (non-void) type traits

        /// Internal statistics type. Default is empty statistics
        typedef typename cds::details::void_selector<
            typename original::statistics,
            dummy_statistics
        >::type        statistics    ;

        /// Item counter implementation. Default is empty item counter
        typedef typename cds::details::void_selector<
            typename original::item_counter_type,
            atomics::empty_item_counter
        >::type        item_counter_type    ;

        /// Back-off strategy implementation. Default is empty back-off strategy
        typedef typename cds::details::void_selector<
            typename original::backoff_strategy,
            cds::backoff::empty
        >::type        backoff_strategy    ;
    } ;
    //@endcond


    /// Generic algorithm for clearing queue \p q in lock-free manner
    template <class Q>
    size_t generic_clear( Q& q )
    {
        typename Q::value_type dest    ;
        size_t nRemoved = 0    ;
        while ( q.dequeue( dest ) )
            ++nRemoved    ;
        return nRemoved    ;
    }

}}  // namespace cds::queue

#endif // #ifndef __CDS_QUEUE_DETAILS_QUEUE_BASE_H
