/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_QUEUE_LMSQUEUE_HP_IMPL_H
#define __CDS_QUEUE_LMSQUEUE_HP_IMPL_H

/*
    Editions:
        2010.10.14 0.7.0 Maxim Khiszinsky   Created, moved from lmsqueue_hzp.h and generalized
*/

#include <cds/queue/lmsqueue.h>
#include <cds/details/allocator.h>
#include <cds/details/markptr.h>
#include <cds/details/aligned_type.h>
#include <cds/user_setup/cache_line.h>

//@cond
namespace cds { namespace queue { namespace details {

    namespace lmsqueue_hp {

        /// Dummy node for LMSQueue<gc::hzp_gc> queue
        /**
            The lowest bit of next pointer contains dummy flag: if the bit is 1 the node is dummy,
            otherwise, it is regular node.
        */
        class dummy_node {
        //@cond
        protected:
            cds::details::marked_ptr<dummy_node, 1>  m_pNext ;
            atomic<dummy_node *>                     m_pPrev ;
        public:
            dummy_node()
                : m_pNext( NULL, true )
                , m_pPrev( NULL )
            {}

            bool isDummy() const { return m_pNext.isMarked(); }

            dummy_node * next(memory_order order)
            {
                return m_pNext.load(order).ptr();
            }
            template <typename ORDER>
            dummy_node * next()
            {
                return m_pNext.load<ORDER>().ptr();
            }
            dummy_node * next( dummy_node * p, memory_order order )
            {
                m_pNext.store( p, isDummy(), order )    ;
                return p    ;
            }
            template <typename ORDER>
            dummy_node * next( dummy_node * p )
            {
                m_pNext.store<ORDER>( p, isDummy() )    ;
                return p    ;
            }

            dummy_node * prev(memory_order order)
            {
                return m_pPrev.load(order)    ;
            }
            template <typename ORDER>
            dummy_node * prev()
            {
                return m_pPrev.load<ORDER>() ;
            }
            dummy_node * prev( dummy_node * p, memory_order order )
            {
                m_pPrev.store( p, order )   ;
                return p    ;
            }
            template <typename ORDER>
            dummy_node * prev( dummy_node * p)
            {
                m_pPrev.store<ORDER>( p )   ;
                return p    ;
            }
        //@endcond
        };

        /// Regular node of LMSQueue<gc::hzp_gc> queue. T is value type that the queue contains
        template <class T>
        class regular_node: public dummy_node
        {
        //@cond
            T       m_data  ;

        public:
            regular_node()
            {
                m_pNext.unmark() ;
            }

            regular_node( const T& src )
                : m_data( src )
            {
                m_pNext.unmark() ;
            }

            dummy_node * next(memory_order order)                    { return dummy_node::next(order) ;   }
            template <typename ORDER>
            dummy_node * next()                                      { return dummy_node::next<ORDER>() ; }
            dummy_node * next( dummy_node * p, memory_order order )   { return dummy_node::next(p, order); }
            template <typename ORDER>
            dummy_node * next( dummy_node * p)                        { return dummy_node::next<ORDER>(p); }

            dummy_node * prev(memory_order order)                    { return dummy_node::prev(order) ;   }
            template <typename ORDER>
            dummy_node * prev()                                      { return dummy_node::prev<ORDER>() ; }
            dummy_node * prev( dummy_node * p, memory_order order )   { return dummy_node::prev(p, order); }
            template <typename ORDER>
            dummy_node * prev( dummy_node * p)                        { return dummy_node::prev<ORDER>(p); }

            const T& data() const { return m_data; }
        //@endcond
        };
    } // namespace lmsqueue_hp


    template <
        typename GC,
        typename T,
        class TRAITS,
        class ALLOCATOR
    > class lmsqueue_adapter ;

    template <class ADAPTER>
    class lmsqueue_implementation: public ADAPTER::container_base
    {
        typedef typename ADAPTER::container_base base_class ;
    public:
        typedef typename ADAPTER::gc_schema     gc_schema   ;
        typedef typename ADAPTER::value_type    value_type  ;
        typedef typename ADAPTER::type_traits   type_traits ;

    protected:
        typedef typename ADAPTER::node_type     node_type       ;
        typedef lmsqueue_hp::dummy_node         dummy_node      ;
        typedef typename ADAPTER::guard         guard           ;
        typedef typename ADAPTER::guard_array   guard_array     ;

        typedef typename base_class::node_allocator::template rebind<dummy_node>::other dummyNode_allocator ;   ///< Type of allocator for dummy nodes

