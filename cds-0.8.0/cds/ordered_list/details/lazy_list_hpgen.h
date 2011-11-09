/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ORDERED_LIST_LAZY_LIST_HPGEN_H
#define __CDS_ORDERED_LIST_LAZY_LIST_HPGEN_H

/*
    Generic implementation of LazyList class for HP-like garbage collectors:
        hzp_gc, hrc_gc, ptb_gc

    Editions:
        2010.09.19  Maxim Khiszinsky    Creation. Generalization current implementation of HP-like GC
*/

#include <cds/ordered_list/lazy_list.h>
#include <cds/concept.h>
#include <cds/ref.h>

//@cond
namespace cds { namespace ordered_list { namespace details { namespace lazy_list {

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
        node_type * m_pNode    ;

    protected:
        nc_iterator( node_type * pNode ): m_pNode( pNode )
        {
            skipDeleted()    ;
        }

        void skipDeleted()
        {
            assert( m_pNode != NULL )    ;
            while ( m_pNode->isMarked() )
                m_pNode = m_pNode->next()    ;
        }

        void next()
        {
            assert( m_pNode != NULL )    ;
            m_pNode = m_pNode->next()   ;
            if ( m_pNode != NULL )
                skipDeleted()    ;
        }

    public:
        /// Default constructor
        nc_iterator(): m_pNode( NULL ) {}
        /// Copy constructor
        nc_iterator( const nc_iterator& it ): m_pNode( it.m_pNode ) {}

        /// Returns const reference to key of node
        const key_type& key() const
        {
            assert( m_pNode != NULL )    ;
            return m_pNode->key()        ;
        }

        /// Returns reference (const reference for const iterator) to value of node
        value_reference value() const
        {
            assert( m_pNode != NULL )    ;
            return m_pNode->value()        ;
        }

        /// Return pointer to node. Debugging method
        const node_type * getNode() const { return m_pNode; }

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
        , public lazy_list_tag<GC>
    {
    protected:
        //@cond
        typedef typename LIST_IMPL::container_base   gc_base_class  ;
        typedef OrderedList< typename LIST_IMPL::node_type::key_type, typename LIST_IMPL::node_type::value_type, typename LIST_IMPL::type_traits >   ordlist_base_class  ;

        template <class LI, typename VALUE_REF> friend class nc_iterator ;
        //@endcond

    public:
        typedef typename ordlist_base_class::key_type   key_type    ;  ///< key type
        typedef typename ordlist_base_class::value_type value_type  ;  ///< value type

        typedef GC                                  gc_schema       ;   ///< Garbage collecing algorithm
        typedef typename LIST_IMPL::pair_traits     pair_traits     ;   ///< Pair (key, value) traits
        typedef typename pair_traits::key_traits    key_traits      ;   ///< Key traits (see cds::map::key_traits)
        typedef typename pair_traits::value_traits  value_traits    ;   ///< value traits (see cds::map::value_traits)
        typedef typename key_traits::key_comparator key_comparator  ;   ///< Key comparator

        typedef typename LIST_IMPL::type_traits     type_traits     ;   ///< Type traits (see cds::map::traits)

    protected:
        //@cond
        typedef typename LIST_IMPL::node_type   node_type   ;

        typedef typename gc_schema::thread_gc   thread_gc   ;
        typedef typename LIST_IMPL::auto_guard  gc_guard    ;
        typedef typename LIST_IMPL::guard_array guard_array ;

        struct position {
            node_type *     pPred   ;    ///< Previous node
            node_type *     pCur    ;    ///< Current node
            guard_array     hpArr   ;

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

            position( thread_gc& gc )
                : hpArr( gc )
            {}
        };

        class auto_lock_position {
            position&   m_pos   ;
        public:
            auto_lock_position( position& pos )
                : m_pos(pos)
            {
                pos.lock()      ;
            }
            ~auto_lock_position()
            {
                m_pos.unlock()  ;
            }
        };

        //@endcond

    protected:
        node_type *     m_pHead ;
        node_type *     m_pTail ;
        key_comparator  m_Cmp   ;

    public:
        /// Inserts new node
        /**
            Inserts new node <\p key, \p data> into the list.
            Returns \p false if key \p key already in list,\p true otherwise.
        */
        bool insert( const key_type& key, const value_type& data )
        {
            return insert( m_pHead, key, data )    ;
        }

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
                void func( VALUE& itemValue, const VALUE& val, bool bNew ) ;
            \endcode
            The first argument \p itemValue of user-defined function \p func is the reference
            to the list's item pointed by key \p key. The second argument \p val is the value passed
            to \p ensure member function. User-defined function \p func must guarantee that during changing
            key's value no any other changes could be made on this list's item by concurrent threads.
            The user-defined function is called only if the key exists in the list.

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

        /// Deletes the key \p key from the list
        /**
            Returns \p true if \p key existed in list and has been deleted, \p false otherwise
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


        /// Checks if the key is in list
        /**
            Returns \p true if key \p key exists in list, \p false otherwise
        */
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

        /// Checks if list is empty
        bool empty() const
        {
            assert( m_pHead != NULL )  ;
            return m_pHead->next() == m_pTail   ;
            //return atomics::load<membar_acquire>( &m_pHead->m_pNext ) == &m_pTail    ;
        }

    protected:
        //@cond
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
            nc_iterator_impl( node_type * node )
                : base_class( node )
            {}

            /*
            nc_iterator_impl( typename marked_node::pointer_type node )
                : base_class( node )
            {}
            */
        };
        //@endcond

