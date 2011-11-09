/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ORDERED_LIST_MICHAEL_LIST_TAGGED_H
#define __CDS_ORDERED_LIST_MICHAEL_LIST_TAGGED_H

#include <cds/ordered_list/michael_list.h>
#include <cds/concept.h>
#include <cds/gc/tagged/gc.h>

#ifndef CDS_DWORD_CAS_SUPPORTED
#   include <cds/gc/tagged/not_supported.h>
#else

#include <cds/details/markptr.h>
#include <cds/details/void_selector.h>
#include <cds/ref.h>

#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_reference.hpp>

namespace cds {
    namespace ordered_list {
        namespace details {

        //@cond
        namespace michael_list_tagged {
            template <typename TNODE>
            struct CDS_TAGGED_ALIGN_ATTRIBUTE node_ptr: public cds::gc::tagged::tagged_type< TNODE * >
            {
                typedef TNODE                                           node_type  ;
                typedef cds::gc::tagged::tagged_type< node_type * >     base_class ;
                typedef cds::gc::tagged::ABA_tag                        ABA_tag    ;

                enum node_bits {
                    Deleted = 1,    /// Node is logically deleted
                    Changing = 2,   /// Node is changing and cannot be physically deleted
                    BitMask = 3     /// Lowest bit mask
                };

                node_ptr()
                {
                    base_class::m_data = NULL;
                }
                node_ptr( node_type * ptr )
                {
                    base_class::m_data = ptr;
                }
                node_ptr( const node_ptr<TNODE>& src )
                {
                    base_class::m_data = src.m_data;
                }
                node_ptr( node_type * ptr, ABA_tag nTag )
                    : base_class( ptr, nTag )
                {}
                node_ptr( node_type * ptr, ABA_tag nTag, int nMask )
                    : base_class( cds::details::setMarkBits<BitMask>( ptr, nMask ), nTag )
                {}

                node_type *    ptr()
                {
                    return cds::details::unmarkBit<BitMask>( base_class::m_data ) ;
                }
                node_type *    ptr() const
                {
                    return cds::details::unmarkBit<BitMask>( base_class::m_data ) ;
                }
                node_type *    all()
                {
                    return base_class::m_data;
                }
                node_type *    all() const
                {
                    return base_class::m_data;
                }
                bool isDeleted() const
                {
                    return cds::details::isMarkedBit<Deleted>( base_class::m_data ) != 0 ;
                }
                node_type *    setDeleted()
                {
                    return base_class::m_data = cds::details::markBit<Deleted>( base_class::m_data );
                }
                node_type *    clearDeleted()
                {
                    return base_class::m_data = cds::details::unmarkBit<Deleted>( base_class::m_data );
                }
                bool isChanging() const
                {
                    return cds::details::isMarkedBit<Changing>( base_class::m_data ) != 0 ;
                }
                node_type *    setChanging()
                {
                    return base_class::m_data = cds::details::markBit<Changing>( base_class::m_data );
                }
                node_type *    clearChanging()
                {
                    return base_class::m_data = cds::details::unmarkBit<Changing>( base_class::m_data );
                }

                bool        isNull() const
                {
                    return ptr() == NULL ;
                }

                node_ptr<TNODE>    getMarked( int nMask ) const
                {
                    return node_ptr<TNODE>(cds::details::unmarkBit( all(), ~nMask & BitMask), base_class::m_nTag );
                }

                operator node_type *()                  { return ptr(); }
                node_type * operator ->()                { return ptr();    }
                const node_type * operator ->() const    { return ptr();    }

                friend bool operator ==( const node_ptr& p1, const node_ptr& p2 )    { return p1.m_data == p2.m_data;  }
                friend bool operator ==( const node_ptr& p1, const node_type * p2 )    { return p1.m_data == p2;        }
                friend bool operator ==( const node_type * p1, const node_ptr& p2 )    { return p1       == p2.m_data;  }

                friend bool operator !=( const node_ptr& p1, const node_ptr& p2 )    { return !(p1 == p2);    }
                friend bool operator !=( const node_ptr& p1, const node_type * p2 )    { return !(p1 == p2);    }
                friend bool operator !=( const node_type * p1, const node_ptr& p2 )    { return !(p1 == p2);    }
            };
        }   // namespace michael_list_tagged
        //@endcond

