/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_QUEUE_MSQUEUE_HP_IMPL_H
#define __CDS_QUEUE_MSQUEUE_HP_IMPL_H

#include <cds/queue/msqueue.h>
#include <cds/details/aligned_type.h>
#include <cds/user_setup/cache_line.h>

#include <boost/type_traits/add_pointer.hpp>

//@cond
namespace cds { namespace queue { namespace details {

    template <
        typename GC,
        typename T,
        class TRAITS,
        class ALLOCATOR
    > class msqueue_adapter ;

    template < typename ADAPTER >
    class msqueue_implementation: public ADAPTER::container_base
    {
        typedef typename ADAPTER::container_base base_class ;
    public:
        typedef typename ADAPTER::gc_schema     gc_schema   ;
        typedef typename ADAPTER::value_type    value_type  ;
        typedef typename ADAPTER::type_traits   type_traits ;

    protected:
        typedef typename ADAPTER::node_type     node_type       ;
        typedef typename ADAPTER::guard         guard           ;
        typedef typename ADAPTER::guard_array   guard_array     ;

    protected:
        typedef typename cds::details::aligned_type< atomic<node_type *>, c_nCacheLineSize >::type  aligned_node_ptr ;
        aligned_node_ptr m_pHead ;
        aligned_node_ptr m_pTail ;

        typename type_traits::item_counter_type m_ItemCounter   ;
        typename type_traits::statistics        m_Stat          ;

    public:
        msqueue_implementation()
        {
            node_type * pNode = base_class::allocNode()         ;

            m_pHead.template store<membar_relaxed>( pNode ) ;
            m_pTail.template store<membar_release>( pNode ) ;
        }

        ~msqueue_implementation()
        {
            clear() ;

            node_type * pHead = m_pHead.template load<membar_relaxed>() ;

            assert( pHead != NULL ) ;
            assert( pHead == m_pTail.template load<membar_relaxed>() )    ;

            base_class::getGC().retirePtr( pHead, type_traits::node_deleter_functor::free ) ;
            m_pHead.template store<membar_relaxed>( reinterpret_cast<node_type *>(NULL) )   ;
            m_pTail.template store<membar_relaxed>( reinterpret_cast<node_type *>(NULL) )   ;
        }

        size_t    size() const
        {
            return m_ItemCounter.value()    ;
        }

        const typename type_traits::statistics&    getStatistics() const
        {
            return m_Stat;
        }

        bool enqueue( const value_type& data )
        {
            guard grd( base_class::getGC() )  ;

            node_type * pNew = base_class::allocNode( data )    ;
            typename type_traits::backoff_strategy bkoff ;

            node_type * t    ;
            while ( true ) {
                grd = t = m_pTail.template load<membar_relaxed>()   ;
                if ( m_pTail.template load<membar_acquire>() != t ) {
                    bkoff()    ;
                    continue    ;
                }
                node_type * pNext = t->m_pNext.template load<membar_acquire>()  ;
                if ( pNext != NULL ) {
                    // Tail is misplaced, advance it
                    m_pTail.template cas<membar_release>( t, pNext ) ;
                    m_Stat.onBadTail()    ;
                    continue    ;
                }
                if ( t->m_pNext.template cas<membar_release>( reinterpret_cast<node_type *>(NULL), pNew ))
                    break    ;
                m_Stat.onEnqueueRace()    ;
                bkoff()    ;
            }
            ++m_ItemCounter    ;
            m_Stat.onEnqueue()    ;

            if ( !m_pTail.template cas<membar_acq_rel>( t, pNew ))
                m_Stat.onAdvanceTailFailed() ;
            return true ;
        }

