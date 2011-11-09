/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_QUEUE_MSQUEUE_HRC_H
#define __CDS_QUEUE_MSQUEUE_HRC_H

/*
    Editions:
        2008.03.30    cds  Created
        2008.10.03  cds  Refactoring: specialization of MSQueue
*/

#include <cds/queue/msqueue.h>
#include <cds/gc/hrc/container.h>

namespace cds {
    namespace queue {

        /// Michael & Scott lock-free queue implementation based on Gidenstam's memory reclamation schema (HRC)
        /**
            \par Source:
                \li [2006] A.Gidenstam "Algorithms for synchronization and consistency
                        in concurrent system services", Chapter 5 "Lock-Free Memory Reclamation"
                        Thesis for the degree of Doctor    of Philosophy

            \par Template parameters:
                \li \p T        Type of data saved in queue's node
                \li \p TRAITS    Traits class (see HRCQueueTraits)
                \li \p ALLOCATOR Memory allocator template. This implementation assumes that memory allocator is a wrapper of global
                object (singleton)

                This algorithm is the integration of Michael's hazard pointer scheme and reference counting method. Hazard pointers
                are used for guaranteeing the safety of local references and reference counts for guaranteeing the safety
                of internal links in the data structure. The reference count of each node should indicate the number of
                globally accessible links that reference that node. The reference count mechanism allows to introduce
                concurrent iterator concept into queue implementation.
        */
        template <typename T, class TRAITS, class ALLOCATOR >
        class MSQueue< gc::hrc_gc, T, TRAITS, ALLOCATOR >: public gc::hrc::Container
        {
            typedef gc::hrc::Container        base_class    ;    ///< Base type

        protected:
            typedef cds::gc::hrc::ThreadGC        TThreadGC    ;    ///< Memory allocation driver
            /// Queue node
            struct Node: public gc::hrc::ContainerNodeT<Node, ALLOCATOR>
            {
                typedef gc::hrc::ContainerNodeT<Node, ALLOCATOR>    base_class            ;    ///< Base type
                typedef typename base_class::node_allocator            TNodeAllocator    ;    ///< Node allocator type

                Node * volatile m_pNext    ;    ///< pointer to next node
                T                m_data    ;    ///< node's data

                /// Default ctor
                Node()
                    : m_pNext( NULL )
                {}

                /// Ctor to assign \p data to the node
                Node( const T& data )
                    : m_pNext( NULL ),
                    m_data( data )
                {}

            private:
                /// Implementation of @ref gc::hrc::ContainerNode::cleanUp.
                virtual void    cleanUp( TThreadGC * pGC )
                {
                    gc::hrc::AutoHPArray<2> hpArr( *pGC )    ;
                    while ( true ) {
                        Node * pNext = pGC->derefLink( &m_pNext, hpArr[0] ) ;
                        if ( pNext != NULL && pNext->m_bDeleted.template load<membar_acquire>() ) {
                            Node * p = pGC->derefLink( &( pNext->m_pNext ), hpArr[1] ) ;
                            pGC->CASRef( &m_pNext, pNext, p )    ;
                            continue    ;
                        }
                        else {
                            break    ;
                        }
                    }
                }

                /// Implementation of @ref gc::hrc::ContainerNode::terminate
                virtual void    terminate( TThreadGC * pGC, bool bConcurrent )
                {
                    if ( bConcurrent ) {
                        Node * pNext ;
                        do {
                            pNext = m_pNext  ;
                        } while ( !pGC->CASRef( &m_pNext, pNext, (Node *) NULL ) ) ;
                    }
                    else {
                        pGC->storeRef( &m_pNext, (Node *) NULL )    ;
                    }
                }
            };
        public:
            typedef gc::hrc_gc                        gc_schema    ;    ///< Garbage collection schema
            typedef normalized_type_traits<TRAITS>    type_traits    ;    ///< Queue traits
            typedef T                                value_type    ;    ///< Type of value saved in queue

            static const size_t        m_nHazardPointerCount = 3    ; ///< Maximum hazard pointer count required for queue algorithm

        protected:
            Node * volatile     m_pHead    ;        ///< Queue's head
            Node * volatile        m_pTail ;        ///< Queue's tail