        namespace michael_list {
            /// Node specialization for gc::tagged_gc
            template <typename KEY, typename VALUE, typename ALLOCATOR>
            class CDS_TAGGED_ALIGN_ATTRIBUTE Node< gc::tagged_gc, KEY, VALUE, ALLOCATOR >
                : public gc::tagged::ContainerNode
            {
            public:
                typedef KEY     key_type    ;   ///< Key type
                typedef VALUE   value_type  ;   ///< Value type
                typedef michael_list_tagged::node_ptr< Node >   marked_node_ptr  ;   ///< Marked pointer type

                /// Rebind key/value type
                template <typename KEY2, typename VALUE2>
                struct rebind {
                    typedef Node<gc::tagged_gc, KEY2, VALUE2, ALLOCATOR>   other   ;    ///< Rebinding result
                };

            public:
                // Caveats: order of fields is important!
                // The split-list implementation uses different node types, dummy node contains only the hash and no key and data field
                // Therefore, key and data must be last fields in the node

                marked_node_ptr m_pNext ;   ///< Next node in list; lowest bits contain bit flags for the current node
                const key_type  m_key   ;   ///< Node's key
                value_type      m_value ;   ///< Node's value

            public:
                /// Default ctor
                Node()    
                {}

                /// Ctor
                Node(
                    const key_type& key ///< Key of the node
                    )
                    : m_key( key )
                {}

                /// Ctor
                Node(
                    const key_type& key,    ///< Node's key
                    const value_type& val   ///< Node's value
                    )
                    : m_key( key )
                    , m_value( val )
                {}

                //@cond
                void destroy_data()
                {
                    cds::details::call_dtor( const_cast<key_type *>(&m_key) )   ;
                    cds::details::call_dtor( &m_value )   ;
                }
                void construct_data()
                {
                    new ( const_cast<key_type *>(&m_key) ) key_type     ;
                    new ( &m_value ) value_type ;
                }
                void construct_data( const key_type& key )
                {
                    new ( const_cast<key_type *>(&m_key) ) key_type( key )  ;
                }
                void construct_data( const key_type& key, const value_type& val )
                {
                    new ( const_cast<key_type *>(&m_key) ) key_type( key )     ;
                    new ( &m_value ) value_type( val )  ;
                }
                //@endcond

                /// Returns the reference to key of the node
                const key_type& key() const      
                { 
                    return m_key; 
                }

                /// Returns the reference to the value of the node
                const value_type& value() const  
                { 
                    return m_value ; 
                }

                //@cond
                value_type& value()              
                { 
                    return m_value ; 
                }
                //@endcond
            };
        } // namespace michael_list

        namespace michael_list_tagged {

            /// Free-list traits
            template <typename NODE, typename TRAITS>
            class free_list_traits
            {
                //@cond
                typedef NODE    node_type   ;
                typedef type_traits_selector<TRAITS>       list_traits ;
                //@endcond

            public:
                ///< back-off strategy
                typedef typename cds::details::void_selector<
                    typename list_traits::backoff_strategy,
                    cds::backoff::Default
                >::type                    backoff_strategy ;

            private:
                //@cond
                typedef typename cds::details::void_selector<
                    typename list_traits::free_list_type,
                    cds::gc::tagged::FreeList< node_type, backoff_strategy >
                >::type                 source_freelist_type   ;

                typedef typename boost::remove_reference< source_freelist_type >::type  source_freelist_noref ;
                //@endcond

            public:
                /// Type of free-list
                typedef typename source_freelist_noref::template rebind< node_type >::other  freelist_type   ;
                /// Type of the free-list (non-reference)
                typedef freelist_type       freelist_noref  ;
                /// Reference to the free-list
                typedef freelist_noref&     freelist_ref    ;

                //@cond
                typedef typename boost::is_reference< source_freelist_type >    is_freelist_ref ;
                //@endcond
            };

            /// Michael's ordered list with Tagged Pointer GC
            /**
                The class is internal implementation of MichaelList<gc::tagged_gc> and cannot be used directly.
            */
            template <typename KEY,
                typename VALUE,
                typename PAIR_TRAITS,
                typename TRAITS,
                class ALLOCATOR = CDS_DEFAULT_ALLOCATOR >
            class list_base: public OrderedList< KEY, VALUE, TRAITS >
            {
                //@cond
                typedef OrderedList< KEY, VALUE, TRAITS >    base_class    ;
                //@endcond

            public:
                typedef michael_list::Node<gc::tagged_gc, KEY, VALUE>        Node        ;   ///< Node type

            protected:
                typedef node_ptr<Node>                      tagged_node    ;   ///< Tagged node type

                typedef free_list_traits< Node, TRAITS >    freelist_traits ;   ///< Free-list traits

            public:
                typedef typename base_class::key_type       key_type    ;    ///< key type
                typedef typename base_class::value_type     value_type  ;    ///< value type
                typedef PAIR_TRAITS                         pair_traits ;    ///< Pair traits type
                typedef typename pair_traits::key_traits    key_traits  ;    ///< Key traits
                typedef typename pair_traits::value_traits  value_traits;    ///< Value traits

                typedef typename key_traits::key_comparator key_comparator    ;    ///< Key comparator (binary predicate)
                typedef typename freelist_traits::backoff_strategy  backoff_strategy    ;   ///< Back-off strategy

            public:
                /// List type traits
                struct type_traits: public base_class::type_traits {
                    typedef typename base_class::type_traits     original    ;   ///< Original (non-void) type traits

                    typedef typename freelist_traits::backoff_strategy  backoff_strategy    ;   ///< back-off strategy

