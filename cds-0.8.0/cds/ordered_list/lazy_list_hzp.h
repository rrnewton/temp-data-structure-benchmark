/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ORDERED_LIST_LAZY_LIST_HZP_H
#define __CDS_ORDERED_LIST_LAZY_LIST_HZP_H

#include <cds/ordered_list/details/lazy_list_hpgen.h>
#include <cds/gc/hzp/hp_object.h>

namespace cds {
    namespace ordered_list {
        //@cond
        namespace details { namespace lazy_list {

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
                typedef typename details::lazy_list::lock_type<typename reduced_type_traits::lock_type>::type  lock_type_   ;

            public:
                typedef Node<gc::hzp_gc, KEY, VALUE, lock_type_, ALLOCATOR> node_type   ;

            private:
                typedef gc::hzp::Container< node_type, ALLOCATOR >  gc_container ;

            public:
                typedef typename cds::details::void_selector<PAIR_TRAITS, cds::map::pair_traits< KEY, VALUE > >::type    pair_traits ;

                struct type_traits: public reduced_type_traits
                {
                    typedef reduced_type_traits     original    ;    ///< Original (non-void) type traits

                    // Default lock type
                    typedef lock_type_ lock_type   ;

                    /// Functor to delete retired node
                    typedef typename cds::details::void_selector<
                        typename original::node_deleter_functor,
                        typename gc_container::deferral_node_deleter
                    >::type                 node_deleter_functor ;
                } ;

                typedef gc::hzp::AutoHPGuard        auto_guard  ;
                typedef gc::hzp::AutoHPArray<2>     guard_array ;

            public:
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

            public:
                static node_type * getAndGuard( atomic<node_type *>& pSrc, node_type *& pTarget, gc::hzp::HazardPtr& hp )
                {
                    hp = pTarget = pSrc.template load<membar_relaxed>()    ;
                    return pSrc.template load<membar_acquire>()  ;
                }
                static node_type * getAndGuard( atomic<node_type *>&  pSrc, node_type *& pTarget, auto_guard& hp )
                {
                    return getAndGuard( pSrc, pTarget, hp.getHPRef() )  ;
                }

                static bool validate( node_type * pPred, node_type * pCur )
                {
                    return !pPred->template isMarked<membar_relaxed>()
                        && !pCur->template isMarked<membar_relaxed>()
                        && pPred->m_pNext.template load<membar_relaxed>() == pCur ;
                }

                static void linkNode( node_type * pNew, node_type * pPred, node_type * pCur )
                {
                    assert( pPred->m_pNext.template load<membar_relaxed>() == pCur )    ;
                    pNew->m_pNext.template store<membar_relaxed>( pCur )   ;
                    pPred->m_pNext.template store<membar_relaxed>( pNew )   ;
                }

                static void logicalRemove( node_type * pNode )
                {
                    pNode->template mark<membar_relaxed>()    ;
                }

                static void linkNode( node_type * pNode, node_type * pNext )
                {
                    pNode->m_pNext.template store<membar_relaxed>( pNext )    ;
                }
                static void linkNode( node_type * pNode, atomic<node_type *>& pNext )
                {
                    linkNode( pNode, pNext.template  load<membar_relaxed>() )   ;
                }

                static void retirePtr( gc::hzp::ThreadGC& gc, node_type * p, gc::hzp::HazardPtr& /*hp*/ )
                {
                    gc.retirePtr( p, type_traits::node_deleter_functor::free )  ;
                }
            };

        }} // namespace details::lazy_list
        //@endcond

