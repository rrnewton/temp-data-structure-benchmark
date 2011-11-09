/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ORDERED_LIST_MICHAEL_LIST_HPGEN_H
#define __CDS_ORDERED_LIST_MICHAEL_LIST_HPGEN_H

/*
    Generic implementation of MichaelList class for HP-like garbage collectors:
        hzp_gc, hrc_gc, ptb_gc

    Editions:
        2010.09.09  Maxim Khiszinsky    Creation. Generalization current implementation of HP-like GC
*/

#include <cds/ordered_list/michael_list.h>
#include <cds/ref.h>

//@cond
namespace cds { namespace ordered_list { namespace details { namespace michael_list {

    template <
        typename GC
        , typename KEY
        , typename VALUE
        , typename PAIR_TRAITS
        , typename TRAITS
        , class ALLOCATOR
    >
    class adapter  ;


    template <class LIST_IMPL, typename VALUE_REF>
    class nc_iterator: public concept::non_concurrent_iterator< typename LIST_IMPL::value_type >
    {
    public:
        typedef typename LIST_IMPL::node_type  node_type       ;
        typedef typename LIST_IMPL::key_type   key_type        ;
        typedef typename LIST_IMPL::value_type value_type      ;
        typedef VALUE_REF                      value_reference ;

    protected:
        typedef typename LIST_IMPL::marked_node marked_node     ;

        node_type *     m_pNode ;

    protected:
        nc_iterator( const marked_node& node )
            : m_pNode( node.ptr() )
        {
            skipDeleted()    ;
        }

        nc_iterator( typename marked_node::pointer_type node )
            : m_pNode( node )
        {
            skipDeleted()   ;
        }

        void skipDeleted()
        {
            while ( m_pNode && m_pNode->isMarked() )
                m_pNode = m_pNode->next()    ;
        }

        void next()
        {
            if ( m_pNode )
                m_pNode = m_pNode->next()    ;
            skipDeleted()    ;
        }

    public:
        nc_iterator()
            : m_pNode( NULL )
        {}

        nc_iterator( const nc_iterator& it )
            : m_pNode( it.m_pNode )
        {}

        const key_type& key() const
        {
            assert( m_pNode != NULL ) ;
            return m_pNode->key()    ;
        }

        value_reference value() const
        {
            assert( m_pNode != NULL ) ;
            return m_pNode->value() ;
        }

        /// Return pointer to node. Debugging method
        const node_type * getNode() const
        {
            return m_pNode;
        }

        /// Pre-increment
        nc_iterator& operator ++()
        {
            assert( m_pNode != NULL )    ;
            next()    ;
            return *this                ;
        }

        /// Post-increment
        nc_iterator operator ++(int)
        {
            assert( m_pNode != NULL )    ;
            nc_iterator<LIST_IMPL, value_reference> it( *this )        ;
            next()                        ;
            return it                    ;
        }

        /// Iterator comparison
        bool operator ==( const nc_iterator<LIST_IMPL, value_reference>& it ) const { return m_pNode == it.m_pNode; }
        /// Iterator comparison
        bool operator !=( const nc_iterator<LIST_IMPL, value_reference>& it ) const { return !( *this == it ) ;}
    };