                    typedef typename freelist_traits::freelist_type     free_list_type      ;   ///< Source free-list type (maybe a reference)

                    typedef typename freelist_traits::freelist_noref    freelist_noref      ;   ///< free-list type (no reference)
                    typedef typename freelist_traits::freelist_ref      freelist_ref        ;   ///< free-list reference type
                    typedef typename freelist_traits::is_freelist_ref   is_freelist_ref     ;   ///< boost::true_type if freelist_type is a reference
                };

                typedef gc::tagged_gc                        gc_schema        ;    ///< Garbage collection schema

            protected:
                tagged_node             m_pHead    ;    ///< list head
                key_comparator            m_Cmp    ;    ///< key comparator

                typename type_traits::freelist_ref      m_FreeList  ;   ///< reference to free-list implementation

                //@cond
                /// Internal position of an item in the list
                struct Position {
                    tagged_node *   pPrev   ;
                    tagged_node     pCur    ;
                    tagged_node     pNext   ;
                };
                //@endcond

                //@cond
                class auto_clear_mark {
                    tagged_node&  pNode     ;
                    int           nFlag     ;

                public:
                    auto_clear_mark( tagged_node&  p, int nMask )
                        : pNode( p )
                        , nFlag( nMask )
                    {
                        assert( cds::details::isMarkedBit( p.m_data, nFlag) == nFlag )   ;
                    }

                    ~auto_clear_mark()
                    {
                        while ( true ) {
                            tagged_node pCur    ;
                            atomics::load<membar_relaxed>( pCur, &pNode ) ;
                            assert( cds::details::isMarkedBit( pCur.m_data, nFlag) == nFlag )   ;
                            if ( gc::tagged::cas_tagged<membar_release>( pNode, pCur, cds::details::unmarkBit( pCur.m_data, nFlag )))
                                break   ;
                        }
                    }
                };
                //@endcond

            protected:
                /// Non-concurrent iterator.
                /**
                    This iterator is not thread safe.
                    Template parameter \p VALUE_REF defines type of iterator return value (const or not)
                */
                template <typename VALUE_REF>
                class NonConcurrentIterator: public concept::non_concurrent_iterator< value_type >
                {
                    //@cond
                    friend class list_base    ;
                    //@endcond

                public:
                    typedef Node                            node_type        ;    ///< internal node type
                    typedef typename list_base::key_type     key_type        ;    ///< key type
                    typedef typename list_base::value_type    value_type        ;    ///< value type
                    typedef VALUE_REF                        value_reference    ;    ///< value reference type

                //@cond
                protected:
                    node_type * m_pNode    ;

                protected:
                    NonConcurrentIterator( const tagged_node& node ): m_pNode( node.data() )
                    {
                        skipDeleted()    ;
                    }

                    void skipDeleted()
                    {
                        while ( m_pNode && m_pNode->m_pNext.isDeleted() )
                            m_pNode = m_pNode->m_pNext.ptr()    ;
                    }