            typename type_traits::item_counter_type    m_ItemCounter;    ///< Item counter
            typename type_traits::statistics        m_Stat    ;        ///< Internal statistics

        protected:
            /// Allocates a node
            Node *        allocNode()
            {
                typename Node::TNodeAllocator a    ;
                return a.New()    ;
            }

            /// Allocates a node and initialize it
            Node *        allocNode( const T& data )
            {
                typename Node::TNodeAllocator a    ;
                return a.New( data )    ;
            }

            /// Returns reference to head node. For VC and GCC compatibility
            Node * volatile &    getHeadRef()    { return m_pHead; }

            /// Returns reference to tail node. For VC and GCC compatibility
            Node * volatile &    getTailRef()    { return m_pTail; }

        public:
            /// Constructs empty queue
            MSQueue()
                : m_pHead( NULL ),
                m_pTail( NULL )
            {
                assert( m_nHazardPointerCount <= gc::hrc::GarbageCollector::instance().getHazardPointerCount() )    ;

                Node * pNode = allocNode()        ;
                TThreadGC& gc = getGC()    ;
                gc.storeRef( &m_pHead, pNode )    ;
                gc.storeRef( &m_pTail, pNode )    ;
            }

            /// Clears queue and destruct
            ~MSQueue()
            {
                clear()    ;
                assert( m_pTail == m_pHead )    ;
                assert( m_pHead != NULL )    ;
                assert( m_pHead->getRefCount() >= 2 )   ;

                m_pHead->decRefCount()          ;
                m_pHead->decRefCount()          ;
                getGC().retireNode( m_pHead )    ;
            }

            /// Returns number of items in queue. Valid only if \p TRAITS::item_counter_type is not the cds::atomics::empty_item_counter
            size_t    size() const
            {
                return m_ItemCounter.value()    ;
            }

            /// Returns internal statistics
            const typename type_traits::statistics&    getStatistics() const { return m_Stat; }

            /// Enqueues \p data to queue. Always returns \a true
            bool enqueue( const T& data )
            {
                typename type_traits::backoff_strategy backoff    ;
                Node * pNode = allocNode( data )    ;

                gc::hrc::AutoHPArray<3> hpArr( base_class::getGC() )    ;
                hpArr.set( 0, pNode ) ;

                Node * pOld = hpArr.getGC().derefLink( &m_pTail, hpArr[1] )    ;
                Node * pPrev = pOld    ;
                while ( true ) {

                    while ( true ) {
                        Node * p = hpArr.getGC().derefLink( &( pPrev->m_pNext), hpArr[2] ) ;
                        if ( p == NULL )
                            break   ;
                        pPrev = p   ;
                        m_Stat.onBadTail()    ;
                    }
                    /*
                    while ( pPrev->m_pNext != NULL ) {
                        pPrev = hpArr.getGC().derefLink( &( pPrev->m_pNext), hpArr[2] ) ;
                        m_Stat.onBadTail()    ;
                    }
                    */

                    if ( hpArr.getGC().CASRef( &( pPrev->m_pNext ), (Node *) NULL, pNode ) )
                        break ;
                    m_Stat.onEnqueueRace()    ;
                    backoff()    ;
                }
                ++m_ItemCounter    ;
                m_Stat.onEnqueue()    ;
                if ( !hpArr.getGC().CASRef( &m_pTail, pOld, pNode ))
                    m_Stat.onAdvanceTailFailed() ;
                return true    ;
            }

            /** Dequeues a value to \p dest. If queue is empty returns \a false, \p dest is unchanged.
                If queue is not empty returns \a true, \p dest contains the value dequeued
            */
            bool dequeue( T& dest )
            {
                gc::hrc::AutoHPArray<2> hpArr( base_class::getGC() )    ;
                Node * pNode    ;
                Node * pNext    ;
                typename type_traits::backoff_strategy backoff ;

                while ( true ) {
                    pNode = hpArr.getGC().derefLink( &m_pHead, hpArr[0] )    ;
                    pNext = hpArr.getGC().derefLink( &( pNode->m_pNext ), hpArr[1] ) ;
                    if ( pNext == NULL ) {
                        hpArr.getGC().releaseRef( hpArr[0] )    ;
                        return false        ;
                    }

                    if ( hpArr.getGC().CASRef( &m_pHead, pNode, pNext ))
                        break    ;

                    m_Stat.onDequeueRace()    ;
                    backoff()            ;
                }
                --m_ItemCounter            ;
                m_Stat.onDequeue()        ;

                hpArr.getGC().retireNode( pNode, hpArr[0] )    ;
                dest = pNext->m_data        ;
                return true            ;
            }