    public:
        typedef nc_iterator_impl< value_type& >          nonconcurrent_iterator    ;    ///< Non-concurrent iterator
        typedef nc_iterator_impl< const value_type& >    nonconcurrent_const_iterator    ;  ///< Non-concurrent const iterator

        /// Returns non-concurrent const forward start iterator
        /**
            For an empty list start iterator is equal to \a end()
        */
        nonconcurrent_iterator    nc_begin()
        {
            return nonconcurrent_iterator( m_pHead->next() )    ;
        }

        /// Returns non-concurrent forward end iterator
        /**
            End iterator is simple NULL. You may use end iterator for comparison only
        */
        nonconcurrent_iterator    nc_end()
        {
            return nonconcurrent_iterator( m_pTail )    ;
        }

        /// Returns non-concurrent const forward start iterator
        /**
            For an empty list start iterator is equal to \a end()
        */
        nonconcurrent_const_iterator    nc_begin() const
        {
            return nonconcurrent_iterator( m_pHead->next() )    ;
        }

        /// Returns non-concurrent const forward end iterator
        /**
            End iterator is simple NULL. You may use end iterator for comparison only
        */
        nonconcurrent_const_iterator    nc_end() const
        {
            return nonconcurrent_iterator( m_pTail )    ;
        }

    protected:
        /// Inserts new node <\p key, \p val> to sublist starting with \p refHead
        bool insert( node_type * pHead, const key_type& key, const value_type& val )
        {
            node_type * pNew = allocNode( key, val )    ;
            if ( insert( pHead, pNew ))
                return true    ;
            freeNode( pNew )    ;
            return false        ;
        }

        template <typename T, typename FUNC>
        bool insert( node_type * pHead, const key_type& key, const T& val, FUNC func )
        {
            node_type * pNew = allocNode( key )    ;
            if ( insert( pHead, pNew, val, func ))
                return true    ;
            freeNode( pNew )    ;
            return false        ;
        }

        /// Inserts node \p pNode into the list
        bool insert( node_type * pNode )
        {
            return insert( m_pHead, pNode ) ;
        }