                    void next()
                    {
                        if ( m_pNode )
                            m_pNode = m_pNode->m_pNext.ptr()    ;
                        skipDeleted()    ;
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


            public:
                /// Ctor with external free-list
                list_base( typename type_traits::freelist_ref freeListRef )
                    : m_FreeList( freeListRef )
                {}

                //@cond
                ~list_base()
                {}
                //@endcond

            protected:
                /// Allocates new node and initializes it by \p key and \p data
                Node *    allocNode( const key_type& key, const value_type& data )
                {
                    Node * p = m_FreeList.alloc( key, data ) ;
                    return p    ;
                }
                Node *    allocNode( const key_type& key )
                {
                    Node * p = m_FreeList.alloc( key ) ;
                    return p    ;
                }

                /// Frees the node \p pNode
                void    freeNode( Node * pNode )
                {
                    m_FreeList.free( pNode )            ;
                }

                /// To inter-compiler compatibility
                tagged_node&        getHeadRef()        { return m_pHead; }

            public:
                /// Inserts new item
                /**
                    Inserts new item (\p key, \p data). Returns \p true if new item has been inserted,
                    and \p false if an item with key \p key already in list.
                */
                bool insert( const key_type& key, const value_type& data )
                {
                    return insert( m_pHead, key, data ) ;
                }

                template <typename T, typename FUNC>
                bool insert( const key_type& key, const T& val, FUNC func )
                {
                    return insert( m_pHead, key, val, func )    ;
                }


                /// Ensures that key \p key exists in list, changes data of existing item to \p data
                /*
                    The operation performs insert or change data with lock-free manner.

                    If \p key exists in the list then changes the value of the item pointed by \p key to \p val.
                    The change is performed by calling user-defined function \p func with signature:
                    \code
                        void func( VALUE& itemValue, const T& val, bool bNew ) ;
                    \endcode
                    The first argument \p itemValue of user-defined function \p func is the reference
                    to the list's item pointed by key \p key. The second argument \p val is the value passed
                    to \p ensure member function. The third argument \p bNew is \p true if new item
                    has been added or \p false if \p key found.
                    User-defined function \p func must guarantee that during changing
                    key's value no any other changes could be made on this list's item by concurrent threads.

                    If \p key is not in list then the function adds it.

                    Returns std::pair<bool, bool> where \p first is true if operation is successfull,
                    \p second is true if new item has been added or \p false if the item with \p key
                    already in list.
                */
                template <typename T, typename FUNC>
                std::pair<bool, bool> ensure( const key_type& key, const T& val, FUNC func )
                {
                    return ensure( m_pHead, key, val, func )  ;
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
                bool emplace( const key_type& key, const T& val, FUNC func )
                {
                    return emplace( m_pHead, key, val, func )   ;
                }

                /// Deletes the key from list
                /**
                    Removes \p key from the list. Returns \p true if an item with key \p key has been removed,
                    and \p false if \p key is not in the list
                */
                bool erase( const key_type& key )
                {
                    return erase( m_pHead, key )    ;
                }

                /// Delete \p key from the list
                /**
                    This function is designed to remove a list item. 
                    Before removing the item found by \p key, the functor \p func is called. The functor has the following signature:
                    \code
                    struct erase_functor {
                        void operator()( T& dest, VALUE& itemValue ) ;
                    };
                    \endcode
                    where \p itemValue is the item to erase. The functor may change \p itemValue; for example, 
                    it may produce a preliminary cleaning of \p itemValue, or it may save some data from \p itemValue
                    to \p dest.
                */
                template <typename T, typename FUNC>
                bool erase( const key_type& key, T& dest, FUNC func )
                {
                    return erase( m_pHead, key, dest, func )    ;
                }

                /// Checks if \p key exists in the list
                bool find( const key_type& key )
                {
                    return find( m_pHead, key ) ;
                }

                /// Changes the data of item
                /**
                    If \p key found the function calls user-defined function \p func with parameters:
                    \code
                    void func( T& data, const VALUE& itemValue )
                    \endcode
                    where \p itemValue is the item found by \p key. The user-defined function
                    copies the item's value \p itemValue or its part to \p data. The list guarantees only
                    that the item found cannot be deleted while \p func works. The user-defined function \p func
                    should take into account that concurrent threads may change the item value.

                    Returns \p true if the key is found, \p false otherwise
                */
                template <typename T, typename FUNC>
                bool find( const key_type& key, T& data, FUNC func )
                {
                    return find( m_pHead, key, data, func )   ;
                }

                /// Clears the list. Not thread safe!!!
                void clear()
                {
                    destroy()   ;
                    /*
                    This code is not thread safe:

                    while ( !empty() )
                        erase( m_pHead.data()->m_key ) ;
                    */
                }

                /// Checks if the list is empty
                bool empty() const
                {
                    return atomics::load<membar_acquire>( &m_pHead ).data() == NULL ;
                }

            public:    // Non-concurrent iterators
                /// Non-concurrent iterator
                /**
                    This iterator is not thread-safe.
                    While traversing the list by the iterator it should not delete the items from the list. Deleting an item
                    pointed by iterator may result access violation.
                */
                typedef NonConcurrentIterator< value_type& >        nonconcurrent_iterator    ;

                /// Non-concurrent iterator
                /**
                    This iterator is not thread-safe.
                    While traversing the list by the iterator it should not delete the items from the list. Deleting an item
                    pointed by iterator may result access violation.
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

            //@cond
            protected:
                /// Inserts new item
                /**
                    Inserts new item (\p key, \p data). Returns \p true if new item has been inserted,
                    and \p false if an item with key \p key already in list.
                    The insert position searches starting with \p refHead node
                */
                bool insert( tagged_node& refHead, const key_type& key, const value_type& data )
                {
                    Node * pNode = allocNode( key, data )    ;
                    if ( !insert( refHead, pNode )) {
                        freeNode( pNode )   ;
                        return false        ;
                    }
                    return true ;
                }

                template <typename T, typename FUNC>
                bool insert( tagged_node& refHead, const key_type& key, const T& val, FUNC func )
                {
                    Node * pNode = allocNode( key )    ;
                    if ( !insert( refHead, pNode, val, func )) {
                        freeNode( pNode )   ;
                        return false        ;
                    }
                    return true ;
                }

                /// Inserts node \p pNode into the list
                bool insert( Node * pNode )
                {
                    return insert( m_pHead, pNode ) ;
                }

                /// Inserts new item
                /**
                    Inserts new node \p pNew. Returns \p true if new item has been inserted,
                    and \p false if an item with key \p key already in list.
                    The insert position searches starting with \p refHead node.
                    The node \p pNew is not freed if the key exists
                */
                bool insert( tagged_node& refHead, Node * pNew )
                {
                    Position pos    ;
                    while ( true ) {
                        if ( search( refHead, pNew->key(), pos ) ) {
                            return false        ;
                        }
                        if ( insertAt( pNew, pos ))
                            return true ;
                    }
                }

                template <typename T, typename FUNC>
                bool insert( tagged_node& refHead, Node * pNew, const T& val, FUNC func )
                {
                    Position pos    ;
                    while ( true ) {
                        if ( search( refHead, pNew->key(), pos ) ) {
                            return false        ;
                        }
                        unref(func)( pNew->value(), val )   ;
                        if ( insertAt( pNew, pos /*, Node::marked_node_ptr::Changing*/ )) {
                            //auto_clear_mark acm( pNew->m_pNext, Node::marked_node_ptr::Changing )   ;
                            //unref(func)( pNew->value(), val )   ;
                            return true ;
                        }
                    }
                }

                /// Helper function. Inserts new node \p pNew into position pointed by \p pos
                bool insertAt( Node * pNew, Position& pos, int nFlags = 0 )
                {
                    pNew->m_pNext.template set<membar_relaxed>( 
                        nFlags 
                        ? cds::details::markBit( pos.pCur.m_data, nFlags ) 
                        : pos.pCur.m_data 
                        ) ;
                /*
                    pNew->m_pNext.m_data = nFlags 
                        ? cds::details::markBit( pos.pCur.m_data, nFlags ) 
                        : pos.pCur.m_data 
                        ;
                    ++pNew->m_pNext.m_nTag  ;
                */

                    return gc::tagged::cas_tagged<membar_release>(
                        *pos.pPrev,
                        pos.pCur.getMarked(0),
                        pNew
                    ) ;
                }

                /// Deletes the key from sublist
                /**
                    Removes \p key from the sublist started with \p refHead. Returns \p true if an item with key \p key has been removed,
                    and \p false if \p key is not in the sublist
                */
                bool erase( tagged_node& refHead, const key_type& key )
                {
                    int n   ;
                    return erase( refHead, key, n, trivial_erase_functor()) ;

                    /*
                    Position pos    ;
                    while ( true ) {
                        if ( !search( refHead, key, pos ))
                            return false ;

                        // Logical deletion
                        // If node is not changing then we may mark it for deleting
                        if ( !gc::tagged::cas_tagged<membar_release>(
                                pos.pCur->m_pNext,
                                pos.pNext.getMarked( Node::marked_node_ptr::Changing | Node::marked_node_ptr::Deleted ),
                                cds::details::markBit<Node::marked_node_ptr::Deleted>( pos.pNext.m_data )
                            ))
                        {
                            continue ;
                        }

                        // Physical deletion
                        if ( gc::tagged::cas_tagged<membar_release>(
                                *pos.pPrev,
                                pos.pCur.getMarked(Node::marked_node_ptr::Changing),
                                pos.pNext.ptr()
                            ))
                        {
                            freeNode( pos.pCur.ptr() )    ;
                        }
                        else
                            search( refHead, key, pos ) ;
                        return true    ;
                    }
                    */
                }

                template <typename T, typename FUNC>
                bool erase( tagged_node& refHead, const key_type& key, T& dest, FUNC func )
                {
                    Position pos    ;
                    while ( true ) {
                        if ( !search( refHead, key, pos ))
                            return false ;

                        // Logical deletion
                        // If node is not changing then we may mark it for deleting
                        if ( !gc::tagged::cas_tagged<membar_release>(
                            pos.pCur->m_pNext,
                            pos.pNext.getMarked(0),    //getMarked( Node::marked_node_ptr::Changing | Node::marked_node_ptr::Deleted ),
                            cds::details::markBit<Node::marked_node_ptr::Deleted>( pos.pNext.ptr() )
                            ))
                        {
                            continue ;
                        }

                        unref(func)( dest, pos.pCur->value() ) ;

                        // Physical deletion
                        if ( gc::tagged::cas_tagged<membar_release>(
                            *pos.pPrev,
                            pos.pCur.getMarked(Node::marked_node_ptr::Changing),
                            pos.pNext.ptr()
                            ))
                        {
                            freeNode( pos.pCur.ptr() )    ;
                        }
                        else
                            search( refHead, key, pos ) ;
                        return true    ;
                    }
                }

                /// Ensures that key \p key exists in the sublist, changes data of existing item to \p data
                template <typename T, typename FUNC>
                std::pair<bool, bool> ensure( tagged_node& refHead, const key_type& key, const T& val, FUNC func )
                {
                    Node * pNew = NULL      ;
                    backoff_strategy bkOff  ;

                    Position pos    ;
                    while ( true ) {
                        if ( search( refHead, key, pos ) ) {
                            // Mark item as changing
                            if ( !gc::tagged::cas_tagged<membar_acquire>(
                                pos.pCur->m_pNext,
                                pos.pNext.getMarked(0),
                                cds::details::markBit<Node::marked_node_ptr::Changing>( pos.pNext.m_data )
                                ))
                            {
                                bkOff()     ;
                                continue    ;
                            }

                            {
                                auto_clear_mark acm( pos.pCur->m_pNext, Node::marked_node_ptr::Changing )   ;
                                unref(func)( pos.pCur->value(), val, false )          ;
                            }
                            if ( pNew )
                                freeNode( pNew )    ;
                            return std::make_pair( true, false )    ;
                        }
                        else {
                            if ( !pNew ) {
                                pNew = allocNode( key ) ;
                                unref(func)( pNew->value(), val, true )    ;
                            }
                            if ( insertAt( pNew, pos ))
                                return std::make_pair( true, true ) ;

                            /*
                            if ( !pNew )
                                pNew = allocNode( key ) ;
                            if ( insertAt( pNew, pos, Node::marked_node_ptr::Changing )) {
                                auto_clear_mark acm( pNew->m_pNext, Node::marked_node_ptr::Changing )   ;
                                unref(func)( pNew->value(), val, true )    ;

                                return std::make_pair( true, true ) ;
                            }
                            */
                        }
                    }
                }

                /// Changes the value of its part for the list item pointed by \p key
                template <typename T, typename FUNC>
                bool emplace( tagged_node& refHead, const key_type& key, const T& val, FUNC func )
                {
                    Position pos    ;
                    backoff_strategy bkOff  ;
                    while ( true ) {
                        if ( search( refHead, key, pos ) ) {
                            // Mark item as changing
                            if ( !gc::tagged::cas_tagged<membar_acquire>(
                                pos.pCur->m_pNext,
                                pos.pNext.getMarked(0),
                                cds::details::markBit<Node::marked_node_ptr::Changing>( pos.pNext.m_data )
                                ))
                            {
                                bkOff()     ;
                                continue    ;
                            }

                            auto_clear_mark acm( pos.pCur->m_pNext, Node::marked_node_ptr::Changing )   ;
                            unref(func)( pos.pCur->value(), val )          ;
                            return true     ;
                        }
                        else
                            return false    ;
                    }
                }

                /// Checks if \p key exists in the list starting with \p refHead node
                bool find( tagged_node& refHead, const key_type& key )
                {
                    Position pos    ;
                    return search( refHead, key, pos ) ;
                }

                /// Changes the data of item
                template <typename T, typename FUNC>
                bool find( tagged_node& refHead, const key_type& key, T& data, FUNC func )
                {
                    Position pos    ;
                    backoff_strategy bkOff  ;
                    while( search( refHead, key, pos ) ) {
                        // Mark item as changing
                        if ( !gc::tagged::cas_tagged<membar_acquire>(
                                pos.pCur->m_pNext,
                                pos.pNext.getMarked(0),
                                cds::details::markBit<Node::marked_node_ptr::Changing>( pos.pNext.m_data )
                            ))
                        {
                            bkOff()     ;
                            continue    ;
                        }

                        auto_clear_mark acm( pos.pCur->m_pNext, Node::marked_node_ptr::Changing )   ;
                        unref(func)( data, pos.pCur->value() ) ;
                        return true     ;
                    }
                    return false    ;
                }
            //@endcond

            //@cond
            private:
                bool search( tagged_node& refHead, const key_type& key, Position& pos )
                {
                    tagged_node * pPrev    ;
                    tagged_node   pCur    ;
                    tagged_node   pNext ;
                    tagged_node   p     ;

                    backoff_strategy    bkOff   ;

                try_again:
                    pPrev = &refHead    ;
                    atomics::load<membar_acquire>( pCur, pPrev ) ;
                    while ( true ) {
                        if ( pCur.isNull() ) {
                            pos.pPrev = pPrev    ;
                            pos.pCur = pCur        ;
                            pos.pNext = pNext    ;
                            return false ;
                        }
                        atomics::load<membar_acquire>( pNext, &( pCur->m_pNext) )    ;
                        const key_type& ckey = pCur->key() ;

                        if ( atomics::load<membar_acquire>( p, pPrev ) != pCur || pPrev->isDeleted() ) {
                            bkOff() ;
                            goto try_again ;
                        }

                        if ( !pNext.isDeleted() ) {        // pNext contains deletion flag for the pCur
                            int nCmp    ;
                            if ( ( nCmp = m_Cmp( ckey, key ) ) >= 0 ) {
                                pos.pPrev = pPrev    ;
                                pos.pCur = pCur        ;
                                pos.pNext = pNext    ;
                                return nCmp == 0    ;
                            }

                            pPrev = &( pCur->m_pNext ) ;
                        }
                        else if ( pNext.isChanging() ) {
                            // Node pCur is marked for deletion but is changing at this moment
                            // Wait while changing is done
                            bkOff() ;
                            goto try_again ;
                        }
                        else {
                            // pCur node is logically deleted
                            // Node can be deleted if it is not changing at this moment
                            if ( gc::tagged::cas_tagged<membar_release>(
                                    *pPrev,
                                    pCur.getMarked( Node::marked_node_ptr::Deleted ),
                                    pNext.ptr()
                                ))
                            {
                                freeNode( pCur.ptr() ) ;
                                //++pCur.m_nTag ;
                            }
                            else {
                                bkOff() ;
                                goto try_again    ;
                            }
                        }
                        pCur = pNext ;
                    }
                }
            //@endcond

            protected:
                //@cond
                /// Delete all items from the list. Not thread-safe!!!
                void destroy()
                {
                    while ( !empty() )
                        erase( m_pHead.data()->key() ) ;
                }
                //@endcond
            } ;

            //@cond

            /// MichaelList implementation selector
            /**
                This class is internally used as implementation selector to shared or internal free-list object.
                If TRAITS::free_list_type is a reference then the class selects implementation with external defined
                (shared) free-list object. Otherwise, the implementation with internal free-list member is selected.

                The template parameter \p IS_FREELIST_REF of type bool selects appropriate template specialization.
            */
            template <bool IS_FREELIST_REF,
                typename KEY,
                typename VALUE,
                typename PAIR_TRAITS,
                typename TRAITS,
                class ALLOCATOR >
            class list_share_adapter ;

            template <
                typename KEY,
                typename VALUE,
                typename PAIR_TRAITS,
                typename TRAITS,
                class ALLOCATOR
            >
            class list_share_adapter< false, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >
                : public list_base< KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >
            {
                typedef list_base< KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR > base_class ;

            protected:
                typename base_class::type_traits::freelist_noref  m_FreeList ;
            public:
                list_share_adapter()
                    : base_class( m_FreeList )
                {}

                template <typename Q>
                list_share_adapter( Q& )
                    : base_class( m_FreeList )
                {}

                ~list_share_adapter()
                {
                    base_class::destroy() ;
                }
            };

            template <
                typename KEY,
                typename VALUE,
                typename PAIR_TRAITS,
                typename TRAITS,
            class ALLOCATOR
            >
            class list_share_adapter< true, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >
                : public list_base< KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >
            {
                typedef list_base< KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR > base_class ;
            public:
                list_share_adapter( typename base_class::type_traits::freelist_ref freeList )
                    : base_class( freeList )
                {}

                ~list_share_adapter()
                {
                    base_class::destroy()    ;
                }
            };

            //@endcond

        }}    // namespace details::michael_list_tagged

