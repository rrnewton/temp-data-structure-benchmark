/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ORDERED_LIST_MICHAEL_LIST_HRC_H
#define __CDS_ORDERED_LIST_MICHAEL_LIST_HRC_H

/*
Editions:
0.7.0   2010.09.16 Maxim.Khiszinsky     Generalization. The implementation is rebuilded based on generic details::michael_list::implementation.
                                        details/michael_list_hrc_node.h is removed
        2010.01.16 Maxim.Khiszinsky     Declaration of Node is moved to details/michael_list_hrc_node.h
        2009.05.10 Maxim.Khiszinsky     Creation
*/

#include <cds/ordered_list/details/michael_list_node.h>
#include <cds/ordered_list/details/michael_list_hpgen.h>
#include <cds/gc/hrc/container.h>
//#include <boost/type_traits.hpp>

namespace cds {
    namespace ordered_list {

        //@cond
        namespace details { namespace michael_list {

            /// Michael's list node for Gidenstam's (\ref cds::gc::hrc_gc) memory reclamation schema
            template <typename KEY, typename VALUE, typename ALLOCATOR>
            class Node<gc::hrc_gc, KEY, VALUE, ALLOCATOR>: public gc::hrc::ContainerNodeT<Node<gc::hrc_gc, KEY, VALUE, ALLOCATOR>, ALLOCATOR>
            {
                typedef gc::hrc::ContainerNodeT<Node, ALLOCATOR>    base_class            ;    ///< Base type
            public:
                typedef KEY     key_type    ;   ///< Key type
                typedef VALUE   value_type  ;   ///< Value type
                typedef typename base_class::node_allocator         node_allocator    ;    ///< Node allocator type

                typedef    cds::details::marked_ptr< Node, 1 >            marked_node    ;    ///< Marked node type

                /// Rebind key/value type
                template <typename KEY2, typename VALUE2>
                struct rebind {
                    typedef Node<gc::hrc_gc, KEY2, VALUE2, ALLOCATOR>   other   ;   ///< Rebinding result
                };

            public:
                /// Max count of Hazard Pointer for node's cleanUp phase
                static const unsigned int   c_nMaxCleanUpHazardPtrCount = 2    ;

                // Caveats: order of fields is important!
                // The split-list implementation uses different node types, dummy node contains only the hash and no key and data field
                // Therefore, key and data must be last fields in the node
                marked_node        m_pNext ;    ///< Pointer to next node in list
                const key_type  m_key   ;   ///< Node's key
                value_type      m_value ;   ///< Node's value

            public:
                /// Default ctor
                Node() {}
                /// Node's key initializer
                Node( const key_type& key )
                    : m_key( key )
                {}
                /// Node's key and value initializer
                Node( const key_type& key, const value_type& val )
                    : m_key( key )
                    , m_value( val )
                {}

                /// Returns node's key
                const key_type& key()    { return m_key ; }

                /// Returns node's data
                value_type&     value()    { return m_value;    }

                /// Checks if the node is marked as logically deleted one
                bool isMarked() const
                {
                    return m_pNext.isMarked() ;
                }

                /// Returns next node with deletion mark cleared
                Node *  next()
                {
                    return m_pNext.ptr()    ;
                }

            protected:
                /// Implementation of @ref gc::hrc::ContainerNode::cleanUp.
                virtual void    cleanUp( gc::hrc::ThreadGC * pGC )
                {
                    gc::hrc::AutoHPArray< c_nMaxCleanUpHazardPtrCount > hpArr( *pGC )    ;
                    while ( true ) {
                        marked_node pNext = pGC->derefLink( &m_pNext, hpArr[0] ) ;
                        if ( !pNext.isNull() && pNext->m_bDeleted.template load<membar_acquire>() ) {
                            marked_node p = pGC->derefLink( &( pNext->m_pNext ), hpArr[1] ) ;
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
                        marked_node pNext ;
                        marked_node emptyNode ;
                        do {
                            pNext = m_pNext  ;
                        } while ( !pGC->CASRef( &m_pNext, pNext, emptyNode ) ) ;
                    }
                    else {
                        pGC->storeRef( &m_pNext, (Node *) NULL )    ;
                    }
                }
            } ;