        /// Lazy list specialization for Hazard Pointer reclamation schema
        /**
            \par Source
                \li [2005] Steve Heller, Maurice Herlihy, Victor Luchangco, Mark Moir, William N. Scherer III, and Nir Shavit
                "A Lazy Concurrent List-Based Set Algorithm"

            \par Template parameters
                \li \p KEY        type of key stored in list
                \li \p VALUE    type of value stored in list
                \li \p PAIR_TRAITS (key, value) pair traits. The default is cds::map::pair_traits <\p KEY, \p VALUE>
                \li \p TRAITS    list traits. The default is cds::ordered_list::type_traits
                \li \p ALLOCATOR node allocator. The default is \ref CDS_DEFAULT_ALLOCATOR

            \par
                Common interface: see \ref ordered_list_common_interface.

        */
        template <typename KEY, typename VALUE, typename PAIR_TRAITS, typename TRAITS, class ALLOCATOR >
        class LazyList<gc::hzp_gc, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
            : public details::lazy_list::implementation<
                gc::hzp_gc
                ,details::lazy_list::adapter< gc::hzp_gc, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                ,ALLOCATOR
            >
        {
            //@cond
            typedef details::lazy_list::implementation<
                gc::hzp_gc
                ,details::lazy_list::adapter< gc::hzp_gc, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                ,ALLOCATOR
            >   base_class  ;
            //@endcond

        public:
            typedef typename base_class::node_type  node_type   ;   ///< Internal node type
        protected:
            node_type   m_Head    ;  ///< List head (dummy node)
            node_type   m_Tail    ;  ///< List tail (dummy node)

        public:
            /// Max count of Hazard Pointer for the list
            static const unsigned int   c_nMaxHazardPtrCount = 2    ;

            typedef typename base_class::key_type       key_type    ;   ///< Key type
            typedef typename base_class::value_type     value_type  ;   ///< Value type
            typedef typename base_class::pair_traits    pair_traits ;   ///< Pair (key, value) traits (see cds::map::pair_traits)
            typedef typename base_class::key_traits     key_traits  ;   ///< Key traits (see cds::map::key_traits)
            typedef typename base_class::value_traits   value_traits;   ///< Value traits (see cds::map::value_traits)
            typedef typename base_class::gc_schema      gc_schema   ;   ///< Garbage collecting (GC) algorithm used
            typedef typename base_class::type_traits    type_traits ;   ///< Type traits (see cds::map::traits)
            typedef typename base_class::thread_gc      thread_gc   ;   ///< GC thread supporting type

            /// Rebind type of key-value pair stored in the list
            template <typename KEY2, typename VALUE2, typename PAIR_TRAITS2, typename TRAITS2 = typename type_traits::original>
            struct rebind {
                typedef LazyList< gc_schema, KEY2, VALUE2, PAIR_TRAITS2, TRAITS2, ALLOCATOR >    other ; ///< Rebinding result
            };

        public:
            /// Constructs empty list
            LazyList()
            {
                gc::hzp::GarbageCollector::checkHPCount( c_nMaxHazardPtrCount )  ;
                base_class::m_pHead = &m_Head   ;
				base_class::m_pTail = &m_Tail	;
                m_Head.m_pNext.template store<membar_relaxed>( base_class::m_pTail )    ;
            }

            /// List destructor
            ~LazyList()
            {
                clear()    ;
            }

            /// Checks if the list is empty
            bool empty() const       { return base_class::empty(); }

            /// Clears the list
            void clear()
            {
                thread_gc& gc =  base_class::gc_base_class::getGC()   ;
                typename base_class::gc_guard hpGuard( gc ) ;

                while ( !empty() ) {
                    node_type * pEntry ;
                    do {
                        hpGuard =
                            pEntry = m_Head.m_pNext.template load<membar_relaxed>()  ;
                    } while ( pEntry != m_Head.m_pNext.template load<membar_acquire>() )    ;

                    m_Head.lock()    ;
                    pEntry->lock()    ;

                    pEntry->template mark<membar_relaxed>()    ;    // logically remove
                    m_Head.m_pNext.template store<membar_relaxed>( pEntry->m_pNext.template load<membar_relaxed>() ) ;

                    pEntry->unlock();
                    m_Head.unlock()    ;

                    gc.retirePtr( pEntry, type_traits::node_deleter_functor::free ) ; // free entry
                }
            }
        };

        //@cond none
        namespace split_list {

            namespace details {
                template < typename REGULAR_KEY, typename DUMMY_KEY, typename VALUE, typename PAIR_TRAITS, typename TRAITS, class ALLOCATOR >
                struct lazy_list_hzp_type_selector
                {
                    typedef ordered_list::LazyList<gc::hzp_gc, REGULAR_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>   list_implementation ;

                    typedef typename list_implementation::node_type                              regular_node    ;
                    typedef typename regular_node::template rebind<REGULAR_KEY, char>::other     dummy_node      ;

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
                    >:: other       lazy_list_type   ;
                };
            }   // namespace details

            template < typename REGULAR_KEY, typename DUMMY_KEY, typename VALUE, typename PAIR_TRAITS, typename TRAITS, class ALLOCATOR >
            class LazyList<gc::hzp_gc, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                : public details::lazy_list_hzp_type_selector< REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >::lazy_list_type
            {
                typedef details::lazy_list_hzp_type_selector< REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >  type_selector   ;
            public:
                typedef typename type_selector::regular_node    regular_node    ;
                typedef typename type_selector::dummy_node      dummy_node     ;

                typedef typename type_selector::regular_node_allocator      regular_node_allocator  ;
                typedef typename type_selector::dummy_node_allocator        dummy_node_allocator    ;

                typedef regular_node *  node_ptr    ;
                typedef regular_node *  node_ref    ;

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
            struct impl_selector< lazy_list_tag<gc::hzp_gc>, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
            {
                typedef LazyList<gc::hzp_gc, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR> type    ;
            };

        }   // namespace split_list
        //@endcond


    } // namespace ordered_list
} // namespace cds

#endif //  #ifndef __CDS_ORDERED_LIST_LAZY_LIST_HZP_H