        bool dequeue( value_type& dest )
        {
            typename guard_array::template rebind<2>::other   hpArr( base_class::getGC() )    ;

            node_type * pNext    ;
            node_type * h        ;
            typename type_traits::backoff_strategy bkoff    ;

            while ( true ) {
                h = m_pHead.template load<membar_relaxed>()    ;
                hpArr[0] = h    ;
                if ( m_pHead.template load<membar_acquire>() != h ) {
                    bkoff()    ;
                    continue    ;
                }

                node_type * t = m_pTail.template load<membar_acquire>() ;
                pNext = h->m_pNext.template load<membar_acquire>() ;
                hpArr[1] = pNext    ;
                if ( m_pHead.template load<membar_acquire>() != h )
                    continue    ;

                if ( pNext == NULL )
                    return false    ;    // empty queue

                if ( h == t ) {
                    // It is needed to help enqueue
                    m_Stat.onBadTail()            ;
                    m_pTail.template cas<membar_release>( t, pNext ) ;
                    continue    ;
                }
                if ( m_pHead.template cas<membar_release>( h, pNext ))
                    break    ;

                m_Stat.onDequeueRace()    ;
                bkoff()    ;
            }

            dest = pNext->m_data     ;
            --m_ItemCounter            ;
            m_Stat.onDequeue()        ;
            hpArr.getGC().retirePtr( h, type_traits::node_deleter_functor::free ) ;
            return true    ;
        }

        bool push( const value_type& data )
        {
            return enqueue( data );
        }

        bool pop( value_type& data )
        {
            return dequeue( data );
        }

        bool empty() const
        {
            return m_pTail.template load<membar_acquire>() == m_pHead.template load<membar_acquire>();
        }

        size_t clear()
        {
            return generic_clear( *this )   ;
        }

    protected:
        template <typename VALUE_REFERENCE>
        class nc_iterator
        {
            node_type *     m_pNode ;
            friend class    msqueue_implementation  ;

        public:
            typedef VALUE_REFERENCE     value_reference ;
            typedef typename boost::add_pointer< value_reference >  value_pointer   ;

        protected:
            nc_iterator( node_type * pNode )
                : m_pNode( pNode )
            {}

        public:
            nc_iterator()
                : m_pNode(NULL)
            {}

            nc_iterator( const nc_iterator<value_reference>& it )
                : m_pNode( it.m_pNode )
            {}

            value_reference operator *()
            {
                assert( m_pNode != NULL )   ;
                return m_pNode->data()  ;
            }

            value_pointer operator ->()
            {
                assert( m_pNode != NULL )   ;
                return &( m_pNode->data() ) ;
            }

            nc_iterator<value_reference>& operator =( const nc_iterator<value_reference>& src )
            {
                m_pNode = src.m_pNode    ;
                return *this            ;
            }

            nc_iterator& operator ++()        // Pre-increment
            {
                assert( m_pNode != NULL )    ;
                m_pNode = m_pNode->template next<membar_acquire>()    ;
                return *this                ;
            }

            nc_iterator operator ++(int)  // Post-increment
            {
                assert( m_pNode != NULL )    ;
                nc_iterator<value_reference> it( *this )        ;
                m_pNode = m_pNode->template next<membar_acquire>()    ;
                return it                    ;
            }

            bool operator ==( const nc_iterator<value_reference>& it ) { return m_pNode == it.m_pNode; }
            bool operator !=( const nc_iterator<value_reference>& it ) { return !( *this == it ) ;}
        };

    public:
        typedef nc_iterator<value_type&>        nonconcurrent_iterator          ;
        typedef nc_iterator<const value_type&>  nonconcurrent_const_iterator    ;

        nonconcurrent_iterator        nc_begin()
        {
            return nonconcurrent_iterator( empty() ? m_pHead: m_pHead->next() ) ;
        }

        nonconcurrent_iterator        nc_end()
        {
            node_type * pNode = m_pTail.template load<membar_acquire>() ;
            node_type * p ;
            while ( ( p = pNode->template next<membar_acquire>()) != NULL )
                pNode = p   ;
            return nonconcurrent_iterator( pNode );
        }

        nonconcurrent_const_iterator    nc_begin() const
        {
            return nonconcurrent_const_iterator( empty() ? m_pHead.template load<membar_acquire>() : m_pHead->template next<membar_acquire>() ) ;
        }

        nonconcurrent_const_iterator    nc_end() const
        {
            node_type * pNode = m_pTail.template load<membar_acquire>()   ;
            node_type * p ;
            while ( ( p = pNode->template next<membar_acquire>()) != NULL )
                pNode = p   ;
            return const_iterator( pNode );
        }
    };


}}} // namespace cds::queue::details
//@endcond

#endif  // __CDS_QUEUE_MSQUEUE_HP_IMPL_H