            template <
                typename KEY
                , typename VALUE
                , typename PAIR_TRAITS
                , typename TRAITS
                , class ALLOCATOR
            >
            class adapter<gc::hrc_gc, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
            {
                typedef typename cds::details::void_selector<TRAITS, ordered_list::type_traits >::type  reduced_type_traits ;

            public:
                typedef details::michael_list::Node<gc::hrc_gc, KEY, VALUE, ALLOCATOR>  node_type   ;
                typedef typename node_type::marked_node marked_node ;

                class container_base: public gc::hrc::Container
                {
                    typedef typename node_type::node_allocator  node_allocator  ;
                protected:
                    /// Allocates new node
                    node_type * allocNode( const KEY& key, const VALUE& val )
                    {
                        node_allocator a    ;
                        return a.New( key, val )    ;
                    }
                    node_type * allocNode( const KEY& key )
                    {
                        node_allocator a    ;
                        return a.New( key )    ;
                    }

                    /// Frees node
                    void   freeNode( node_type * pNode )
                    {
                        static_cast<gc::hrc::ContainerNode *>(pNode)->destroy() ;
                    }
                };

                typedef gc::hrc::AutoHPGuard        auto_guard  ;
                typedef gc::hrc::AutoHPArray<3>     guard_array     ;

                typedef typename cds::details::void_selector<PAIR_TRAITS, cds::map::pair_traits< KEY, VALUE > >::type    pair_traits ;

                struct type_traits: public reduced_type_traits
                {
                    typedef reduced_type_traits     original    ;    ///< Original (non-void) type traits
                } ;

                static node_type * getAndGuard( marked_node * pSrc, marked_node& pTarget, gc::hrc::ContainerNode *& hp )
                {
                    // it is safe to use relaxed load here: derefLink already has all fences needed
                    return (pTarget = gc::hrc::ThreadGC::derefLink( pSrc, hp )).all()  ;
                }
                static node_type * getAndGuard( marked_node * pSrc, marked_node& pTarget, auto_guard& hp )
                {
                    return getAndGuard( pSrc, pTarget, hp.getHPRef() )  ;
                }

                static bool linkNode( node_type * pNewNode, marked_node * pPrev, node_type * pNext )
                {
                    marked_node next( pNext )   ;
                    gc::hrc::ThreadGC::storeRef( &(pNewNode->m_pNext), next )    ;
                    marked_node newNode( pNewNode ) ;
                    if ( !gc::hrc::ThreadGC::CASRef( pPrev, next, newNode )) {
                        marked_node pNull   ;
                        gc::hrc::ThreadGC::storeRef( &(pNewNode->m_pNext), pNull ) ;
                        return false ;
                    }
                    return true ;
                }

                static bool logicalDeletion( marked_node * p, node_type * pVal )
                {
                    return p->template cas<membar_release>( marked_node( pVal ), marked_node( pVal, true ))    ;
                }

                static bool physicalDeletion( marked_node * p, marked_node pCur, marked_node pNew )
                {
                    return gc::hrc::ThreadGC::CASRef( p, pCur, pNew ) ;
                }

                static void retirePtr( gc::hrc::ThreadGC& gc, node_type * p, gc::hrc::ContainerNode *& hp )
                {
                    gc.retireNode( p, hp ) ;
                }
            };

        }}  // namespace details::michael_list
        //@endcond

