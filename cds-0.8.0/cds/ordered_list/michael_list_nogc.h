/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ORDERED_LIST_MICHAEL_LIST_NOGC_H
#define __CDS_ORDERED_LIST_MICHAEL_LIST_NOGC_H

#include <cds/ordered_list/michael_list.h>
#include <cds/gc/no_gc.h>
#include <cds/concept.h>
#include <cds/ref.h>

namespace cds {
    namespace ordered_list {

        namespace details { namespace michael_list {

            /// Michael's list node for implementation without node deletion (no reclamation schema needed)
            template <typename KEY, typename VALUE, typename ALLOCATOR>
            class Node< gc::no_gc, KEY, VALUE, ALLOCATOR >
            {
            public:
                typedef KEY     key_type    ;   ///< Key type
                typedef VALUE   value_type  ;   ///< Value type
                typedef cds::details::Allocator< Node, ALLOCATOR>   node_allocator  ;   ///< Node allocator type

                /// Rebind key/value type
                template <typename KEY2, typename VALUE2>
                struct rebind {
                    typedef Node<gc::no_gc, KEY2, VALUE2, ALLOCATOR>   other   ;    ///< Rebinding result
                };

                // Caveats: order of fields is important!
                // The split-list implementation uses different node types, dummy node contains only the hash and no key and data field
                // Therefore, key and data must be last fields in the node

                atomic<Node *>  m_pNext ;    ///< Pointer to the next node in the list
                const key_type  m_key   ;   ///< Node's key
                value_type      m_value ;   ///< Node's value

                /// Default ctor
                Node()
                    : m_pNext(NULL)
                {}

                /// Node's key initializing
                Node( const key_type& key )
                    : m_pNext(NULL)
                    , m_key( key )
                {}

                /// Node's key and value initializing
                Node( const key_type& key, const value_type& val )
                    : m_pNext(NULL)
                    , m_key( key )
                    , m_value( val )
                {}

                /// Returns node's key
                const key_type& key()    { return m_key ; }

                /// Returns node's data
                value_type&     value()    { return m_value    ;    }
            };
        }} // namespace details::michael_list

