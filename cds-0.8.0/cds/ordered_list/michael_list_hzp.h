/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ORDERED_LIST_MICHAEL_LIST_HZP_H
#define __CDS_ORDERED_LIST_MICHAEL_LIST_HZP_H

/*
    Editions:
0.7.0   2010.09.16 Maxim.Khiszinsky     Generalization. The implementation is rebuilded based on generic details::michael_list::implementation.
                                        details/michael_list_hzp_node.h is removed
        2010.01.01 Maxim.Khiszinsky     Declaration of Node is moved to details/michael_list_hzp_node.h
        2007       Maxim.Khiszinsky     Creation
*/

#include <cds/ordered_list/details/michael_list_node.h>
#include <cds/ordered_list/details/michael_list_hpgen.h>
#include <cds/gc/hzp/hp_object.h>

namespace cds {
    namespace ordered_list {

        //@cond
        namespace details { namespace michael_list {

            template <
                typename KEY
                , typename VALUE
                , typename PAIR_TRAITS
                , typename TRAITS
                , class ALLOCATOR
            >
            class adapter<gc::hzp_gc, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
            {
                typedef typename cds::details::void_selector<TRAITS, ordered_list::type_traits >::type   reduced_type_traits ;
            public:
                typedef details::michael_list::Node<gc::hzp_gc, KEY, VALUE, ALLOCATOR>  node_type   ;
                typedef typename node_type::marked_node marked_node ;
                typedef gc::hzp::AutoHPGuard        auto_guard  ;
                typedef gc::hzp::AutoHPArray<3>     guard_array ;

                typedef typename cds::details::void_selector<PAIR_TRAITS, cds::map::pair_traits< KEY, VALUE > >::type    pair_traits ;

            private:
                typedef gc::hzp::Container< node_type, ALLOCATOR >  gc_container ;

            public:
                class container_base    ;
                struct type_traits: public reduced_type_traits
                {
                    typedef reduced_type_traits  original    ;   ///< Original (non-void) type traits

                    /// Functor to delete retired node
                    typedef typename cds::details::void_selector<
                        typename original::node_deleter_functor,
                        typename container_base::deferral_node_deleter
                    >::type                 node_deleter_functor ;
                } ;

                class container_base: public gc_container
                {
                protected:
                    /// Allocate new node
                    node_type * allocNode( const KEY& key, const VALUE& val )
                    {
                        return gc_container::allocNode( key, val )    ;
                    }
                    node_type * allocNode( const KEY& key )
                    {
                        return gc_container::allocNode( key )    ;
                    }
                    /// Free node
                    void   freeNode( node_type * pNode )
                    {
                        type_traits::node_deleter_functor::free( pNode )    ;
                    }
                };

                static node_type * getAndGuard( marked_node * pSrc, marked_node& pTarget, gc::hzp::HazardPtr& hp )
                {
                    pTarget = *pSrc     ;
                    hp = pTarget.ptr()    ;
                    return pSrc->template load<membar_acquire>().all() ;
                }
                static node_type * getAndGuard( marked_node * pSrc, marked_node& pTarget, auto_guard& hp )
                {
                    return getAndGuard( pSrc, pTarget, hp.getHPRef() )  ;
                }

                static bool linkNode( node_type * pNewNode, marked_node * pPrev, node_type * pNext )
                {
                    pNewNode->m_pNext.template store<membar_relaxed>( pNext, false ) ;
                    return pPrev->template cas<membar_release>( marked_node( pNext ), marked_node( pNewNode ) ) ;
                }

                static bool logicalDeletion( marked_node * p, node_type * pVal )
                {
                    return p->template cas<membar_release>( marked_node( pVal ), marked_node( pVal, true ))    ;
                }

                static bool physicalDeletion( marked_node * p, marked_node pCur, marked_node pNew )
                {
                    return p->template cas<membar_release>( pCur, pNew )   ;
                }

                static void retirePtr( gc::hzp::ThreadGC& gc, node_type * p, gc::hzp::HazardPtr& /*hp*/ )
                {
                    gc.retirePtr( p, type_traits::node_deleter_functor::free )  ;
                }
            };

        }}  // namespace details::michael_list
        //@endcond

