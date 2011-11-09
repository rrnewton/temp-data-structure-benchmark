/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_QUEUE_MSQUEUE_TAGGED_H
#define __CDS_QUEUE_MSQUEUE_TAGGED_H

/*
    Editions:
        2008.10.02    Maxim.Khiszinsky    Refactoring
*/

#include <cds/queue/msqueue.h>
#include <cds/gc/tagged/gc.h>

#ifdef CDS_DWORD_CAS_SUPPORTED
namespace cds {
    namespace queue {
        namespace details {

            /// details of MSQueue implementation based on gc::tagged_gc reclamation schema
            namespace msqueue_tagged {
                /// Node of MSQueue<cds::gc::tagged_gc>
                template <typename T>
                struct CDS_TAGGED_ALIGN_ATTRIBUTE Node: public gc::tagged::ContainerNode
                {
                //@cond
                    //typedef gc::tagged::ContainerNode   base_class    ;
                    typedef gc::tagged::tagged_type<Node *>     tagged_ptr  ;

                    tagged_ptr      m_Next    ;
                    T               m_data    ;

                    Node()    {}
                    Node( const T& src ): m_data( src ) {}

                    void destroy_data()
                    {
                        m_data.T::~T()   ;
                    }

                    void construct_data()
                    {
                        new( &m_data ) T  ;
                    }

                    void construct_data(const T& src)
                    {
                        new( &m_data ) T(src)  ;
                    }
                //@endcond
                };

                /// Michael's queue implementation based on on ABA-prevention tag (IBM tagged pointers) scheme
                /**
                    \par Source:
                        [1998] Maged Michael, Michael Scott "Simple, fast, and practical non-blocking
                            and blocking concurrent queue algorithms"

                    \par Template parameters:
                        \li \p T        Type of data saved in queue's node
                        \li \p TRAITS    Traits class, see QueueTraitsT
                        \li \p ALLOCATOR    The implementation of IBM free list

                    \par Notes
                        The algorithm assumes that deleted nodes are never returned back to system; however, they is pushed to the
                        free list. The pointer to the node is an aggregation contained the pointer and the tag. The tag is used for
                        prevention of the ABA problem. The tag is incremented at any time when the pointer is changed.
                        Thus, the algorithm expects that the platform supports double word CAS primitive.
                */
                template <typename T,
                    class TRAITS = traits,
                    class FREELIST= cds::gc::tagged::FreeList< T, typename TRAITS::backoff_strategy > >
                class queue_impl {
                public:
                    typedef T                value_type    ;                    ///< Type of value stored in queue (the same as \par T)

                    /// Type traits
                    struct type_traits: public TRAITS
                    {
                        typedef TRAITS    original    ;   ///< original (input) traits

                        /// FreeList implementation
                        typedef typename FREELIST::template rebind<
                            details::msqueue_tagged::Node<T>,
                            typename original::backoff_strategy
                        >::other free_list_type  ;    
                    };

                protected:
                    typedef details::msqueue_tagged::Node<T>                    node_type   ;    ///< Node type
                    typedef typename node_type::tagged_ptr                      tagged_ptr  ;    ///< Pointer type
                    typedef typename type_traits::free_list_type::node_type *   node_ptr    ;    ///< Type of pointer to node

                    tagged_ptr        m_Head        ;        ///< Head
                    tagged_ptr        m_Tail        ;        ///< Tail

                    typename type_traits::free_list_type    m_FreeList      ;    ///< Free list

                    typename type_traits::item_counter_type m_ItemCounter   ;    ///< Item counter
                    typename type_traits::statistics        m_Stat          ;    ///< Internal statistics

                protected:
                //@cond
                    node_ptr    allocNode()                    
                    { 
                        return m_FreeList.alloc()            ; 
                    }

                    node_ptr    allocNode( const T& src )    
                    { 
                        return m_FreeList.alloc( src )    ; 
                    }

                    void        freeNode( node_ptr pNode )    
                    { 
                        m_FreeList.free( pNode )            ; 
                    }