            /// Synonym for @ref enqueue
            bool push( const T& data )        { return enqueue( data ); }
            /// Synonym for  \ref dequeue
            bool pop( T& data )                { return dequeue( data ); }

            /// Checks if queue is empty
            bool empty() const                { return m_pTail == m_pHead; }

            /// Clears queue in lock-free manner. Return the number of removed items
            size_t clear()
            {
                return generic_clear( *this )   ;
            }

        public:    // Not thread safe methods
            /// Non-concurrent iterator class. Not thread-safe!!! For debugging only
            template <typename Q>
            class NonConcurrentIterator: public concept::non_concurrent_iterator< Q >
            {
                typedef concept::non_concurrent_iterator< Q >    base_class    ;    ///< Base class
                Node * m_pNode    ;            ///< The node pointed by iterator
                friend class MSQueue    ;

            protected:
                /// Ctor
                NonConcurrentIterator( Node * pNode ): m_pNode( pNode ) {}

            public:
                /// Default ctor
                NonConcurrentIterator(): m_pNode( NULL ) {}
                /// Copy ctor
                NonConcurrentIterator( const NonConcurrentIterator<Q>& it): m_pNode( it.m_pNode ) {}

                /// Return reference to node's data
                Q& operator *()        { return m_pNode->m_data; }
                /// Return pointer to node's data
                Q * operator ->()    { return &(m_pNode->m_data) ;}

                /// Assignment operator
                NonConcurrentIterator& operator =( const NonConcurrentIterator<Q>& src )
                {
                    m_pNode = src.m_pNode    ;
                    return *this            ;
                }

                /// Returns pointer to node. Debugging method
                const Node * getNode() const { return m_pNode; }

                /// Pre-increment
                NonConcurrentIterator& operator ++()
                {
                    assert( m_pNode != NULL )    ;
                    m_pNode = m_pNode->m_pNext    ;
                    return *this                ;
                }

                /// Post-increment
                NonConcurrentIterator operator ++(int)
                {
                    assert( m_pNode != NULL )    ;
                    NonConcurrentIterator<Q> it( *this )        ;
                    m_pNode = m_pNode->m_pNext    ;
                    return it                    ;
                }

                //@{
                /// Iterator comparision
                bool operator ==( const NonConcurrentIterator<Q>& it ) { return m_pNode == it.m_pNode; }
                bool operator !=( const NonConcurrentIterator<Q>& it ) { return !( *this == it ) ;}
                //@}

            };

            /// Non-concurrent iterator. Not thread-safe!!!
            typedef NonConcurrentIterator<T>            nonconcurrent_iterator        ;

            /// Non-concurrent const iterator. Not thread-safe!!!
            typedef NonConcurrentIterator<const T>        nonconcurrent_const_iterator    ;

            /// Returns start non-concurrent iterator
            nonconcurrent_iterator        begin()
            {
                return nonconcurrent_iterator( empty() ? m_pHead: m_pHead->m_pNext ) ;
            }

            /// Returns end non-concurrent iterator
            nonconcurrent_iterator        end()
            {
                Node * pNode = m_pTail    ;
                while ( pNode->m_pNext != NULL )
                    pNode = pNode->m_pNext    ;
                return nonconcurrent_iterator( pNode );
            }

            /// Returns start non-concurrent const iterator
            nonconcurrent_const_iterator    begin() const
            {
                return nonconcurrent_const_iterator( empty() ? m_pHead : m_pHead->m_pNext ) ;
            }

            /// Returns end non-concurrent const iterator
            nonconcurrent_const_iterator    end() const
            {
                Node * pNode = m_pTail    ;
                while ( pNode->m_pNext != NULL )
                    pNode = pNode->m_pNext    ;
                return nonconcurrent_const_iterator( pNode );
            }
        };


    } // namespace queue
} // namespace cds

#endif    // #ifndef __CDS_QUEUE_MSQUEUE_HRC_H