    template <
        class GC,
        class LIST_IMPL,
        class ALLOCATOR
    >
    class implementation:
        public LIST_IMPL::container_base
        , public OrderedList< typename LIST_IMPL::node_type::key_type, typename LIST_IMPL::node_type::value_type, typename LIST_IMPL::type_traits >
        , public michael_list_tag<GC>
    {
        typedef typename LIST_IMPL::container_base   gc_base_class  ;
        typedef OrderedList< typename LIST_IMPL::node_type::key_type, typename LIST_IMPL::node_type::value_type, typename LIST_IMPL::type_traits >   ordlist_base_class  ;

        template <class LI, typename VALUE_REF> friend class nc_iterator ;

    public:
        typedef typename ordlist_base_class::key_type        key_type        ;
        typedef typename ordlist_base_class::value_type        value_type        ;

        typedef GC                                  gc_schema       ;
        typedef typename LIST_IMPL::pair_traits     pair_traits     ;
        typedef typename pair_traits::key_traits    key_traits        ;
        typedef typename pair_traits::value_traits    value_traits    ;
        typedef typename key_traits::key_comparator    key_comparator    ;

        typedef typename LIST_IMPL::type_traits     type_traits     ;

    protected:
        typedef typename LIST_IMPL::node_type   node_type   ;
        typedef typename node_type::marked_node marked_node ;

        typedef typename gc_schema::thread_gc   thread_gc   ;
        typedef typename LIST_IMPL::auto_guard  gc_guard    ;
        typedef typename LIST_IMPL::guard_array guard_array ;

    protected:
        marked_node             m_pHead    ;        ///< List head pointer
        key_comparator            m_Cmp    ;        ///< Key comparator

        /// Position pointer for item search
        struct position {
            marked_node *   pPrev   ;   ///< Previous node
            node_type *     pCur    ;   ///< Current node
            node_type *     pNext   ;   ///< Next node
            guard_array     hpArr   ;   ///< Hazard pointer array

            //@cond
            position( thread_gc& gc )
                : hpArr( gc )
            {}
            //@endcond
        };

    public:
        implementation()
        {}

        ~implementation()
        {
            clear() ;
        }

    public:
        /// Inserts new node
        bool insert( const key_type& key, const value_type& val )
        {
            return insert( m_pHead, key, val )    ;
        }

        /// Inserts new node
        /**
            This function inserts new node with default-constructed value and then it calls 
            \p func functor with signature
            \code
                void func( VALUE& itemValue, const T& val ) ;
            \endcode
            The first argument \p itemValue of user-defined functor \p func is the reference
            to the list's item inserted. The second argument \p val is the value passed
            to \p insert member function. User-defined functor \p func must guarantee that during changing
            key's value no any other changes could be made on this list's item by concurrent threads.
            The user-defined functor is called only if the inserting is success.

            Note that \p VALUE type should be default-constructible. The functor \p func is called
            <u>after</u> inserting the new item; this means that there may be some period of time 
            when the new item has a default value.
        */
        template <typename T, typename FUNC>
        bool insert( const key_type& key, const T& val, FUNC func )
        {
            return insert( m_pHead, key, val, func )    ;
        }

        /// Ensures that key \p key exists in list, changes data of existing item to \p data
        /**
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

            If \p key is not in list then add it.

            Returns std::pair<bool, bool> where \p first is true if operation is successfull,
            \p second is true if new item has been added or \p false if the item with \p key
            already in list.
        */
        template <typename T, typename FUNC>
        std::pair<bool, bool> ensure( const key_type& key, const T& val, FUNC func )
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
        bool emplace( const key_type& key, const T& val, FUNC func )
        {
            return emplace( m_pHead, key, val, func )   ;
        }

        /// Delete \p key from the list. Return \p true if key has been deleted
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

        /// Find \p key in the list
        bool find( const key_type& key )
        {
            return find( m_pHead, key )    ;
        }

        /// Find \p key in the list. If key is found copies its data into \p data
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
            return find( m_pHead, key, data, func )    ;
        }

        /// Check is the list is empty
        bool empty() const
        {
            return m_pHead == NULL ;
        }

        /// Clears the list.
        void clear()
        {
            gc_guard    guard( gc_base_class::getGC() ) ;
            marked_node head ;
            while ( m_pHead.ptr() ) {
                if ( LIST_IMPL::getAndGuard( &m_pHead, head, guard ) == head.all() ) {
                    erase( head->key() )    ;
                }
            }
        }

    protected:
        template <class VALUE_REF>
        class nc_iterator_impl: public nc_iterator< implementation, VALUE_REF>
        {
            friend class implementation ;
            typedef nc_iterator< implementation, VALUE_REF> base_class  ;

        public:
            nc_iterator_impl()
            {}

            nc_iterator_impl( const base_class& it )
                : base_class( it )
            {}

        protected:
            nc_iterator_impl( const marked_node& node )
                : base_class( node )
            {}

            nc_iterator_impl( typename marked_node::pointer_type node )
                : base_class( node )
            {}
        };

    public:
        typedef nc_iterator_impl< value_type& >          nonconcurrent_iterator    ;
        typedef nc_iterator_impl< const value_type& >    nonconcurrent_const_iterator    ;