                    tagged_ptr& getHeadRef() { return m_Head; }
                    tagged_ptr& getTailRef() { return m_Tail; }
                //@endcond

                public:
                    queue_impl()
                    {
                        // create the dummy node
                        node_ptr pNode = allocNode()    ;
                        assert( pNode->m_Next.m_data == NULL )    ;
                        m_Head.m_data =
                            m_Tail.m_data = pNode    ;
                    }
                    ~queue_impl()
                    {
                        clear()    ;
                    }

                    /// Returns number of items in queue. Valid only if \p TRAITS::item_counter_type is not the cds::atomics::empty_item_counter
                    size_t    size() const
                    {
                        return m_ItemCounter    ;
                    }

                    /// Returns internal statistics
                    const typename type_traits::statistics&    getStatistics() const { return m_Stat; }

                    /// Enqueues new node with \p data to queue
                    bool enqueue( const T& data )
                    {
                        typename type_traits::backoff_strategy backoff    ;
                        node_ptr pNode = allocNode( data )    ;

                        pNode->m_Next.m_data = NULL         ;

                        tagged_ptr tail ;
                        while ( true ) {
                            tail = atomics::load<membar_acquire>( &m_Tail )    ;
                            tagged_ptr next = atomics::load<membar_acquire>( &(tail.m_data->m_Next) )    ;
                            if ( tail == atomics::load<membar_acquire>( &m_Tail ) ) {
                                if ( next.m_data == NULL ) {
                                    if ( gc::tagged::cas_tagged<membar_release>( tail.m_data->m_Next, next, pNode ))
                                        break    ;
                                }
                                else {
                                    // Следует завершить предыдущую неоконченную операцию enqueue - продвинуть m_Tail
                                    if ( gc::tagged::cas_tagged<membar_release>( m_Tail, tail, next.data() )) {
                                        m_Stat.onBadTail()  ;
                                        continue ;
                                    }
                                }
                            }
                            m_Stat.onEnqueueRace()  ;
                            backoff();
                        }

                        if ( !gc::tagged::cas_tagged<membar_release>( m_Tail, tail, pNode ))
                            m_Stat.onAdvanceTailFailed()    ;

                        ++m_ItemCounter    ;
                        m_Stat.onEnqueue()    ;

                        return true ;
                    }
                    /// Synonym for @ref enqueue
                    bool push( const T& data )    { return enqueue( data ); }

                    /// Dequeues the tail node from the queue to \p dest
                    bool dequeue( T& dest )
                    {
                        typename type_traits::backoff_strategy    backoff ;
                        tagged_ptr head    ;
                        while ( true ) {
                            head = atomics::load<membar_acquire>( &m_Head )    ;
                            tagged_ptr tail = atomics::load<membar_acquire>( &m_Tail )    ;
                            tagged_ptr next = atomics::load<membar_acquire>( &head.m_data->m_Next ) ;
                            if ( head == atomics::load<membar_acquire>( &m_Head )) {
                                if ( head.data() == tail.data() ) {
                                    if ( next.data() == NULL )        // Is queue empty?..
                                        return false    ;
                                    // Следует завершить предыдущую неоконченную операцию enqueue - продвинуть m_Tail
                                    if ( gc::tagged::cas_tagged<membar_release>( m_Tail, tail, next.data() )) {
                                        m_Stat.onBadTail()  ;
                                        continue ;
                                    }
                                }
                                else {
                                    // Важно! Сначала присваиваем, а потом исключаем из очереди
                                    dest = next.data()->m_data ;
                                    if ( gc::tagged::cas_tagged<membar_release>( m_Head, head, next.data() ) )
                                        break    ;
                                }
                            }
                            m_Stat.onDequeueRace()    ;
                            backoff();
                        }
                        freeNode( head.data() )    ;

                        --m_ItemCounter            ;
                        m_Stat.onDequeue()        ;

                        return true    ;
                    }

                    /// Synonym for @ref dequeue operation
                    bool pop( T& dest )                { return dequeue( dest ) ; }

                    /// Checks if queue is empty
                    bool empty() const
                    {
                        return m_Tail.data() == m_Head.data()    ;
                    }