        /// Persistent (no deletion) Michael's ordered list
        /**
            Implementation of Michael's ordered list with no garbage collection

            \par Template parameters
                \li \p KEY        type of key stored in list
                \li \p VALUE    type of value stored in list
                \li \p PAIR_TRAITS (key, value) pair traits. The default is cds::map::pair_traits <\p KEY, \p VALUE>
                \li \p TRAITS    list traits. The default is cds::ordered_list::type_traits
                \li \p ALLOCATOR node allocator. The default is \ref CDS_DEFAULT_ALLOCATOR

            This is a specialization of \a MichaelList class. Deletion of item is not supported.

            \par Source
                [2002] Maged Michael "High performance dynamic lock-free hash tables and list-based sets"

            \par
                Common interface: see \ref ordered_list_common_interface.
        */
        template <typename KEY, typename VALUE, typename PAIR_TRAITS, typename TRAITS, class ALLOCATOR >
        class MichaelList<gc::no_gc, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >
            : public OrderedList< KEY, VALUE, TRAITS >
            , public michael_list_tag<gc::no_gc>
        {
        protected:
            //@cond
            typedef OrderedList<
                KEY,
                VALUE,
                TRAITS
            >        base_class    ;

            typedef MichaelList this_class   ;
            //@endcond

        public:
            typedef gc::no_gc                            gc_schema        ;    ///< Garbage collection schema
            typedef typename base_class::key_type        key_type        ;    ///< Key type
            typedef typename base_class::value_type        value_type        ;    ///< Value type
            typedef PAIR_TRAITS                            pair_traits        ;    ///< Pair traits type
            typedef typename pair_traits::key_traits    key_traits        ;    ///< Key traits
            typedef typename pair_traits::value_traits    value_traits    ;    ///< Value traits

            typedef typename key_traits::key_comparator    key_comparator    ;    ///< Key comparator (binary predicate)

            typedef typename details::michael_list::Node<gc_schema, key_type, value_type, ALLOCATOR>    Node        ;    ///< internal node

            /// List type traits
            struct type_traits: public base_class::type_traits {
                typedef typename base_class::type_traits     original        ;   ///< Original (non-void) type traits

                /// Functor to delete retired node
                typedef typename cds::details::void_selector<
                    typename original::node_deleter_functor,
                    cds::details::deferral_deleter<Node, ALLOCATOR>
                >::type                 node_deleter_functor ;
            } ;

            /// Rebind type of key-value pair stored in the list
            template <typename KEY2, typename VALUE2, typename PAIR_TRAITS2, typename TRAITS2 = typename type_traits::original>
            struct rebind {
                typedef MichaelList< gc_schema, KEY2, VALUE2, PAIR_TRAITS2, TRAITS2, ALLOCATOR >    other ; ///< Rebinding result
            };

            typedef typename Node::node_allocator                               node_allocator        ;    ///< node allocator

        protected:
            Node * volatile         m_pHead ;           ///< List head node
            key_comparator            m_Cmp    ;            ///< Key comparator
            node_allocator            m_Allocator    ;        ///< Node allocator

            //@cond
            struct Position {
                Node * volatile *   ppPrev    ;
                Node *                pCur    ;
                Node *                pNext    ;
            };
            //@endcond

            /// Non-concurrent iterator.
            /**
                This iterator is not thread safe.
                Template parameter \p Q defines type of iterator return value (const or not)
            */
            template <typename VALUE_REF>
            class NonConcurrentIterator: public concept::non_concurrent_iterator< value_type >
            {
                friend class MichaelList    ;

            public:
                typedef Node                            node_type    ;    ///< internal node type
                typedef typename this_class::key_type        key_type    ;    ///< key type
                typedef typename this_class::value_type        value_type    ;    ///< value type
                typedef VALUE_REF                        value_reference    ;    ///< value reference type

            protected:
                //@cond
                node_type * m_pNode    ;
                //@endcond

            protected:
                //@cond
                NonConcurrentIterator( node_type * pNode ): m_pNode( pNode )
                {}

                void next()
                {
                    if ( m_pNode )
                        m_pNode = m_pNode->m_pNext.template load<membar_relaxed>()    ;
                }
                //@endcond

            public:
                /// Default ctor
                NonConcurrentIterator(): m_pNode( NULL ) {}
                /// Copy ctor
                NonConcurrentIterator( const NonConcurrentIterator& it ): m_pNode( it.m_pNode ) {}

                /// Returns const reference to key of node
                const key_type& key() const
                {
                    assert( m_pNode != NULL ) ;
                    return m_pNode->key()    ;
                }

                /// Returns reference (const reference for const iterator) to value of node
                VALUE_REF value() const
                {
                    assert( m_pNode != NULL ) ;
                    return m_pNode->value() ;
                }

                /// Return pointer to node. Debugging method
                const node_type * getNode() const { return m_pNode; }

                /// Pre-increment
                NonConcurrentIterator& operator ++()
                {
                    assert( m_pNode != NULL )    ;
                    next()    ;
                    return *this                ;
                }

                /// Post-increment
                NonConcurrentIterator operator ++(int)
                {
                    assert( m_pNode != NULL )    ;
                    NonConcurrentIterator<VALUE_REF> it( *this )        ;
                    next()                        ;
                    return it                    ;
                }

                /// Iterator comparison
                bool operator ==( const NonConcurrentIterator<VALUE_REF>& it ) const { return m_pNode == it.m_pNode; }
                /// Iterator comparison
                bool operator !=( const NonConcurrentIterator<VALUE_REF>& it ) const { return !( *this == it ) ;}
            };

        protected:
            //@cond
            Node * allocNode( const KEY& key, const VALUE& val )
            {
                return m_Allocator.New( key, val )    ;
            }
            Node * allocNode( const KEY& key )
            {
                return m_Allocator.New( key )    ;
            }
            void   freeNode( Node * pNode )
            {
                type_traits::node_deleter_functor::free( pNode )    ;
            }
            //@endcond

        public:
            /// Create empty list
            MichaelList()
                : m_pHead( NULL )
            {}

            ~MichaelList()
            {
                clear() ;
            }

        public:
            /// Inserts new node
            bool insert(
                const KEY& key,            ///< Node's key
                const VALUE& val        ///< Node's value
            )
            {
                return insert( m_pHead, key, val )    ;
            }

            /// Inserts new node. If \p key exists returns NULL, else returns pointer to the value field of inserted item
            VALUE * insert( const KEY& key )
            {
                Node * pNode = allocNode( key );
                if ( !insert( m_pHead, pNode )) {
                    freeNode( pNode )    ;
                    return NULL ;
                }
                return &( pNode->m_data )   ;
            }

            template <typename T, typename FUNC>
            bool insert( const KEY& key, const T& val, FUNC func )
            {
                return insert( m_pHead, key, val, func )    ;
            }

            /// Ensures that key \p key exists in list, changes data of existing item to \p data
            /**
                The operation performs insert or change data with lock-free manner.

                If \p key is in the list then changes its value to \p val
                If \p key is not in list then add it.
                Returns std::pair<bool, bool> where \p first is true if operation is successfull,
                \p second is true if new item has been added or \p false if the item with \p key
                already in list.
            */
            template <typename T, typename FUNC>
            std::pair<bool, bool> ensure( const KEY& key, const T& val, FUNC func )
            {
                return ensure( m_pHead, key, val, func )    ;
            }

            /// Emplaces the vale of key \p key with new value \p val
            /**
                The operation changes the value (or a part of the value) of key \p key to new one \p val.
                The user-defined functor \p func has the purpose like \p func argument of \ref ensure
                member function. The signature of \p func is
                \code
                void func( VALUE& itemValue, const T& val ) ;
                \endcode
                The first argument \p itemValue of user-defined function \p func is the reference
                to the list's item pointed by key \p key. The second argument \p val is the value passed
                to \p emplace member function. User-defined function \p func must guarantee that during changing
                key's value no any other changes could be made on this list's item by concurrent threads.
                The list only garantees that the item found by key cannot be deleted while \p func worked

                The function returns \p true if the key \p key exists in the list.
            */
            template <typename T, typename FUNC>
            bool emplace( const KEY& key, const T& val, FUNC func )
            {
                return emplace( m_pHead, key, val, func )   ;
            }


               /// Gets value for the key
            /**
                Returns pointer to value for \p key in list, \p NULL if the list does not contain \p key
            */
            VALUE * get( const KEY& key )
            {
                return find( m_pHead, key ) ;
            }

            /// Finds node by \p key. Returns false if \p key is not in the list
            bool find( const KEY& key )
            {
                return find( m_pHead, key )    != NULL ;
            }

            /// Checks if list is empty
            bool empty() const
            {
                return atomics::load<membar_acquire>( &m_pHead ) == NULL ;
            }

            /// Clear the list. Not thread safe!!!
            void clear()
            {
                Node * pHead ;
                while ( true ) {
                    pHead = m_pHead ;
                    if ( atomics::cas<membar_release>( &m_pHead, pHead, (Node *) NULL ))
                        break;
                }

                while ( pHead ) {
                    Node * p = pHead->m_pNext.template load<membar_relaxed>()   ;
                    freeNode( pHead )   ;
                    pHead = p           ;
                }
            }

        public:    // Non-concurrent iterators
            /// Non-concurrent iterator
            /**
                This iterator is not thread-safe.
            */
            typedef NonConcurrentIterator< value_type& >        nonconcurrent_iterator    ;

            /// Non-concurrent iterator
            /**
                This iterator is not thread-safe.
            */
            typedef NonConcurrentIterator< const value_type& >    nonconcurrent_const_iterator    ;

            /// Returns non-concurrent forward start iterator
            /**
                For an empty list start iterator is equal to \a end().
            */
            nonconcurrent_iterator    nc_begin()
            {
                return nonconcurrent_iterator( m_pHead )    ;
            }

            /// Returns non-concurrent forward end iterator
            /**
                End iterator is simple NULL. You may use end iterator for comparison only
            */
            nonconcurrent_iterator    nc_end()
            {
                return nonconcurrent_iterator()    ;
            }

            /// Returns non-concurrent const forward start iterator
            /**
                For an empty list start iterator is equal to \a end()
            */
            nonconcurrent_const_iterator    nc_begin() const
            {
                return nonconcurrent_iterator( m_pHead )    ;
            }

            /// Returns non-concurrent const forward end iterator
            /**
                End iterator is simple NULL. You may use end iterator for comparison only
            */
            nonconcurrent_const_iterator    nc_end() const
            {
                return nonconcurrent_iterator()    ;
            }

        protected:
        //@cond
            /// Inserts new node <\p key, \p val> to sublist starting with \p refHead
            bool insert( Node * volatile & refHead, const KEY& key, const VALUE& val )
            {
                Node * pNode = allocNode( key, val );
                if ( !insert( refHead, pNode )) {
                    freeNode( pNode )    ;
                    return false    ;
                }
                return true    ;
            }

            template <typename T, typename FUNC>
            bool insert( Node * volatile & refHead, const KEY& key, const T& val, FUNC func )
            {
                Node * pNode = allocNode( key );
                if ( !insert( refHead, pNode )) {
                    freeNode( pNode )    ;
                    return false    ;
                }
                unref(func)( pNode->value(), val ) ;
                return true    ;
            }

            /// Inserts node \p pNode into the list
            bool insert( Node * pNode )
            {
                return insert( m_pHead, pNode ) ;
            }

            /// Inserts node \p pNode to list starting with \p refHead
            bool insert( Node * volatile & refHead, Node * pNode )
            {
                Position pos    ;
                while ( true ) {
                    if ( search( refHead, pNode->key(), pos ) )
                        return false        ;
                    pNode->m_pNext.template store<membar_relaxed>( pos.pCur )   ;
                    if ( atomics::cas<membar_release>( pos.ppPrev, pos.pCur, pNode ))
                        return true    ;
                }
            }

            /// Ensures that key \p key exists in list starting with \p refHead. If key is not exists then it will be added in list
            template <typename T, typename FUNC>
            std::pair<bool, bool> ensure( Node * volatile & refHead, const KEY& key, const T& val, FUNC func )
            {
                Position pos    ;
                if ( search( refHead, key, pos )) {
                    unref(func)( pos.pCur->m_value, val, false )   ;
                    return std::pair<bool, bool>(true, false) ;
                }

                Node * pNode = allocNode( key ) ;
                while ( true ) {
                    if ( search( refHead, key, pos ) ) {
                        freeNode( pNode )   ;
                        unref(func)( pos.pCur->m_value, val, false )       ;
                        return std::pair<bool, bool>(true, false) ;
                    }
                    pNode->m_pNext.template store<membar_relaxed>( pos.pCur ) ;
                    if ( atomics::cas<membar_release>( pos.ppPrev, pos.pCur, pNode )) {
                        unref(func)( pNode->m_value, val, true )   ;
                        return std::pair<bool, bool>(true, true) ;
                    }
                }
            }

            /// Changes the value of its part for the list item pointed by \p key
            template <typename T, typename FUNC>
            bool emplace( Node * volatile & refHead, const KEY& key, const T& val, FUNC func )
            {
                VALUE * p = find( refHead, key ) ;
                if ( !p )
                    return false    ;
                unref(func)( *p, val )        ;
                return true            ;
            }


            /// Finds key \p key in list starting with refHead. Returns pointer to value found or NULL, if key is not found
            VALUE * find( Node * volatile & refHead, const KEY& key )
            {
                Position pos ;
                if ( search( refHead, key, pos ))
                    return &( pos.pCur->value() )    ;
                return NULL ;
            }
        //@endcond

        private:
        //@cond
            /// Search position of kee \p key in list startinf with \p pHead
            bool search( Node * volatile & pHead, const KEY& key, Position& pos )
            {
                Node * volatile * pPrev    ;
                Node * pNext    ;
                Node * pCur        ;
try_again:
                pPrev = &pHead    ;
                pCur = *pPrev    ;
                pNext = NULL    ;

                while ( true ) {
                    if ( !pCur ) {
                        pos.ppPrev = pPrev    ;
                        pos.pCur = pCur        ;
                        pos.pNext = pNext    ;
                        return false        ;
                    }

                    pNext = pCur->m_pNext.template load<membar_relaxed>()    ;
                    if ( pCur->m_pNext.template load<membar_acquire>() != pNext )
                        goto try_again ;

                    KEY ckey = pCur->key() ;
                    if ( atomics::load<membar_acquire>( pPrev ) != pCur )
                        goto try_again    ;

                    int nCmp ;
                    if ( ( nCmp = m_Cmp( ckey, key )) >= 0 ) {
                        pos.ppPrev = pPrev    ;
                        pos.pCur = pCur        ;
                        pos.pNext = pNext    ;
                        return nCmp == 0    ;
                    }
                    pPrev = &( pCur->m_pNext ) ;

                    pCur = pNext ;
                }
            }
            //@endcond
        };