        /// Inserts new node \p pNew into the list starting with node \p pHead
        bool insert( node_type * pHead, node_type * pNew )
        {
            position pos( gc_base_class::getGC() )  ;

            while ( true ) {
                search( pHead, pNew->m_key, pos )    ;
                {
                    auto_lock_position alp(pos) ;

                    if ( LIST_IMPL::validate( pos.pPred, pos.pCur )) {
                        if ( pos.pCur != m_pTail && m_Cmp( pos.pCur->m_key, pNew->m_key ) == 0 ) {
                            // failed: key already in list
                            return false        ;
                        }
                        else {
                            LIST_IMPL::linkNode( pNew, pos.pPred, pos.pCur )    ;
                            return true        ;
                        }
                    }                
                }
            }
        }

        template <typename T, typename FUNC>
        bool insert( node_type * pHead, node_type * pNew, const T& val, FUNC func )
        {
            position pos( gc_base_class::getGC() )  ;

            while ( true ) {
                search( pHead, pNew->m_key, pos )    ;
                {
                    auto_lock_position alp(pos) ;

                    if ( LIST_IMPL::validate( pos.pPred, pos.pCur )) {
                        if ( pos.pCur != m_pTail && m_Cmp( pos.pCur->m_key, pNew->m_key ) == 0 ) {
                            // failed: key already in list
                            return false        ;
                        }
                        else {
                            unref(func)( pNew->value(), val )   ;
                            LIST_IMPL::linkNode( pNew, pos.pPred, pos.pCur )    ;
                            return true        ;
                        }
                    }                
                }
            }
        }

        /// Ensures that key \p key exists in sublist starting with \p refHead, changes data of existing item to \p val
        template <typename T, typename FUNC>
        std::pair<bool, bool> ensure( node_type * pHead, const key_type& key, const T& val, FUNC func )
        {
            position pos( gc_base_class::getGC() )  ;

            while ( true ) {
                search( pHead, key, pos )    ;
                {
                    auto_lock_position alp(pos) ;

                    if ( LIST_IMPL::validate( pos.pPred, pos.pCur )) {
                        if ( pos.pCur != m_pTail && m_Cmp( pos.pCur->m_key, key ) == 0 ) {
                            // key already in list
                            unref(func)( pos.pCur->value(), val, false )  ;
                            return std::make_pair( true, false )    ;
                        }
                        else {
                            node_type * pNewNode = allocNode( key ) ;
                            unref(func)( pNewNode->value(), val, true )    ;
                            LIST_IMPL::linkNode( pNewNode, pos.pPred, pos.pCur )    ;                            
                            return std::make_pair( true, true )     ;
                        }
                    }
                }
            }
        }

        /// Changes the value of its part for the list item pointed by \p key
        template <typename T, typename FUNC>
        bool emplace( node_type * pHead, const key_type& key, const T& val, FUNC func )
        {
            position pos( gc_base_class::getGC() )  ;

            while ( true ) {
                search( pHead, key, pos )    ;
                {
                    auto_lock_position alp(pos) ;
                    if ( LIST_IMPL::validate( pos.pPred, pos.pCur )) {
                        if ( pos.pCur != m_pTail && m_Cmp( pos.pCur->m_key, key ) == 0 ) {
                            // key already in list
                            func( pos.pCur->value(), val )  ;
                            return true     ;
                        }
                        else
                            return false    ;
                    }
                }
            }
        }

