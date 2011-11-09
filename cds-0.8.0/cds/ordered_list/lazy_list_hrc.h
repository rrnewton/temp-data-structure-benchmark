/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ORDERED_LIST_LAZY_LIST_HRC_H
#define __CDS_ORDERED_LIST_LAZY_LIST_HRC_H

#include <cds/ordered_list/details/lazy_list_hpgen.h>
#include <cds/gc/hrc/container.h>
#include <cds/concept.h>

namespace cds {
    namespace ordered_list {

        //@cond
        namespace details { namespace lazy_list {

            /// Lazy list node
            template <typename KEY, typename VALUE, typename LOCK, typename ALLOCATOR>
            class Node<gc::hrc_gc, KEY, VALUE, LOCK, ALLOCATOR>: public gc::hrc::ContainerNodeT<Node<gc::hrc_gc, KEY, VALUE, LOCK, ALLOCATOR>, ALLOCATOR>
            {
                //@cond
                typedef gc::hrc::ContainerNodeT<Node, ALLOCATOR>    base_class            ;
                //@endcond
            public:
                typedef KEY     key_type    ;   ///< Key type
                typedef VALUE   value_type  ;   ///< Value type

                typedef typename base_class::node_allocator            node_allocator    ;    ///< Node allocator type

                typedef LOCK    lock_type       ;    ///< node locker type

                /// Rebind key/value type
                template <typename KEY2, typename VALUE2, typename LOCK2=lock_type>
                struct rebind {
                    typedef Node<gc::hrc_gc, KEY2, VALUE2, LOCK2, ALLOCATOR>    other ; ///< Rebinding result
                };

            public:
                Node * volatile m_pNext        ;    ///< Pointer to next node in list
            private:
                lock_type        m_Lock        ;    ///< Node locker
                atomic<bool>    m_bMarked    ;    ///< Logical deletion mark

            public:
                // Caveats: order of fields is important!
                // The split-list implementation uses different node types, dummy node contains only the hash and no key and data field
                // Therefore, key and data must be last fields in the node
                const key_type    m_key        ;    ///< Node's key
                value_type        m_data        ;    ///< Node's data

            public:
                /// Default ctor
                Node()
                    : m_key()
                    , m_pNext( NULL )
                    , m_bMarked( false )
                {}

                /// Key ctor
                Node( const key_type& key )
                    : m_key(key)
                    , m_pNext( NULL )
                    , m_bMarked( false )
                {}

                /// Key and data ctor
                Node( const key_type& key, const value_type& data )
                    : m_key(key)
                    , m_pNext( NULL )
                    , m_bMarked( false )
                    , m_data( data )
                {}

                /// Locks the node
                void    lock()                { m_Lock.lock();    }
                /// Unlocks the node
                void    unlock()            { m_Lock.unlock();    }

                /// Checks if node is marked (logically deleted)
                bool    isMarked() const
                {
                    return m_bMarked.load<membar_acquire>()   ;
                }

                /// Mark node as logically deleted ones
                void    mark()
                {
                    m_bMarked.store<membar_release>( true ) ;
                }

                Node * next() const
                {
                    return atomics::load<membar_acquire>( &m_pNext ) ;
                }

                /// Returns node's key
                const key_type& key()    { return m_key ; }

                /// Returns node's data
                value_type&     value()    { return m_data;    }