        /// Lock-free ordered single-linked list developed by Michael.
        /**
            The implementation is based on Tagged Pointers memory reclamation schema

            \par Source
                [2002] Maged Michael "High performance dynamic lock-free hash tables and list-based sets"

            This class is used as the collision list in some lock-free hash map implementation.

            \par Template parameters
                \li \p KEY        type of key stored in list
                \li \p VALUE    type of value stored in list
                \li \p PAIR_TRAITS (key, value) pair traits. The default is cds::map::pair_traits <\p KEY, \p VALUE>
                \li \p TRAITS    list traits. The default is cds::ordered_list::type_traits
                \li \p ALLOCATOR node allocator. The default is \ref CDS_DEFAULT_ALLOCATOR

            This implementation uses the following types defined in \p TRAITS:
                \li \p backoff_strategy - back-off strategy. The default is \p cds::backoff::Default
                \li \p free_list_type - free-list implementation. The default is \p cds::gc::tagged::FreeList.
                    You may set \p TRAITS::free_list_type as a reference to free-list type. In such case
                    the \p MichaelList will use shared free-list object provided by you. Note, that free-list type you
                    provide should have \p rebind< Q, B > template to rebuild template parameters of free-list.
                    See \p cds::gc::tagged::FreeList::rebind.

            For class members documentation see \p details::michael_list_tagged::list_base

            \par
                Common interface: see \ref ordered_list_common_interface.
        */