        nonconcurrent_iterator    nc_begin()
        {
            return nonconcurrent_iterator( m_pHead.ptr() )    ;
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
            return nonconcurrent_iterator( m_pHead.ptr() )    ;
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
        /// Allocate new node
        node_type * allocNode( const key_type& key, const value_type& val )
        {
            node_type * pNode = gc_base_class::allocNode( key, val )    ;
            assert( !cds::details::isMarkedBit( pNode ))            ;
            return pNode ;
        }

        /// Allocate new node
        node_type * allocNode( const key_type& key )
        {
            node_type * pNode = gc_base_class::allocNode( key ) ;
            assert( !cds::details::isMarkedBit( pNode ))        ;
            return pNode ;
        }

        /// Free node
        void   freeNode( node_type * pNode )
        {
            assert( !cds::details::isMarkedBit( pNode ))    ;
            gc_base_class::freeNode( pNode )    ;
        }

    protected:

        /// Inserts new node <\p key, \p val> to sublist starting with \p refHead
        bool insert( marked_node& refHead, const key_type& key, const value_type& val )
        {
            node_type * pNode = allocNode( key, val );
            if ( !insert( refHead, pNode )) {
                freeNode( pNode )    ;
                return false    ;
            }
            return true    ;
        }

        template <typename T, typename FUNC>
        bool insert( marked_node& refHead, const key_type& key, const T& val, FUNC func )
        {
            node_type * pNode = allocNode( key );
            if ( !insert( refHead, pNode, val, func )) {
                freeNode( pNode )    ;
                return false    ;
            }
            return true    ;
        }

        /// Inserts node \p pNode into the list
        bool insert( node_type * pNode )
        {
            return insert( m_pHead, pNode ) ;
        }

        /// Inserts node \p pNode to sublist starting with \p refHead
        bool insert( marked_node& refHead, node_type * pNode )
        {
            position pos( gc_base_class::getGC() )  ;

            while ( true ) {
                if ( search( refHead, pNode->key(), pos ) )
                    return false        ;
                if ( LIST_IMPL::linkNode( pNode, pos.pPrev, pos.pCur ))
                    return true ;
            }
        }

        template <typename T, typename FUNC>
        bool insert( marked_node& refHead, node_type * pNode, const T& val, FUNC func )
        {
            position pos( gc_base_class::getGC() )  ;

            while ( true ) {
                if ( search( refHead, pNode->key(), pos ) )
                    return false        ;

                gc_guard guard( gc_base_class::getGC() ) ;
                guard = pNode ;
                if ( LIST_IMPL::linkNode( pNode, pos.pPrev, pos.pCur )) {
                    // pNode is guarded, therefore, func call is safe
                    unref(func)( pNode->value(), val ) ;
                    return true ;
                }
            }
        }

        /// Ensures that key \p key exists in sublist starting with \p refHead, changes data of existing item to \p val
        template <typename T, typename FUNC>
        std::pair<bool, bool> ensure( marked_node& refHead, const key_type& key, const T& val, FUNC func )
        {
            node_type * pNode = allocNode( key );
            std::pair<bool,bool> bRet = ensure_at( refHead, pNode, val, func )    ;
            if ( !bRet.first || !bRet.second )
                freeNode( pNode )    ;
            return bRet    ;
        }

        /// Ensures that key \p key exists in sublist starting with \p refHead, changes data of existing item to \p data
        template <typename T, typename FUNC>
        std::pair<bool, bool> ensure_at( marked_node& refHead, node_type * pNode, const T& val, FUNC func )
        {
            position pos( gc_base_class::getGC() )  ;
            while ( true ) {
                if ( search( refHead, pNode->key(), pos )) {
                    // If pos.pCur is marked - try again
                    if ( pos.pCur->isMarked() )
                        continue    ;

                    // pos.pCur is protected
                    unref(func)( pos.pCur->value(), val, false )   ;

                    return std::make_pair( true, false)     ;
                }
                else {
                    gc_guard  guard( gc_base_class::getGC() ) ;
                    guard = pNode   ;
                    if ( LIST_IMPL::linkNode( pNode, pos.pPrev, pos.pCur )) {
                        // pNode is guarded, so, func call is safe
                        unref(func)( pNode->value(), val, true )    ;
                        return std::make_pair( true, true )     ;
                    }
                }
            }
        }

        /// Changes the value of its part for the list item pointed by \p key
        template <typename T, typename FUNC>
        bool emplace( marked_node& refHead, const key_type& key, const T& val, FUNC func )
        {
            position pos( gc_base_class::getGC() )  ;
            if ( search( refHead, key, pos )) {
                // pos.pCur is protected by Hazard Pointer.
                unref(func)( pos.pCur->value(), val ) ;
                return true     ;
            }
            return false    ;
        }

        /// Delete item with \p key
        bool erase( marked_node& refHead, const key_type& key )
        {
            int n   ;
            return erase( refHead, key, n, trivial_erase_functor()) ;
            /*
            position pos(gc_base_class::getGC())    ;
            while ( true ) {
                if ( !search( refHead, key, pos ))
                    return false ;
                assert( m_Cmp( pos.pCur->key(), key ) == 0 ) ;

                // Logical deletion - mark pCur
                if (!LIST_IMPL::logicalDeletion( &(pos.pCur->m_pNext), pos.pNext ))
                    continue ;

                // Physical deletion
                if ( LIST_IMPL::physicalDeletion( pos.pPrev, marked_node( pos.pCur ), marked_node( pos.pNext ) ))
                    LIST_IMPL::retirePtr( pos.hpArr.getGC(), pos.pCur, pos.hpArr[1] )   ;
                else
                    search( refHead, key, pos ) ;

                return true    ;
            }
            */
        }

        template <typename T, typename FUNC>
        bool erase( marked_node& refHead, const key_type& key, T& dest, FUNC func )
        {
            position pos(gc_base_class::getGC())    ;
            while ( true ) {
                if ( !search( refHead, key, pos ))
                    return false ;
                assert( m_Cmp( pos.pCur->key(), key ) == 0 ) ;

                // Logical deletion - mark pCur
                if (!LIST_IMPL::logicalDeletion( &(pos.pCur->m_pNext), pos.pNext ))
                    continue ;

                // Physical deletion
                if ( LIST_IMPL::physicalDeletion( pos.pPrev, marked_node( pos.pCur ), marked_node( pos.pNext ) )) {
                    // Call user-provided functor
                    // func call is safe because pos.pCur is guarded
                    unref(func)( dest, pos.pCur->m_value ) ;

                    LIST_IMPL::retirePtr( pos.hpArr.getGC(), pos.pCur, pos.hpArr[1] )   ;
                }
                else
                    search( refHead, key, pos ) ;

                return true    ;
            }
        }


        /// Check if \p key is in list starting from \p refHead
        bool find( marked_node& refHead, const key_type& key )
        {
            position pos(gc_base_class::getGC()) ;
            return search( refHead, key, pos ) ;
        }

        /// Check if \p key is in list starting from \p refHead. If item is found returns copy of its data in \p data
        template <typename T, typename FUNC>
        bool find( marked_node& refHead, const key_type& key, T& data, FUNC func )
        {
            position pos(gc_base_class::getGC()) ;
            if ( search( refHead, key, pos )) {
                assert( pos.pCur != NULL )    ;
                // The node found cannot be deleted because data is being protected by HP
                unref(func)( data, pos.pCur->value() ) ;
                return true    ;
            }
            return false    ;
        }

    protected:
        bool search( marked_node& refHead, const key_type& key, position& pos )
        {
            marked_node * pPrev ;
            marked_node pNext   ;
            marked_node pCur    ;

try_again:
            pPrev = &refHead        ;
            pNext = NULL            ;
            if ( LIST_IMPL::getAndGuard( pPrev, pCur, pos.hpArr[1] ) != pCur.ptr() )
                goto try_again  ;

            while ( true ) {
                if ( pCur.isNull() ) {
                    pos.pPrev = pPrev    ;
                    pos.pCur = pCur.ptr()        ;
                    pos.pNext = pNext.ptr()        ;
                    return false        ;
                }
                if ( LIST_IMPL::getAndGuard( &(pCur->m_pNext), pNext, pos.hpArr[0]) != pNext.all())
                    goto try_again ;

                if ( pPrev->all() != pCur.ptr() )
                    goto try_again    ;

                // pNext contains deletion mark of pCur
                if ( pNext.isMarked() ) {
                    if ( LIST_IMPL::physicalDeletion( pPrev, marked_node( pCur.ptr() ), marked_node( pNext.ptr()) ))
                        LIST_IMPL::retirePtr( pos.hpArr.getGC(), pCur.ptr(), pos.hpArr[1] ) ;
                    else
                        goto try_again  ;
                }
                else {
                    int nCmp = m_Cmp( pCur->key(), key ) ;
                    if ( nCmp >= 0 ) {
                        pos.pPrev = pPrev    ;
                        pos.pCur = pCur.ptr()    ;
                        pos.pNext = pNext.ptr()    ;
                        return nCmp == 0    ;
                    }
                    pPrev = &( pCur->m_pNext ) ;
                    pos.hpArr[2] = pCur.ptr() ;
                }
                pCur = pNext ;
                pos.hpArr[1] = pNext.ptr() ;
            }
        }
    };


}}}} // namespace cds::ordered_list::details::michael_list
//@endcond

#endif  // #ifndef __CDS_ORDERED_LIST_MICHAEL_LIST_HPGEN_H