            protected:
                /// Implementation of @ref gc::hrc::ContainerNode::cleanUp.
                virtual void    cleanUp( gc::hrc::ThreadGC * pGC )
                {
                    gc::hrc::AutoHPArray<2> hpArr( *pGC )    ;
                    while ( true ) {
                        Node * pNext = pGC->derefLink( &m_pNext, hpArr[0] ) ;
                        if ( pNext && pNext->m_bDeleted.template load<membar_acquire>() ) {
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
                virtual void    terminate( gc::hrc::ThreadGC * pGC, bool bConcurrent )
                {
                    if ( bConcurrent ) {
                        Node * pNext ;
                        Node * emptyNode = NULL ;
                        do {
                            pNext = m_pNext  ;
                        } while ( !pGC->CASRef( &m_pNext, pNext, emptyNode ) ) ;
                    }
                    else {
                        pGC->storeRef( &m_pNext, (Node *) NULL )    ;
                    }
                }
            };

            template <
                typename KEY
                , typename VALUE
                , typename PAIR_TRAITS
                , typename TRAITS
                , class ALLOCATOR
            >
            class adapter<gc::hrc_gc, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >
            {
                typedef typename cds::details::void_selector<TRAITS, ordered_list::type_traits >::type  reduced_type_traits ;

            public:
                typedef typename cds::details::void_selector<PAIR_TRAITS, cds::map::pair_traits< KEY, VALUE > >::type    pair_traits ;
                struct type_traits: public reduced_type_traits
                {
                    typedef reduced_type_traits     original    ;    ///< Original (non-void) type traits

                    // Default lock type
                    typedef typename details::lazy_list::lock_type<typename original::lock_type>::type  lock_type   ;

                } ;

                typedef Node<gc::hrc_gc, KEY, VALUE, typename type_traits::lock_type, ALLOCATOR> node_type   ;


                class container_base: public gc::hrc::Container
                {
                protected:
                    typedef typename node_type::node_allocator  node_allocator  ;

                    /// Allocates new node
                    node_type * allocNode( const KEY& key, const VALUE& val )
                    {
                        node_allocator a    ;
                        node_type * pNode = a.New( key, val )    ;
                        assert( !cds::details::isMarkedBit( pNode ))            ;
                        return pNode ;
                    }
                    node_type * allocNode( const KEY& key )
                    {
                        node_allocator a    ;
                        node_type * pNode = a.New( key )    ;
                        assert( !cds::details::isMarkedBit( pNode ))            ;
                        return pNode ;
                    }

                    /// Frees node
                    void   freeNode( node_type * pNode )
                    {
                        assert( !cds::details::isMarkedBit( pNode ))    ;
                        static_cast<gc::hrc::ContainerNode *>(pNode)->destroy() ;
                    }
                };

                typedef gc::hrc::AutoHPGuard        auto_guard  ;
                typedef gc::hrc::AutoHPArray<3>     guard_array     ;

            public:
                static node_type * getAndGuard( node_type * volatile & pSrc, node_type *& pTarget, gc::hrc::ContainerNode *& hp )
                {
                    // it is safe to use relaxed load here: derefLink already has all fences needed
                    return pTarget = gc::hrc::ThreadGC::derefLink( &pSrc, hp )  ;
                }
                static node_type * getAndGuard( node_type * volatile & pSrc, node_type *& pTarget, auto_guard& hp )
                {
                    return getAndGuard( pSrc, pTarget, hp.getHPRef() )  ;
                }

                static bool validate( node_type * pPred, node_type * pCur )
                {
                    return !pPred->isMarked() && !pCur->isMarked() && pPred->m_pNext == pCur ;
                }

                static void linkNode( node_type * pNew, node_type * pPred, node_type * pCur )
                {
                    assert( pPred->m_pNext == pCur )    ;
                    gc::hrc::ThreadGC::storeRef( &pNew->m_pNext, pCur ) ;

                    // We may use storeRef instead of xchgRef/CASRef because pPred and pCur are locked now
                    gc::hrc::ThreadGC::storeRef( &pPred->m_pNext, pNew )    ;
                }

                static void logicalRemove( node_type * pNode )
                {
                    pNode->mark()    ;
                }

                static void linkNode( node_type * pNode, node_type * pNext )
                {
                    gc::hrc::ThreadGC::storeRef( &pNode->m_pNext, pNext )   ;
                }

                static void retirePtr( gc::hrc::ThreadGC& gc, node_type * p, gc::hrc::ContainerNode *& hp )
                {
                    gc.retireNode( p, hp ) ;
                }
            };
        }}    // namespace details::lazy_list
        //@endcond