        template <typename KEY, typename VALUE, typename PAIR_TRAITS, typename TRAITS, class ALLOCATOR >
        class MichaelList<gc::tagged_gc, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >:
            public details::michael_list_tagged::list_share_adapter<
                details::michael_list_tagged::free_list_traits< any_type, TRAITS >::is_freelist_ref::value,
                KEY,
                VALUE,
                PAIR_TRAITS,
                TRAITS,
                ALLOCATOR
            >
            , public michael_list_tag<gc::tagged_gc>
        {
            //@cond
            typedef details::michael_list_tagged::list_share_adapter<
                details::michael_list_tagged::free_list_traits< any_type, TRAITS >::is_freelist_ref::value,
                KEY,
                VALUE,
                PAIR_TRAITS,
                TRAITS,
                ALLOCATOR
            >    base_class    ;
            //@endcond
        public:
            typedef typename base_class::gc_schema            gc_schema        ;    ///< Garbage collection schema

            /// List type traits
            struct type_traits: public base_class::type_traits {
                typedef typename base_class::type_traits     original        ;   ///< Original (non-void) type traits
            } ;

            typedef typename base_class::key_type            key_type        ;    ///< Key type
            typedef typename base_class::value_type            value_type        ;    ///< Value type
            typedef typename base_class::pair_traits        pair_traits        ;    ///< Pair traits type
            typedef typename pair_traits::key_traits        key_traits        ;    ///< Key traits
            typedef typename pair_traits::value_traits        value_traits    ;    ///< Value traits

            typedef typename key_traits::key_comparator    key_comparator    ;    ///< Key comparator (binary predicate)

            /// Rebind type of key-value pair stored in the list
            template <typename KEY2, typename VALUE2, typename PAIR_TRAITS2, typename TRAITS2 = typename type_traits::original>
            struct rebind {
                typedef MichaelList<
                    typename base_class::gc_schema,
                    KEY2,
                    VALUE2,
                    PAIR_TRAITS2,
                    TRAITS2,
                    ALLOCATOR
                >    other ;     ///< Rebinding result
            };


        public:
            /// External free-list ctor. This ctor should be used when \p TRAITS::free_list_type is a reference
            MichaelList( typename base_class::type_traits::freelist_ref ref )
                : base_class( ref )
            {}

            /// Internal free-list ctor. This ctor should be used when \p TRAITS::free_list_type is not the reference
            MichaelList()
                : base_class()
            {}
        } ;