    protected:
        typedef typename cds::details::aligned_type< atomic<dummy_node *>, c_nCacheLineSize >::type  aligned_node_ptr ;
        aligned_node_ptr m_pTail ;   ///< Pointer to tail node
        aligned_node_ptr m_pHead ;   ///< Pointer to head node

        typename type_traits::item_counter_type m_ItemCounter   ;   ///< Item counter
        typename type_traits::statistics        m_Stat          ;   ///< Internal statistics

        dummyNode_allocator     m_DummyNodeAllocator    ;   ///< Dummy node allocator

        protected:
            /// Allocates new regular node from the heap
            node_type * allocNode( const value_type& data )
            {
                return base_class::allocNode( data ) ;
            }

            /// Allocates new dummy node from the heap
            dummy_node * allocDummyNode()
            {
                return m_DummyNodeAllocator.New() ;
            }

            /// Frees (deletes) the node \p pNode.
            /**
                This function can free dummy and regular nodes.
            */
            void        freeNode( dummy_node * pNode )
            {
                if ( pNode->isDummy())
                    m_DummyNodeAllocator.Delete( pNode ) ;
                else
                    base_class::freeNode( static_cast<node_type *>( pNode )) ;
            }

            /// HZP-schema helper function to free retired dummy node
            static void freeRetiredDummyNode( dummy_node * pNode )
            {
                dummyNode_allocator    alloc    ;
                alloc.Delete( pNode )           ;
            }

            /// Retires (places to deleting list of HZP GC) \p pNode.
            /**
                This function selects dummy and regular nodes to free them in right manner
            */
            void        retireNode( dummy_node * pNode )
            {
                if ( pNode->isDummy() )
                    base_class::getGC().retirePtr( pNode, freeRetiredDummyNode ) ;
                else
                    base_class::getGC().retirePtr( static_cast<node_type *>( pNode ), type_traits::node_deleter_functor::free ) ;
            }

            /// Helper function for optimistic queue. Corrects \p prev pointer of queue's nodes if it is needed
            void fixList( dummy_node * pTail, dummy_node * pHead )
            {
                // pTail and pHead is guarded by hazard pointer
                dummy_node * pCurNode         ;
                dummy_node * pCurNodeNext     ;
                dummy_node * pNextNodePrev    ;

                typename guard_array::template rebind<3>::other hp( base_class::getGC() )   ;

                pCurNode = pTail        ;
                while ( (pHead == m_pHead.load<membar_acquire>()) && (pCurNode != pHead) ) { // While not at head
                    while ( true ) {
                        hp[0] = pCurNodeNext = pCurNode->next<membar_relaxed>() ;
                        if ( pCurNodeNext == pCurNode->next<membar_acquire>() )
                            break   ;
                    }
                    while ( true ) {
                        hp[1] = pNextNodePrev = pCurNodeNext->prev<membar_relaxed>()   ;
                        if ( pNextNodePrev == pCurNodeNext->prev<membar_acquire>() )
                            break   ;
                    }
                    if ( pNextNodePrev != pCurNode )            // Ptr don’t equal?
                        pCurNodeNext->prev<membar_release>( pCurNode )     ;    // Fix
                    hp[2] = pCurNode = pCurNodeNext        ;    // Advance curNode
                }
            }

    public:
        /// Constructor creates empty queue
        lmsqueue_implementation()
        {
            dummy_node * p = allocDummyNode() ;  ;
            m_pTail.store<membar_relaxed>( p )    ;
            m_pHead.store<membar_relaxed>( p )    ;
        }

        ~lmsqueue_implementation()
        {
            clear() ;
            assert( m_pHead.load<membar_relaxed>() == m_pTail.load<membar_relaxed>() )   ;
            assert( m_pHead.load<membar_relaxed>() != NULL )    ;
            assert( m_pHead.load<membar_relaxed>()->isDummy() ) ;
            freeNode( m_pHead.load<membar_relaxed>() )  ;
        }

        /// Enqueues \p data in lock-free manner. Always return \a true
        bool enqueue( const value_type& data )
        {
            typename type_traits::backoff_strategy bkoff ;
            guard hpGuard( base_class::getGC() )    ;

            node_type * pNew = allocNode( data ) ;
            dummy_node * pTail ;
            while( true ) {
                hpGuard = pTail = m_pTail.load<membar_acquire>() ;     // Read the tail
                if ( pTail != m_pTail.load<membar_acquire>() ) {
                    bkoff()     ;
                    continue    ;
                }

                pNew->template next<membar_release>( pTail ) ;                   // Set node’s next ptr
                if ( m_pTail.cas<membar_release>( pTail, static_cast<dummy_node *>( pNew ) ) ) {   // Try to CAS the tail
                    pTail->prev<membar_release>( pNew )     ;           // Success, write prev
                    ++m_ItemCounter         ;
                    break ;                             // Enqueue done!
                }
                bkoff() ;
            }
            return true ;
        }