                    /// Clears the queue.
                    size_t clear()
                    {
                        return generic_clear( *this )   ;
                    }

                public:    // Not thread safe methods

                    /// Non-concurrent iterator class. Not thread-safe!!! For debugging only
                    template <typename Q>
                    class NonConcurrentIterator: public concept::non_concurrent_iterator< Q >
                    {
                        //@cond
                        node_type * m_pNode            ;
                        friend class queue_impl    ;
                        //@endcond
                    protected:
                        //@cond
                        NonConcurrentIterator( node_type * pNode ): m_pNode( pNode ) {}
                        //@endcond

                    public:
                        /// Default (empty) ctor
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
                        const node_type * getNode() const { return m_pNode; }

                        /// Pre-increment
                        NonConcurrentIterator& operator ++()
                        {
                            assert( m_pNode != NULL )    ;
                            m_pNode = m_pNode->m_Next.m_data    ;
                            return *this                ;
                        }

                        /// Post-increment
                        NonConcurrentIterator operator ++(int)
                        {
                            assert( m_pNode != NULL )    ;
                            NonConcurrentIterator<Q> it( *this )        ;
                            m_pNode = m_pNode->m_Next.m_data    ;
                            return it                    ;
                        }

                        //@{
                        /// Iterator comparison
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
                        return nonconcurrent_iterator( m_Head.m_data ) ;
                    }

                    /// Returns end non-concurrent iterator
                    nonconcurrent_iterator        end()
                    {
                        node_type * pNode = m_Tail.m_data    ;
                        while ( pNode->m_Next.m_data != NULL )
                            pNode = pNode->m_Next.m_data    ;
                        return nonconcurrent_iterator( pNode );
                    }

                    /// Returns start non-concurrent const iterator
                    nonconcurrent_const_iterator    begin() const
                    {
                        return nonconcurrent_const_iterator( empty() ? m_Head : m_Head.m_Next ) ;
                    }

                    /// Returns end non-concurrent const iterator
                    nonconcurrent_const_iterator    end() const
                    {
                        node_type * pNode = m_Tail.m_data    ;
                        while ( pNode->m_Next.m_data != NULL )
                            pNode = pNode->m_Next.m_data    ;
                        return nonconcurrent_const_iterator( pNode );
                    }

                };

                //@cond none
                template <typename T, typename BACKOFF, typename FREELIST>
                struct free_list_selector {
                    typedef FREELIST    free_list_type    ;
                };

                template <typename T, typename BACKOFF>
                struct free_list_selector<T, BACKOFF, void> {
                    typedef cds::gc::tagged::FreeList< T, BACKOFF >    free_list_type    ;
                };
                //@endcond

            }   // namespace msqueue_tagged
        }    // namespace details

        /// Implementation tagged Michael & Scott queue based on tagged pointers methodology
        /**
            The FreeList implementation is taken from TRAITS::free_list_type. If TRAITS::free_list_type is void (default) then
            default free-list cds::gc::tagged::FreeList< T, TRAITS::backoff_strategy > is used

            \par Template parameters:
                \li \p T        Type of data saved in queue's node
                \li \p TRAITS    Traits class, see cds::queue::traits
                \li \p ALLOCATOR Memory allocator template (not used in tagged MSQueue)
        */
        template <typename T, class TRAITS, class ALLOCATOR >
        class MSQueue< gc::tagged_gc, T, TRAITS, ALLOCATOR >:
            public queue::details::msqueue_tagged::queue_impl< T,
                normalized_type_traits<TRAITS>,
                typename queue::details::msqueue_tagged::free_list_selector<T,
                    typename normalized_type_traits<TRAITS>::backoff_strategy,
                    typename normalized_type_traits<TRAITS>::free_list_type
                >::free_list_type
            >
        {
        public:
            typedef gc::tagged_gc                        gc_schema    ;    ///< Garbage collection schema
        };

    }    //namespace queue
}    // namespace

#endif // #ifndef CDS_DWORD_CAS_SUPPORTED

#endif    // #ifndef __CDS_QUEUE_MSQUEUE_TAGGED_H