        //@cond none
        namespace split_list {
            namespace details {
                template < typename REGULAR_KEY, typename DUMMY_KEY, typename VALUE, typename PAIR_TRAITS, typename TRAITS, class ALLOCATOR >
                struct michael_list_tagged_type_selector {
                    typedef ordered_list::MichaelList<gc::tagged_gc, REGULAR_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>   list_implementation ;

                    typedef typename list_implementation::Node                                  regular_node    ;
                    typedef typename regular_node::template rebind< DUMMY_KEY, char>::other     dummy_node      ;

                    struct type_traits: public list_implementation::type_traits::original
                    {
                        typedef typename list_implementation::type_traits::free_list_type               regular_node_freelist   ;
                        typedef typename regular_node_freelist::template rebind<dummy_node>::other      dummy_node_freelist     ;

                        class free_list_type
                        {
                            dummy_node_freelist     m_DummyFreeList     ;
                            regular_node_freelist   m_RegularFreeList   ;

                        public:
                            typedef typename regular_node_freelist::node_type           value_type          ;
                            typedef typename regular_node_freelist::node_type *         value_ptr           ;
                            typedef typename regular_node_freelist::allocator_type      allocator_type      ;
                            typedef typename regular_node_freelist::backoff_strategy    backoff_strategy    ;

