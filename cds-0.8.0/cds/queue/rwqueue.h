/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_QUEUE_RWQUEUE_H
#define __CDS_QUEUE_RWQUEUE_H

/*
    Filename: rwqueue.h
    Created 2007.03.01 by Maxim.Khiszinsky

    Source:
        [1998] Maged Michael, Michael Scott "Simple, fast, and practical non-blocking and blocking concurrent queue algorithms"

    Editions:
        2007.03.01  Maxim.Khiszinsky    Created
*/

#include <cds/queue/details/queue_base.h>
#include <cds/lock/spinlock.h>
#include <cds/details/allocator.h>

namespace cds {
    namespace queue {
        namespace details {
            /// RWQueueT node
            template <typename T>
            struct rwqueue_node {
                rwqueue_node * volatile    m_pNext        ;    ///< next item of queue
                T                        m_data        ;    ///< data of node

                //@cond
                rwqueue_node()
                    : m_pNext( NULL )
                {}

                rwqueue_node( const T& src)
                    : m_pNext( NULL )
                    , m_data( src )
                {}

                rwqueue_node *    next() { return m_pNext; }
                T&                data() { return m_data;  }
                //@endcond
            };
        }

        /// Michael & Scott blocking queue with fine-grained synchronization schema
        /*
            The queue has two diggerent locks: one for reading and one for writing.
            Therefore, one writer and one reader can simultaneously access to the queue

            \par Source:
                \li [1998] Maged Michael, Michael Scott "Simple, fast, and practical non-blocking
                    and blocking concurrent queue algorithms"

            \par Template parameters:
                \li \p T        Type of data saved in queue's node
                \li \p LOCK        Synchronization primitive. It must have methods \p lock for lock access and \p unlock to unlock access
                \li \p TRAITS   Queue traits class. Default is traits.
                \li \p ALLOCATOR Memory allocator template
        */
        template <typename T, class LOCK, class TRAITS = traits, class ALLOCATOR = CDS_DEFAULT_ALLOCATOR >
        class RWQueue
        {
        public:
            typedef T                                    value_type    ;    ///< value type
            typedef details::rwqueue_node<T>                Node        ;    ///< node type
            typedef cds::details::Allocator< Node, ALLOCATOR >    TAllocator    ;    ///< Node allocator type
            typedef TRAITS                              type_traits ;   ///< type traits
            typedef typename type_traits::statistics    statistics    ;    ///< Internal statistics type

        protected:
            typedef typename type_traits::item_counter_type    item_counter_type    ;    ///< Item counter implementation

        private:
            Node *    m_pHead    ;                ///< Pointer to head
            Node *    m_pTail    ;                ///< Pointer to tail
            mutable LOCK    m_HeadLock    ;    ///< Head access lock
            mutable LOCK    m_TailLock    ;    ///< Tail access lock

            TAllocator        m_Allocator        ;    ///< Node allocator

            item_counter_type    m_ItemCounter;    ///< Item counter
            statistics            m_Stat    ;        ///< Internal statistics

            typedef lock::Auto<LOCK>    AutoLock    ;    ///< Auto-lock type

        private:
            /// New node allocation
            Node * allocNode()
            {
                return m_Allocator.New()    ;
            }

            /// New node allocation with initialization
            Node * allocNode( const T& data )
            {
                return m_Allocator.New( data )    ;
            }

            /// Node reclamation
            void freeNode( Node * pNode )
            {
                m_Allocator.Delete( pNode )    ;
            }

        public:
            RWQueue()
            {
                Node * pNode = allocNode()    ;
                m_pHead =
                    m_pTail = pNode    ;
            }

            ~RWQueue()
            {
                clear()    ;
                assert( m_pHead == m_pTail )    ;
                freeNode( m_pHead )    ;
            }

            /// Enqueues \p data in lock-free manner. Always return \a true
            bool enqueue( const T& data )
            {
                Node * pNode = allocNode( data )    ;
                {
                    AutoLock lock( m_TailLock )    ;
                    m_pTail =
                        m_pTail->m_pNext = pNode    ;
                }
                ++m_ItemCounter     ;
                m_Stat.onEnqueue()  ;
                return true ;
            }

