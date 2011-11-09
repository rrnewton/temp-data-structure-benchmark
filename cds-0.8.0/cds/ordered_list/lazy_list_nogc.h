/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ORDERED_LIST_LAZY_LIST_NOGC_H
#define __CDS_ORDERED_LIST_LAZY_LIST_NOGC_H

#include <cds/ordered_list/lazy_list.h>
#include <cds/gc/no_gc.h>
#include <cds/concept.h>
#include <cds/ref.h>


namespace cds {
    namespace ordered_list {

        namespace details { namespace lazy_list {
            /// Lazy list node for implementation without node deletion (no reclamation schema needed)
            /**
                Template parameters:
                    \li \p GC        Memory reclamation schema
                    \li \p KEY        type of key stored in list
                    \li \p VALUE    type of value stored in list
                    \li \p LOCK        lock primitive
            */
            template <typename KEY, typename VALUE, typename LOCK, typename ALLOCATOR>
            class Node<gc::no_gc, KEY, VALUE, LOCK, ALLOCATOR>  {
            public:
                typedef KEY     key_type    ;   ///< Key type
                typedef VALUE   value_type  ;   ///< Value type
                typedef LOCK    lock_type       ;    ///< node locker type

                typedef cds::details::Allocator<Node, ALLOCATOR>   node_allocator    ;    ///< Node allocator type

                /// Rebind key/value type
                template <typename KEY2, typename VALUE2, typename LOCK2=lock_type>
                struct rebind {
                    typedef Node<gc::no_gc, KEY2, VALUE2, LOCK2, ALLOCATOR>    other ; ///< Rebinding result
                };

            public:
                Node *              m_pNext        ;    ///< Pointer to next node in list
            private:
                lock_type            m_Lock        ;    ///< Node locker
            public:
                // Caveats: order of fields is important!
                // The split-list implementation uses different node types, dummy node contains only the hash and no key and data field
                // Therefore, key and data must be last fields in the node
                const key_type        m_key        ;    ///< Node's key
                value_type            m_data        ;    ///< Node's data

            public:
                /// Default ctor.
                Node()
                    : m_key()
                    , m_pNext( NULL )
                {}

                /// Node's key initializer
                Node( const key_type& key )
                    : m_key( key )
                    , m_pNext( NULL )
                {}

                /// Node's key and value initializer
                Node( const key_type& key, const value_type& data )
                    : m_key(key)
                    , m_pNext( NULL )
                    , m_data( data )
                {}

                /// Locks the node
                void    lock()                { m_Lock.lock();    }
                /// Unlocks the node
                void    unlock()            { m_Lock.unlock();    }

                /// Returns node's key
                const key_type& key()    { return m_key ; }

                /// Returns node's data
                value_type&     value()    { return m_data;    }

            };
        }} // namespace details::lazy_list