           /// Dequeues a value to \p dest.
        /**
            If queue is empty returns \a false, \p dest is unchanged
            If queue is not empty returns \a true, \p dest contains the value dequeued
        */
        bool dequeue( value_type& dest )
        {
            typename guard_array::template rebind<3>::other hpArr( base_class::getGC() )   ;

            dummy_node * pTail ;
            dummy_node * pHead ;
            dummy_node * pFirstNodePrev ;

            while ( true ) { // Try till success or empty
                while ( true ) {
                    hpArr[0] = pHead = m_pHead.load<membar_relaxed>() ;    // Read the head
                    hpArr[1] = pTail = m_pTail.load<membar_relaxed>() ;    // Read the tail
                    if ( pHead == m_pHead.load<membar_acquire>() && pTail == m_pTail.load<membar_acquire>() )
                        break;
                }
                assert( pHead != NULL )    ;

                if ( pHead == m_pHead.load<membar_acquire>() && pTail == m_pTail.load<membar_acquire>() ) { // Check consistency
                    if ( !pHead->isDummy() ) { // Head is dummy?
                        hpArr[2] = pFirstNodePrev = pHead->prev<membar_relaxed>() ; // Read first node prev
                        if ( pFirstNodePrev != pHead->prev<membar_acquire>() )
                            continue ;

                        if ( pTail != pHead) { // More than 1 node?
                            if ( pFirstNodePrev == NULL || pFirstNodePrev->next<membar_acquire>() != pHead ) {
                                fixList( pTail, pHead)  ;
                                continue ; // Re-iterate
                            }
                        }
                        else { // Last node in queue
                            dummy_node * pDummy = allocDummyNode()   ;
                            pDummy->next<membar_release>( pTail ) ;
                            if ( m_pTail.cas<membar_release>( pTail, pDummy )) { // CAS tail
                                pHead->prev<membar_release >( pDummy ) ;
                            }
                            else {      // CAS failed
                                freeNode( pDummy ) ;
                            }
                            continue ; // Re-iterate
                        }
                        if ( m_pHead.cas<membar_release>( pHead, pFirstNodePrev )) {
                            assert( !pHead->isDummy() )  ;
                            dest = static_cast<node_type *>( pHead )->data()    ;   // get node's data
                            --m_ItemCounter         ;
                            retireNode( pHead )     ;   // free dequeued node
                            return true             ;   // Dequeue done!
                        }
                    }
                    else {                                  // Head points to dummy
                        if ( pTail == pHead ) {             // Tail points to dummy?
                            return false    ;               // Empty queue, done!
                        }
                        else {                              // Need to skip dummy
                            hpArr[2] = pFirstNodePrev = pHead->prev<membar_relaxed>() ; // Read first node prev
                            if ( pFirstNodePrev != pHead->prev<membar_acquire>() )
                                continue ;

                            if ( pFirstNodePrev == NULL || pFirstNodePrev->next<membar_acquire>() != pHead ) { // Tags not equal?
                                fixList( pTail, pHead);
                                continue;                   // Re-iterate
                            }
                            if ( m_pHead.cas<membar_release>( pHead, pFirstNodePrev ))  // Skip dummy
                                retireNode( pHead ) ;
                        }
                    }
                }
            }
        }

        /// Synonym for @ref enqueue
        bool push( const value_type& data )
        {
            return enqueue( data );
        }

        /// Synonym for \ref dequeue
        bool pop( value_type& data )
        {
            return dequeue( data );
        }

        /// Clears queue in lock-free manner. Return the number of removed items
        size_t clear()
        {
            return generic_clear( *this )   ;
        }

        /// Checks if queue is empty
        bool empty() const
        {
            dummy_node * pTail = m_pTail.load<membar_acquire>() ;
            return pTail == m_pHead.load<membar_acquire>() && pTail->isDummy();
        }

        /// Returns number of items in queue. Valid only if \p TRAITS::item_counter_type is not the cds::atomics::empty_item_counter
        size_t    size() const
        {
            return m_ItemCounter.value()    ;
        }

        /// Returns internal statistics
        const typename type_traits::statistics&    getStatistics() const
        {
            return m_Stat;
        }
    };

}}} // namespace cds::queue::details
//@endcond


#endif // ifndef __CDS_QUEUE_LMSQUEUE_HP_IMPL_H