        //@cond none
        namespace split_list {
            namespace details {
                template < typename REGULAR_KEY, typename DUMMY_KEY, typename VALUE, typename PAIR_TRAITS, typename TRAITS, class ALLOCATOR >
                struct michael_list_nogc_type_selector {
                    typedef ordered_list::MichaelList<gc::no_gc, REGULAR_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>   list_implementation ;

                    typedef typename list_implementation::Node    regular_node    ;
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
            class MichaelList<gc::no_gc, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                : public details::michael_list_nogc_type_selector< REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >::michael_list_type
            {
                typedef details::michael_list_nogc_type_selector< REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR > type_selector ;
            public:
                typedef typename type_selector::regular_node    regular_node    ;
                typedef typename type_selector::dummy_node      dummy_node     ;

                typedef typename type_selector::regular_node_allocator  regular_node_allocator  ;
                typedef typename type_selector::dummy_node_allocator    dummy_node_allocator    ;

                typedef regular_node *  node_ptr    ;
                typedef regular_node *  node_ref    ;

            public:
                dummy_node *    allocDummy( const DUMMY_KEY& key )
                {
                    dummy_node_allocator a  ;
                    return a.New( key )     ;
                }

                static regular_node *  deref( node_ref refNode ) { return refNode  ; }
            };

            template <
                typename REGULAR_KEY,
                typename DUMMY_KEY,
                typename VALUE,
                typename PAIR_TRAITS,
                typename TRAITS,
                class ALLOCATOR
            >
            struct impl_selector< michael_list_tag<gc::no_gc>, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
            {
                typedef MichaelList<gc::no_gc, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR> type    ;
            };

        }   // namespace split_list
        //@endcond


    }    // namespace ordered_list
}    // namespace cds

#endif // #ifndef __CDS_ORDERED_LIST_MICHAEL_LIST_NOGC_H