                            template <typename Q, typename B = backoff_strategy, typename A = allocator_type >
                            struct rebind {
                                typedef free_list_type  other ;
                            };

                        public:
                            value_ptr    alloc()
                            {
                                return m_RegularFreeList.alloc()    ;
                            }

                            template <typename T1>
                            value_ptr    alloc( const T1& init)
                            {
                                return m_RegularFreeList.alloc( init )    ;
                            }

                            template <typename T1, typename T2>
                            value_ptr    alloc( const T1& init1, const T2& init2)
                            {
                                return m_RegularFreeList.alloc( init1, init2 )    ;
                            }

                            dummy_node * allocDummy( const DUMMY_KEY& key )
                            {
                                return m_DummyFreeList.alloc( key ) ;
                            }

                            void free( value_ptr p )
                            {
                                if ( p->key().isDummy() )
                                    m_DummyFreeList.free( reinterpret_cast<dummy_node *>(p) )   ;
                                else
                                    m_RegularFreeList.free( p ) ;
                            }
                        };
                    };

                    typedef typename list_implementation::template rebind<
                        typename list_implementation::key_type,
                        typename list_implementation::value_type,
                        typename list_implementation::pair_traits,
                        type_traits
                    >::other        michael_list_type   ;
                };
            }   // namespace details

            template < typename REGULAR_KEY, typename DUMMY_KEY, typename VALUE, typename PAIR_TRAITS, typename TRAITS, class ALLOCATOR >
            class MichaelList<gc::tagged_gc, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                : public details::michael_list_tagged_type_selector< REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >::michael_list_type
            {
                typedef details::michael_list_tagged_type_selector< REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >  type_selector   ;
                typedef typename type_selector::michael_list_type   base_class   ;
            public:
                typedef typename type_selector::regular_node    regular_node    ;
                typedef typename type_selector::dummy_node      dummy_node     ;

                typedef typename base_class::tagged_node        node_ptr    ;
                typedef typename base_class::tagged_node&        node_ref    ;

            public:
                dummy_node *    allocDummy( const DUMMY_KEY& key )
                {
                    return base_class::m_FreeList.allocDummy( key ) ;
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
            struct impl_selector< michael_list_tag<gc::tagged_gc>, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
            {
                typedef MichaelList<gc::tagged_gc, REGULAR_KEY, DUMMY_KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR> type    ;
            };
        }   // namespace split_list
        //@endcond

    }    // namespace ordered_list
}    // namespace cds

#endif  // #ifndef CDS_DWORD_CAS_SUPPORTED

#endif // #ifndef __CDS_ORDERED_LIST_MICHAEL_LIST_TAGGED_H