        /// Lock-free ordered single-linked list developed by Michael based on Hazard Pointer reclamation schema.
        /**
            The implementation is based on Hazard Pointers memory reclamation schema (gc::hzp_gc)

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
            gc::hzp_gc,
            KEY,
            VALUE,
            PAIR_TRAITS,
            TRAITS,
            ALLOCATOR
        > : public details::michael_list::implementation<
                gc::hzp_gc
                ,details::michael_list::adapter< gc::hzp_gc, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                ,ALLOCATOR
            >
        {
            //@cond
            typedef details::michael_list::implementation<
                gc::hzp_gc
                ,details::michael_list::adapter< gc::hzp_gc, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                ,ALLOCATOR
            >   implementation  ;
            //@endcond

        public:
            /// Max count of Hazard Pointer for the list
            static const unsigned int   c_nMaxHazardPtrCount = 4    ; // 3 + 1 for clear

            typedef typename implementation::gc_schema      gc_schema        ;    ///< Garbage collection schema
            typedef typename implementation::type_traits    type_traits     ;   ///< List traits

            typedef typename implementation::key_type        key_type        ;    ///< Key type
            typedef typename implementation::value_type        value_type        ;    ///< Value type
            typedef typename implementation::pair_traits    pair_traits        ;    ///< Pair traits type
            typedef typename implementation::key_traits        key_traits        ;    ///< Key traits
            typedef typename implementation::value_traits    value_traits    ;    ///< Value traits
            typedef typename implementation::key_comparator    key_comparator    ;    ///< Key comparator (binary predicate)
            typedef typename implementation::node_type      node_type       ;   ///< node type

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
                >    other ;         ///< Rebinding result
            };

        public:
            MichaelList()
            {
                assert( gc::hzp::GarbageCollector::isUsed() )   ;
                assert( c_nMaxHazardPtrCount <= gc::hzp::GarbageCollector::instance().getHazardPointerCount() ) ;
            }
        };

        //@cond none
        namespace split_list {

            namespace details {
                template < typename REGULAR_KEY, typename DUMMY_KEY, typename VALUE, typename PAIR_TRAITS, typename TRAITS, class ALLOCATOR >
                struct michael_list_hzp_type_selector {
                    typedef ordered_list::MichaelList<gc::hzp_gc, REGULAR_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>   list_implementation ;

                    typedef typename list_implementation::node_type    regular_node    ;
                    typedef typename regular_node::template rebind<REGULAR_KEY, char>::other  dummy_node     ;

                    typedef typename list_implementation::node_allocator                            regular_node_allocator  ;
                    typedef typename regular_node_allocator::template rebind<dummy_node>::other     dummy_node_allocator    ;

                    struct type_traits: public list_implementation::type_traits::original {

                        typedef typename list_implementation::type_traits::original::template split_list_node_deleter_functor<
                            regular_node
                            , dummy_node
                            , regular_node_allocator
                            , dummy_node_allocator
                        >   node_deleter_functor    ;
                    };

                    typedef typename list_implementation::template rebind<
                        typename list_implementation::key_type,
                        typename list_implementation::value_type,
                        typename list_implementation::pair_traits,
                        type_traits
                    >:: other       michael_list_type   ;
                };
            }   // namespace details

            template < typename REGULAR_KEY, typename DUMMY_KEY, typename VALUE, typename PAIR_TRAITS, typename TRAITS, class ALLOCATOR >
            class MichaelList<gc::hzp_gc, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                : public details::michael_list_hzp_type_selector< REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >::michael_list_type
            {
                typedef details::michael_list_hzp_type_selector< REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >  type_selector   ;
                typedef typename type_selector::michael_list_type   base_class   ;
            public:
                typedef typename type_selector::regular_node    regular_node    ;
                typedef typename type_selector::dummy_node      dummy_node      ;

                typedef typename type_selector::regular_node_allocator      regular_node_allocator  ;
                typedef typename type_selector::dummy_node_allocator        dummy_node_allocator    ;

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
            struct impl_selector< michael_list_tag<gc::hzp_gc>, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
            {
                typedef MichaelList<gc::hzp_gc, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR> type    ;
            };

        }   // namespace split_list
        //@endcond

    }    // namespace ordered_list
}    // namespace cds


#endif // #ifndef __CDS_ORDERED_LIST_MICHAEL_LIST_HZP_H