        /// Lock-free ordered single-linked list developed by Michael based on Gidenstam's memory reclamation schema.
        /**
            The implementation is based on Gidenstam's memory reclamation schema (gc::hrc_gc)

            \par Source
                [2002] Maged Michael "High performance dynamic lock-free hash tables and list-based sets"

            \par Template parameters
                \li \p KEY        type of key stored in list
                \li \p VALUE    type of value stored in list
                \li \p PAIR_TRAITS (key, value) pair traits. The default is cds::map::pair_traits <\p KEY, \p VALUE>
                \li \p TRAITS    list traits. The default is cds::ordered_list::type_traits
                \li \p ALLOCATOR node allocator. The default is \ref CDS_DEFAULT_ALLOCATOR

            \par
                Common interface: see \ref ordered_list_common_interface.
        */
        template <
            typename KEY,
            typename VALUE,
            typename PAIR_TRAITS,
            typename TRAITS,
            class ALLOCATOR
        >
        class MichaelList<
            gc::hrc_gc,
            KEY,
            VALUE,
            PAIR_TRAITS,
            TRAITS,
            ALLOCATOR
        > : public details::michael_list::implementation<
                gc::hrc_gc
                ,details::michael_list::adapter< gc::hrc_gc, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                ,ALLOCATOR
            >
        {
            //@cond
            typedef details::michael_list::implementation<
                gc::hrc_gc
                ,details::michael_list::adapter< gc::hrc_gc, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                ,ALLOCATOR
            >   implementation  ;
            //@endcond

        public:
            typedef typename implementation::gc_schema      gc_schema        ;    ///< Garbage collection schema
            typedef typename implementation::type_traits    type_traits     ;   ///< List traits

            typedef typename implementation::key_type        key_type        ;    ///< Key type
            typedef typename implementation::value_type        value_type        ;    ///< Value type
            typedef typename implementation::pair_traits    pair_traits        ;    ///< Pair traits type
            typedef typename implementation::key_traits        key_traits        ;    ///< Key traits
            typedef typename implementation::value_traits    value_traits    ;    ///< Value traits
            typedef typename implementation::key_comparator    key_comparator    ;    ///< Key comparator (binary predicate)
            typedef typename implementation::node_type      node_type       ;   ///< node type
            typedef typename node_type::node_allocator      node_allocator  ;   ///< node allocator

            /// Max count of Hazard Pointer for the list
            static const unsigned int   c_nMaxHazardPtrCount = 4 + node_type::c_nMaxCleanUpHazardPtrCount    ;   // 3 + 1 for clear


            /// Rebind type of key-value pair stored in the list
            template <
                typename KEY2,
                typename VALUE2,
                typename PAIR_TRAITS2,
                typename TRAITS2 = typename type_traits::original
            >
            struct rebind {
                typedef MichaelList<
                    gc_schema,
                    KEY2,
                    VALUE2,
                    PAIR_TRAITS2,
                    TRAITS2,
                    ALLOCATOR
                >    other ; ///< Rebinding result
            };

        public:
            MichaelList()
            {
                assert( gc::hrc::GarbageCollector::isUsed() )   ;
                assert( c_nMaxHazardPtrCount <= gc::hrc::GarbageCollector::instance().getHazardPointerCount() ) ;
            }
        };

        //@cond none
        namespace split_list {

            template < typename REGULAR_KEY, typename DUMMY_KEY, typename VALUE, typename PAIR_TRAITS, typename TRAITS, class ALLOCATOR >
            class MichaelList<gc::hrc_gc, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                : public ordered_list::MichaelList<gc::hrc_gc, REGULAR_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
            {
                typedef ordered_list::MichaelList<gc::hrc_gc, REGULAR_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR> base_class ;
            public:
                typedef typename base_class::node_type    regular_node    ;
                typedef typename regular_node::template rebind<REGULAR_KEY, char>::other  dummy_node     ;

                typedef typename base_class::node_allocator                                     regular_node_allocator  ;
                typedef typename regular_node_allocator::template rebind<dummy_node>::other     dummy_node_allocator    ;

                typedef typename base_class::marked_node        node_ptr    ;    ///< Split-ordered list support: pointer to head of the list part
                typedef typename base_class::marked_node&       node_ref    ;    ///< Split-ordered list support: argument of node type

            public:
                dummy_node *    allocDummy( const DUMMY_KEY& key )
                {
                    dummy_node_allocator a  ;
                    return a.New( key )     ;
                }

                static regular_node *  deref( node_ref refNode ) { return refNode.ptr(); }
                static regular_node *  deref( node_ptr * refNode ) { return refNode->ptr(); }
            };

            template <
                typename REGULAR_KEY,
                typename DUMMY_KEY,
                typename VALUE,
                typename PAIR_TRAITS,
                typename TRAITS,
                class ALLOCATOR
            >
            struct impl_selector< michael_list_tag<gc::hrc_gc>, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
            {
                typedef MichaelList<gc::hrc_gc, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR> type    ;
            };
        }   // namespace split_list
        //@endcond

    }    // namespace ordered_list

}    // namespace cds


#endif // #ifndef __CDS_ORDERED_LIST_MICHAEL_LIST_HRC_H