        /// Lazy list specialization for Gidenstam's reference counting (gc::hrc_gc) memory reclamation schema
        /**
            \par Template parameters
                \li \p KEY        type of key stored in list
                \li \p VALUE    type of value stored in list
                \li \p PAIR_TRAITS (key, value) pair traits. The default is cds::map::pair_traits <\p KEY, \p VALUE>
                \li \p TRAITS    list traits. The default is cds::ordered_list::type_traits
                \li \p ALLOCATOR node allocator. The default is \ref CDS_DEFAULT_ALLOCATOR.

            \par Source
                [2005] Steve Heller, Maurice Herlihy, Victor Luchangco, Mark Moir, William N. Scherer III, and Nir Shavit
                "A Lazy Concurrent List-Based Set Algorithm"

            \par
                Common interface: see \ref ordered_list_common_interface.
        */
        template <typename KEY, typename VALUE, typename PAIR_TRAITS, typename TRAITS, class ALLOCATOR >
        class LazyList<gc::hrc_gc, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
            : public details::lazy_list::implementation<
                gc::hrc_gc
                ,details::lazy_list::adapter< gc::hrc_gc, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                ,ALLOCATOR
            >
        {
            //@cond
            typedef details::lazy_list::implementation<
                gc::hrc_gc
                ,details::lazy_list::adapter< gc::hrc_gc, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                ,ALLOCATOR
            > base_class ;
            //@endcond

        protected:
            //@cond
            typedef typename base_class::node_allocator node_allocator  ;
            //@endcond

        public:
            typedef typename base_class::node_type      node_type   ;   ///< node type
            typedef typename base_class::key_type       key_type    ;   ///< key type
            typedef typename base_class::value_type     value_type  ;   ///< value type
            typedef typename base_class::pair_traits    pair_traits ;   ///< pair (key, value) traits
            typedef typename base_class::key_traits     key_traits  ;   ///< key traits
            typedef typename base_class::value_traits   value_traits;   ///< value traits
            typedef typename base_class::gc_schema      gc_schema   ;   ///< garbage collecting schema
            typedef typename base_class::type_traits    type_traits ;   ///< type traits
            typedef typename base_class::thread_gc      thread_gc   ;   ///< thread-local GC class

            /// Rebind type of key-value pair stored in the list
            template <typename KEY2, typename VALUE2, typename PAIR_TRAITS2, typename TRAITS2 = typename type_traits::original>
            struct rebind {
                typedef LazyList< gc_schema, KEY2, VALUE2, PAIR_TRAITS2, TRAITS2, ALLOCATOR >    other ; ///< Rebinding result
            };

        public:
            LazyList()
            {
                node_allocator a        ;
                base_class::m_pHead = a.New()       ;
                base_class::m_pTail = a.New()       ;
                base_class::m_pHead->incRefCount()  ;
                base_class::m_pTail->incRefCount()  ;
                gc::hrc::ThreadGC::storeRef( &base_class::m_pHead->m_pNext, base_class::m_pTail )   ;
            }

            ~LazyList()
            {
                clear()    ;
                assert( empty() ) ;

                gc::hrc::ThreadGC::storeRef( &base_class::m_pHead->m_pNext, reinterpret_cast<node_type *>(NULL) )   ;

                base_class::m_pHead->decRefCount()      ;
                base_class::m_pTail->decRefCount()      ;
                base_class::gc_base_class::getGC().retireNode( base_class::m_pHead )  ;
                base_class::gc_base_class::getGC().retireNode( base_class::m_pTail )  ;
            }

            /// Checks if the list is empty
            bool empty() const  { return base_class::empty(); }

            /// Clears the list
            void clear()
            {
                thread_gc& gc =  base_class::gc_base_class::getGC()   ;
                typename base_class::guard_array hp( gc )  ;

                while ( !empty() ) {
                    node_type * pEntry = gc::hrc::ThreadGC::derefLink( &base_class::m_pHead->m_pNext, hp[0] )   ;
                    node_type * pNext = gc::hrc::ThreadGC::derefLink( &pEntry->m_pNext, hp[1] )   ;

                    base_class::m_pHead->lock()    ;
                    pEntry->lock()    ;

                    bool const bCanRemove = pEntry->m_pNext == pNext ;
                    if ( bCanRemove ) {
                        pEntry->mark()    ;                        // logically remove
                        gc.storeRef( &( base_class::m_pHead->m_pNext ), pNext  ) ;    // physically remove
                        gc.storeRef( &(pEntry->m_pNext), reinterpret_cast<node_type *>( NULL ) )    ; // release reference to pEntry->m_pNext
                    }

                    pEntry->unlock();
                    base_class::m_pHead->unlock()    ;

                    if ( bCanRemove )
                        gc.retireNode( pEntry, hp[0] ) ; // free entry
                }
            }
        };

        //@cond none
        namespace split_list {

            template < typename REGULAR_KEY, typename DUMMY_KEY, typename VALUE, typename PAIR_TRAITS, typename TRAITS, class ALLOCATOR >
            class LazyList<gc::hrc_gc, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                : public ordered_list::LazyList<gc::hrc_gc, REGULAR_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
            {
                typedef ordered_list::LazyList<gc::hrc_gc, REGULAR_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR> base_class ;
            public:
                typedef typename base_class::node_type regular_node    ;
                typedef typename regular_node::template rebind< REGULAR_KEY, char>::other     dummy_node      ;

                typedef typename base_class::node_allocator                                     regular_node_allocator  ;
                typedef typename regular_node_allocator::template rebind<dummy_node>::other     dummy_node_allocator    ;

                typedef regular_node *    node_ptr    ;
                typedef regular_node *    node_ref    ;

            public:
                dummy_node *    allocDummy( const DUMMY_KEY& key )
                {
                    dummy_node_allocator a  ;
                    return a.New( key )     ;
                }

                static regular_node *  deref( node_ref refNode ) { return refNode; }
            };

            template <
                typename REGULAR_KEY,
                typename DUMMY_KEY,
                typename VALUE,
                typename PAIR_TRAITS,
                typename TRAITS,
                class ALLOCATOR
            >
            struct impl_selector< lazy_list_tag<gc::hrc_gc>, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
            {
                typedef LazyList<gc::hrc_gc, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR> type    ;
            };

        }   // namespace split_list
        //@endcond

    } // namespace ordered_list
} // namespace cds

#endif //  #ifndef __CDS_ORDERED_LIST_LAZY_LIST_HRC_H