            /** Dequeues a value to \p dest.

                If queue is empty returns \a false, \p dest may be corrupted.
                If queue is not empty returns \a true, \p dest contains the value dequeued
            */
            bool dequeue( T& dest )
            {
                Node * pNode    ;
                {
                    AutoLock lock( m_HeadLock )    ;
                    pNode = m_pHead    ;
                    Node * pNewHead = pNode->m_pNext    ;
                    if ( pNewHead == NULL )
                        return false    ;
                    dest = pNewHead->m_data ;
                    m_pHead = pNewHead        ;
                }    // unlock here
                --m_ItemCounter     ;
                freeNode( pNode )    ;
                m_Stat.onDequeue()  ;
                return true    ;
            }

            /// Synonym for @ref enqueue
            bool push( const T& data )        { return enqueue( data ); }
            /// Synonym for \ref dequeue
            bool pop(  T& data )            { return dequeue( data ); }

            /// Checks if queue is empty
            bool empty() const
            {
                AutoLock lock( m_HeadLock )    ;
                return m_pHead->m_pNext == NULL    ;
            }

            /// Clears queue
            size_t clear()
            {
                size_t nRemoved = 0    ;
                AutoLock lockR( m_HeadLock )    ;
                AutoLock lockW( m_TailLock )    ;
                while ( m_pHead->m_pNext != NULL ) {
                    Node * pHead = m_pHead    ;
                    m_pHead = m_pHead->m_pNext    ;
                    freeNode( pHead )        ;
                    ++nRemoved                ;
                }
                return nRemoved    ;
            }

            /// Returns number of items in queue. Valid only if \p TRAITS::item_counter_type is not the cds::atomics::empty_item_counter
            size_t    size() const
            {
                return m_ItemCounter    ;
            }

        protected:

            /// Non-concurrent iterator class. Not thread-safe!!! For debugging only
            template <typename Q>
            class NonConcurrentIterator: public concept::non_concurrent_iterator< Q >
            {
                //@cond
                Node * m_pNode    ;
                friend class RWQueue    ;
                //@endcond

            protected:
                //@cond
                NonConcurrentIterator( Node * pNode ): m_pNode( pNode ) {}
                //@endcond

            public:
                /// Default ctor
                NonConcurrentIterator(): m_pNode( NULL ) {}
                /// Copy ctor
                NonConcurrentIterator( const NonConcurrentIterator<Q>& it): m_pNode( it.m_pNode ) {}

                /// Return reference to node's data
                Q& operator *()        { return m_pNode->data(); }
                /// Return pointer to node's data
                Q * operator ->()    { return &(m_pNode->data()) ;}

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
                    m_pNode = m_pNode->next()    ;
                    return *this                ;
                }

                /// Post-increment
                NonConcurrentIterator operator ++(int)
                {
                    assert( m_pNode != NULL )    ;
                    NonConcurrentIterator<Q> it( *this )        ;
                    m_pNode = m_pNode->next()    ;
                    return it                    ;
                }

                //@{
                /// Iterator comparison
                bool operator ==( const NonConcurrentIterator<Q>& it ) { return m_pNode == it.m_pNode; }
                bool operator !=( const NonConcurrentIterator<Q>& it ) { return !( *this == it ) ;}
                //@}
            };

        public:

            /// Non-concurrent iterator. Not thread-safe!!!
            typedef NonConcurrentIterator<T>            nonconcurrent_iterator        ;

            /// Non-concurrent const iterator. Not thread-safe!!!
            typedef NonConcurrentIterator<const T>        nonconcurrent_const_iterator    ;

            /// Returns start non-concurrent iterator
            nonconcurrent_iterator        begin()
            {
                return nonconcurrent_iterator( empty() ? m_pHead: m_pHead->next() ) ;
            }

            /// Returns end non-concurrent iterator
            nonconcurrent_iterator        end()
            {
                Node * pNode = m_pTail    ;
                while ( pNode->m_pNext != NULL )
                    pNode = pNode->next()    ;
                return nonconcurrent_iterator( pNode );
            }

            /// Returns start non-concurrent const iterator
            nonconcurrent_const_iterator    begin() const
            {
                return nonconcurrent_const_iterator( empty() ? m_pHead : m_pHead->next() ) ;
            }

            /// Returns end non-concurrent const iterator
            nonconcurrent_const_iterator    end() const
            {
                Node * pNode = m_pTail    ;
                while ( pNode->next() != NULL )
                    pNode = pNode->next()    ;
                return const_iterator( pNode );
            }
        };

    }    // namespace queue
}    // namespace cds

#endif //#ifndef __CDS_QUEUE_RWQUEUE_H