        /// Persistent (no item deletion) lazy list specialization
        /**
            \par Template parameters
                \li \p KEY        type of key stored in list
                \li \p VALUE    type of value stored in list
                \li \p PAIR_TRAITS (key, value) pair traits. The default is cds::map::pair_traits <\p KEY, \p VALUE>
                \li \p TRAITS    list traits. The default is cds::ordered_list::type_traits
                \li \p ALLOCATOR node allocator. The default is \ref CDS_DEFAULT_ALLOCATOR

            \par Source
                [2005] Steve Heller, Maurice Herlihy, Victor Luchangco, Mark Moir, William N. Scherer III, and Nir Shavit
                "A Lazy Concurrent List-Based Set Algorithm"

            \par
                Common interface: see \ref ordered_list_common_interface.
        */
        template <typename KEY, typename VALUE, typename PAIR_TRAITS, typename TRAITS, class ALLOCATOR >
        class LazyList<gc::no_gc, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
            : public OrderedList< KEY, VALUE, TRAITS >
            , public lazy_list_tag< gc::no_gc >
        {
            //@cond
            typedef OrderedList< KEY, VALUE, TRAITS >            base_class    ;
            typedef LazyList    this_class    ;

            typedef typename details::lazy_list::lock_type<typename base_class::type_traits::lock_type>::type  lock_impl   ;
            //@endcond
        public:
            typedef gc::no_gc                            gc_schema        ;    ///< Garbage collection schema

            /// List node type
            typedef details::lazy_list::Node<
                gc_schema
                , KEY
                , VALUE
                , lock_impl
                , ALLOCATOR
            > Node        ;

            typedef typename base_class::key_type        key_type        ;    ///< Key type
            typedef typename base_class::value_type        value_type        ;    ///< Value type
            typedef PAIR_TRAITS                            pair_traits        ;    ///< Pair traits type
            typedef typename pair_traits::key_traits    key_traits        ;    ///< Key traits
            typedef typename pair_traits::value_traits    value_traits    ;    ///< Value traits

            typedef typename key_traits::key_comparator    key_comparator    ;    ///< Key comparator (binary predicate)

            /// Ordered list traits
            struct type_traits: public base_class::type_traits
            {
                typedef typename base_class::type_traits     original    ;   ///< Original (template parameter \p TRAITS) type traits

                /// Node Locker type
                typedef lock_impl   lock_type   ;

                /// Functor for physically removing of retired node
                typedef typename cds::details::void_selector<
                    typename original::node_deleter_functor,
                    cds::details::deferral_deleter<Node, ALLOCATOR>
                >::type                 node_deleter_functor ;
            };

            /// Rebind type of key-value pair stored in the list
            template <typename KEY2, typename VALUE2, typename PAIR_TRAITS2, typename TRAITS2 = typename type_traits::original>
            struct rebind {
                typedef LazyList< gc_schema, KEY2, VALUE2, PAIR_TRAITS2, TRAITS2, ALLOCATOR >    other ; ///< Rebinding result
            };

            /// Non-concurrent iterator.
            /**
                This iterator is not thread safe.
                Template parameter \p VALUE_REF defines type of iterator return value (const or not)
            */
            template <typename VALUE_REF>
            class NonConcurrentIterator: public concept::non_concurrent_iterator< value_type >
            {
                friend class LazyList    ;

            public:
                typedef Node                            node_type    ;    ///< internal node type
                typedef typename this_class::key_type    key_type    ;    ///< key type
                typedef typename this_class::value_type    value_type    ;    ///< value type
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
                    assert( m_pNode != NULL )    ;
                    if ( m_pNode->m_pNext != NULL ) {
                        m_pNode = m_pNode->m_pNext    ;
                    }
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
                    assert( m_pNode != NULL )    ;
                    return m_pNode->key()        ;
                }

                /// Returns reference (const reference for const iterator) to value of node
                VALUE_REF value() const
                {
                    assert( m_pNode != NULL )    ;
                    return m_pNode->value()        ;
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

            typedef typename Node::node_allocator    node_allocator        ;    ///< node allocator

        protected:
            Node                    m_Head    ;        ///< List head (dummy node)
            Node                    m_Tail    ;        ///< List tail (dummy node)
            key_comparator            m_Cmp    ;        ///< Key comparator
            node_allocator            m_Allocator    ;    ///< Node allocator

            /// Position pointer for item search.
            /**
                New node must be inserted between \p pPred and \p pCur

            \b Condition: \p pPred != NULL and \p pCur != NULL because the list contains dummy nodes \p m_Head and \p m_Tail
            */
            struct Position {
                Node *        pPred    ;    ///< Previous node
                Node *        pCur    ;    ///< Current node

                /// Locks nodes \p pPred and \p pCur
                void lock()
                {
                    pPred->lock()    ;
                    pCur->lock()    ;
                }

                /// Unlocks nodes \p pPred and \p pCur
                void unlock()
                {
                    pCur->unlock()    ;
                    pPred->unlock()    ;
                }
            };

        protected:
            /// Allocates node with \p key
            Node * allocNode( const KEY& key )                        { return m_Allocator.New( key ); }

            /// Allocates new node with \p key and \p data
            Node * allocNode( const KEY& key, const VALUE& data )    { return m_Allocator.New( key, data )    ; }

            /// Frees the node \p pEntry
            void   freeNode( Node * pEntry )                        { type_traits::node_deleter_functor::free( pEntry )    ; }

            /// Returns the reference to list head
            Node&    getHead()    { return m_Head; }

        public:
            LazyList()
            {
                m_Head.m_pNext = &m_Tail    ;
            }

            ~LazyList()
            {
                clear()    ;
            }

        public:

            /// Inserts new node
            /**
                Inserts new node <\p key, \p data> into the list.
                Returns \p false if key \p key already in list,\p true otherwise.
            */
            bool insert( const KEY& key, const VALUE& data )
            {
                return insert( &m_Head, key, data )    ;
            }

            template <typename T, typename FUNC>
            bool insert( const KEY& key, const T& val, FUNC func )
            {
                return insert( &m_Head, key, val, func )    ;
            }

            /// Ensures that key \p key exists in list, changes data of existing item to \p data
            /*
                The operation performs insert or change data with lock-free manner.

                If \p key is in the list then changes its value to \p val
                If \p key is not in list then add it.
                Returns std::pair<bool, bool> where \p first is true if operation is successful,
                \p second is true if new item has been added or \p false if the item with \p key
                already in the list.
            */
            template <typename T, typename FUNC>
            std::pair<bool, bool> ensure( const KEY& key, const T& val, FUNC func )
            {
                return ensure( &m_Head, key, val, func )    ;
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
                return emplace( &m_Head, key, val, func )   ;
            }

            /// Gets value for the key
            /**
                Returns pointer to value for \p key in list, \p NULL if the list does not contain \p key
            */
            VALUE * get( const KEY& key )
            {
                return find( &m_Head, key ) ;
            }

            /// Checks if the key is in list
            /**
                Returns \p true if the list contains \p key in list and \p false otherwise
            */
            bool find( const KEY& key )
            {
                return find( &m_Head, key )    != NULL ;
            }

            /// Checks if list is empty
            bool empty() const
            {
                return m_Head.m_pNext == &m_Tail    ;
            }

            /// Clears the list. Not thread safe!!!
            void clear()
            {
                Node * pEntry = m_Head.m_pNext  ;
                m_Head.m_pNext = &m_Tail    ;

                while ( pEntry != &m_Tail ) {
                    Node * p = pEntry->m_pNext  ;
                    freeNode( pEntry )  ;
                    pEntry = p      ;
                }

            }

        public:    // Non-concurrent iterators
            /// Non-concurrent iterator
            /**
                This iterator is not thread-safe.
                В процессе обхода упорядоченного списка с помощью данного итератора не допускается удаление элементов из списка, так как удаление
                элемента, на который указывает итератор, может привести к обращению по удаленному указателю.
            */
            typedef NonConcurrentIterator< value_type& >        nonconcurrent_iterator    ;

            /// Non-concurrent iterator
            /**
                This iterator is not thread-safe.
                В процессе обхода упорядоченного списка с помощью данного итератора не допускается удаление элементов из списка, так как удаление
                элемента, на который указывает итератор, может привести к обращению по удаленному указателю.
            */
            typedef NonConcurrentIterator< const value_type& >    nonconcurrent_const_iterator    ;

            /// Returns non-concurrent forward start iterator
            /**
                For an empty list start iterator is equal to \a end().
            */
            nonconcurrent_iterator    nc_begin()
            {
                return nonconcurrent_iterator( m_Head.m_pNext )    ;
            }

            /// Returns non-concurrent forward end iterator
            /**
                End iterator points to tail dummy node. You may use end iterator for comparison only
            */
            nonconcurrent_iterator    nc_end()
            {
                return nonconcurrent_iterator( &m_Tail )    ;
            }

            /// Returns non-concurrent const forward start iterator
            /**
                For an empty list start iterator is equal to \a end()
                В процессе обхода упорядоченного списка не допускается удаление его элементов.
            */
            nonconcurrent_const_iterator    nc_begin() const
            {
                return nonconcurrent_iterator( m_Head.m_pNext )    ;
            }

            /// Returns non-concurrent const forward end iterator
            /**
                End iterator points to tail dummy node. You may use end iterator for comparison only
            */
            nonconcurrent_const_iterator    nc_end() const
            {
                return nonconcurrent_iterator( &m_Tail )    ;
            }

        protected:
            /// Inserts new node <\p key, \p val> to sublist starting with \p refHead
            bool insert( Node * pHead, const KEY& key, const VALUE& val )
            {
                Node * pNew = allocNode( key, val )    ;
                if ( insert( pHead, pNew ))
                    return true    ;
                freeNode( pNew )    ;
                return false        ;
            }

            template <typename T, typename FUNC>
            bool insert( Node * pHead, const KEY& key, const T& val, FUNC func )
            {
                Node * pNew = allocNode( key )  ;
                if ( insert( pHead, pNew )) {
                    unref(func)( pNew->value(), val )  ;
                    return true    ;
                }
                freeNode( pNew )    ;
                return false        ;
            }

            /// Inserts node \p pNode into the list
            bool insert( Node * pNode )
            {
                return insert( &m_Head, pNode ) ;
            }

            /// Inserts new node \p pNew into the list starting with node \p pHead
            bool insert( Node * pHead, Node * pNew )
            {
                while ( true ) {
                    Position pos = search( pHead, pNew->m_key )    ;
                    pos.lock()    ;

                    if ( validate( pos )) {
                        if ( pos.pCur != &m_Tail && m_Cmp( pos.pCur->m_key, pNew->m_key ) == 0 ) {
                            // failed: key already in list
                            pos.unlock()    ;
                            return false    ;
                        }
                        else {
                            pNew->m_pNext = pos.pCur    ;
                            atomics::store<membar_release>( &( pos.pPred->m_pNext ), pNew ) ;
                            pos.unlock()    ;
                            return true        ;
                        }
                    }
                    pos.unlock() ;
                }
            }

            /// Changes the value of its part for the list item pointed by \p key
            template <typename T, typename FUNC>
            bool emplace( Node * pHead, const KEY& key, const T& val, FUNC func )
            {
                VALUE * p = find( pHead, key )    ;
                if ( !p )
                    return false    ;
                unref(func)( *p, val )        ;
                return true            ;
            }


            /// Ensures that key \p key exists in sublist starting with \p refHead, changes data of existing item to \p val
            template <typename T, typename FUNC>
            std::pair<bool, bool> ensure( Node * pHead, const KEY& key, const T& val, FUNC func )
            {
                Position pos = search( pHead, key ) ;
                pos.lock()  ;
                if ( pos.pCur != &m_Tail && m_Cmp( pos.pCur->m_key, key ) == 0 ) {
                    unref(func)( pos.pCur->m_data, val, false )  ;
                    pos.unlock()    ;
                    return std::make_pair( true, false ) ;
                }

                Node * pNode = allocNode( key, val ) ;

                // pos is locked
                while ( true ) {
                    if ( validate( pos ) ) {
                        pNode->m_pNext = pos.pCur    ;
                        atomics::store<membar_release>( &( pos.pPred->m_pNext ), pNode ) ; // atomic assign
                        unref(func)( pNode->m_data, val, true )    ;
                        pos.unlock()    ;
                        return std::make_pair( true, true ) ;
                    }
                    pos.unlock() ;

                    pos = search( pHead, key ) ;
                    pos.lock()  ;
                    if ( pos.pCur != &m_Tail && m_Cmp( pos.pCur->m_key, key ) == 0 ) {
                        freeNode( pNode )    ;
                        unref(func)( pos.pCur->m_data, val, false )  ;
                        pos.unlock()        ;
                        return std::make_pair( true, false ) ;
                    }
                }
            }

            /// Finds \p key in list starting with node \p pHead
            VALUE * find( Node * pHead, const KEY& key )
            {
                Position pos = search( pHead, key ) ;
                if ( pos.pCur != &m_Tail && m_Cmp( pos.pCur->m_key, key ) == 0 )
                    return &( pos.pCur->m_data ) ;
                return NULL ;
            }

        private:
            /// Searches and returns insertion point for \p key starting with node \p pHead
            Position search( const Node * pHead, const KEY& key ) const
            {
                Position pos    ;
                const Node * pTail = &m_Tail    ;
                pos.pCur = pos.pPred = const_cast<Node *>( pHead )    ;

                while ( pos.pCur != pTail && ( pos.pCur == pHead || m_Cmp( pos.pCur->m_key, key ) < 0 )) {
                    pos.pPred = pos.pCur    ;
                    do {
                        pos.pCur = atomics::load<membar_relaxed>( &( pos.pPred->m_pNext ) )    ;
                    } while ( atomics::load<membar_acquire>( &( pos.pPred->m_pNext ) ) != pos.pCur )    ;
                    assert( pos.pCur != NULL ) ;
                }
                return pos    ;
            }

            /// Checks if position \p pos is valid (all position nodes is not marked)
            bool validate( const Position& pos ) const
            {
                return pos.pPred->m_pNext == pos.pCur ;
            }
        };

        //@cond none
        namespace split_list {

            namespace details {
                template < typename REGULAR_KEY, typename DUMMY_KEY, typename VALUE, typename PAIR_TRAITS, typename TRAITS, class ALLOCATOR >
                struct lazy_list_nogc_type_selector {
                    typedef ordered_list::LazyList<gc::no_gc, REGULAR_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>   list_implementation ;

                    typedef typename list_implementation::Node                                  regular_node    ;
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
            class LazyList<gc::no_gc, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                : public details::lazy_list_nogc_type_selector< REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >::lazy_list_type
            {
                typedef details::lazy_list_nogc_type_selector< REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR > type_selector ;
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
            struct impl_selector< lazy_list_tag<gc::no_gc>, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
            {
                typedef LazyList<gc::no_gc, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR> type    ;
            };

        }   // namespace split_list
        //@endcond


    } // namespace ordered_list
} // namespace cds

#endif //  #ifndef __CDS_ORDERED_LIST_LAZY_LIST_NOGC_H