        /// Deletes \p key from list starting with node \p pHead
        bool erase( node_type * pHead, const key_type& key )
        {
            int n   ;
            return erase( pHead, key, n, trivial_erase_functor()) ;
            /*
            position pos( gc_base_class::getGC() )  ;

            while ( true ) {
                search( pHead, key, pos )    ;
                pos.lock()    ;

                if ( LIST_IMPL::validate( pos.pPred, pos.pCur )) {
                    if ( pos.pCur == m_pTail || m_Cmp( pos.pCur->m_key, key ) != 0 ) {
                        // key is not in list - no remove
                        pos.unlock()    ;
                        return false    ;
                    }
                    else {
                        // key found - remove it
                        LIST_IMPL::logicalRemove( pos.pCur )    ;
                        LIST_IMPL::linkNode( pos.pPred, pos.pCur->m_pNext ) ;   // physically remove
                        LIST_IMPL::linkNode( pos.pCur, pHead )    ;           // back-link for searching 

                        pos.unlock()        ;

                        LIST_IMPL::retirePtr( pos.hpArr.getGC(), pos.pCur, pos.hpArr[1] )   ;
                        return true            ;
                    }
                }
                pos.unlock() ;
            }
            */
        }

        template <typename T, typename FUNC>
        bool erase( node_type * pHead, const key_type& key, T& dest, FUNC func )
        {
            position pos( gc_base_class::getGC() )  ;

            while ( true ) {
                search( pHead, key, pos )    ;
                pos.lock()    ;

                if ( LIST_IMPL::validate( pos.pPred, pos.pCur )) {
                    if ( pos.pCur == m_pTail || m_Cmp( pos.pCur->m_key, key ) != 0 ) {
                        // key is not in list - no remove
                        pos.unlock()    ;
                        return false    ;
                    }
                    else {
                        // key found - remove it
                        LIST_IMPL::logicalRemove( pos.pCur )    ;
                        LIST_IMPL::linkNode( pos.pPred, pos.pCur->m_pNext ) ;   // physically remove
                        LIST_IMPL::linkNode( pos.pCur, pHead )    ;           // back-link for searching 
                        unref(func)( dest, pos.pCur->value() ) ;

                        pos.unlock()        ;

                        LIST_IMPL::retirePtr( pos.hpArr.getGC(), pos.pCur, pos.hpArr[1] )   ;
                        return true            ;
                    }
                }
                pos.unlock() ;
            }
        }

        /// Finds \p key in list starting with node \p pHead
        bool find( node_type * pHead, const key_type& key )
        {
            position pos( gc_base_class::getGC() )  ;
            search( pHead, key, pos ) ;
            return pos.pCur != m_pTail && !pos.pCur->isMarked() && m_Cmp( pos.pCur->m_key, key ) == 0 ;
        }

        /// Finds \p key in list starting with node \p pHead
        template <typename T, typename FUNC>
        bool find( node_type * pHead, const key_type& key, T& data, FUNC func )
        {
            position pos( gc_base_class::getGC() )  ;
            search( pHead, key, pos ) ;
            if ( pos.pCur != m_pTail && !pos.pCur->isMarked() && m_Cmp( pos.pCur->m_key, key ) == 0 ) {
                unref(func)( data, pos.pCur->m_data )  ;
                return true    ;
            }
            return false ;
        }


    protected:
        void search( const node_type * pHead, const key_type& key, position& pos ) const
        {
            const node_type * pTail = m_pTail    ;
            pos.pCur = pos.pPred = const_cast<node_type *>( pHead )    ;

            while ( pos.pCur != pTail && ( pos.pCur == pHead || m_Cmp( pos.pCur->m_key, key ) < 0 )) {
                pos.hpArr[0] =         // pos.pCur is already guarded by hazard pointer or is equal to pHead
                    pos.pPred = pos.pCur    ;

                node_type * p   ;
                do {
                    p = LIST_IMPL::getAndGuard( pos.pPred->m_pNext, pos.pCur, pos.hpArr[1] ) ;
                } while( p != pos.pCur )    ;
                //while ( LIST_IMPL::getAndGuard( pos.pPred->m_pNext, pos.pCur, pos.hpArr[1] ) != pos.pCur )   ;
                assert( pos.pCur != NULL ) ;
            }
        }
    };

}}}}    // namespace cds::ordered_list::details::lazy_list
//@endcond

#endif // #ifndef __CDS_ORDERED_LIST_LAZY_LIST_HPGEN_H
