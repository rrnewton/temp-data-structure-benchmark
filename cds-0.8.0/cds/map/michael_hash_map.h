/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_MAP_MICHAEL_HASH_MAP_H
#define __CDS_MAP_MICHAEL_HASH_MAP_H

#include <cds/map/_base.h>

#include <cds/ordered_list/michael_list_hzp.h>
#include <cds/concept.h>
#include <cds/numtraits.h>
#include <cds/gc/tagged_gc.h>
#include <cds/gc/no_gc.h>

#include <ostream>      // for dumping
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/remove_reference.hpp>

namespace cds {
    namespace map {

        /// Details of \p MichaelHashMap implementation
        namespace michael_map {

            /// Michael's hash map implementation details
            namespace details {
                /// Helper class to define bucket and GC schema implementations
                template <
                    typename KEY,
                    typename VALUE,
                    typename PAIR_TRAITS,
                    typename TRAITS,
                    class ALLOCATOR
                >
                class bucket_selector
                {
                    //@cond
                    typedef map_base< KEY, VALUE, PAIR_TRAITS, TRAITS >   map_defaults ;
                    //@endcond
                public:
                    /// Bucket implementation (ordered list with GC)
                    typedef typename cds::details::void_selector<
                        typename map_defaults::type_traits::bucket_type,
                        ordered_list::MichaelList<
                            gc::hzp_gc,
                            typename map_defaults::key_type,
                            typename map_defaults::value_type,
                            typename map_defaults::pair_traits,
                            void,
                            ALLOCATOR
                        >
                    >::type::template rebind< KEY, VALUE, typename map_defaults::pair_traits>::other    bucket_type ;

                    typedef typename bucket_type::gc_schema                gc_schema    ;    ///< Memory reclamation schema
                } ;

                /// Hash table implementation for MichaelHashMap
                template <
                    class BUCKET_TYPE,      ///< Bucket type
                    class ITEM_COUNTER,     ///< Item counter type
                    class ALLOCATOR         ///< Memory allocator
                >
                class hash_table_base
                {
                public:
                    typedef BUCKET_TYPE     bucket_type         ;   ///< Bucket type. One of ordered list class
                    typedef ITEM_COUNTER    item_counter_type   ;   ///< Class for item counting.

                    item_counter_type    m_ItemCounter    ;    ///< item count
                protected:
                    bucket_type    *    m_arrBuckets    ;    ///< Bucket array
                    const size_t    m_nSize            ;    ///< Size of bucket array

                    //@cond
                    typedef cds::details::Allocator< bucket_type, ALLOCATOR >    table_allocator    ;
                    //@endcond

                public:
                    /// Initializes hash table
                    hash_table_base(
                        size_t nBucketCount     ///< Bucket count (== hash table size)
                        )
                        : m_nSize( nBucketCount )
                    {}

                    ~hash_table_base()
                    {}

                    /// Create bucket array
                    void create()
                    {
                        table_allocator a    ;
                        m_arrBuckets = a.NewArray( m_nSize )    ;
                    }

                    /// Destroy buckets
                    void destroy()
                    {
                        table_allocator a    ;
                        a.Delete( m_arrBuckets, m_nSize )    ;
                    }

                    /// Get bucket by index
                    bucket_type& operator []( size_t nIndex )
                    {
                        assert( nIndex < m_nSize );
                        return m_arrBuckets[ nIndex ] ;
                    }

                    /// Size of hash table (count of buckets that is constant value)
                    size_t    size() const        { return m_nSize; }

                    //@cond
                    bucket_type *   begin() { return m_arrBuckets ;}
                    bucket_type *   begin() const { return m_arrBuckets ;}
                    bucket_type *   end() { return m_arrBuckets + size();}
                    bucket_type *   end() const { return m_arrBuckets + size();}
                    //@endcond
                };
            }   // namespace details

            /// Hash table for MichaelHashMap
            template <
                typename GC,            ///< Garbage collector schema
                typename IMPL_SELECTOR, ///< Implementation selector (free list separated for each bucket or shared for all buckets)
                class BUCKET_TYPE,      ///< Bucket type
                class ITEM_COUNTER,     ///< Item counter type
                class ALLOCATOR         ///< Memory allocator
            >
            class hash_table: public details::hash_table_base< BUCKET_TYPE, ITEM_COUNTER, ALLOCATOR >
            {
                //@cond
                typedef details::hash_table_base< BUCKET_TYPE, ITEM_COUNTER, ALLOCATOR >   base_class ;
                //@endcond
            public:
                /// Hash table constructor
                hash_table(
                    size_t nBucketCount     ///< Bucket count
                    )
                    : base_class( nBucketCount )
                {}
            };

#ifdef CDS_DWORD_CAS_SUPPORTED
            /// Hash table specialization for gc::tagged_gc GC
            /**
                This specialization is intended for supporting separated free list object for each bucket
            */
            template <
                class BUCKET_TYPE,      ///< Bucket type
                class ITEM_COUNTER,     ///< Item counter type
                class ALLOCATOR         ///< Memory allocator
            >
            class hash_table< gc::tagged_gc, boost::false_type, BUCKET_TYPE, ITEM_COUNTER, ALLOCATOR >:
                public details::hash_table_base< BUCKET_TYPE, ITEM_COUNTER, ALLOCATOR >
            {
                //@cond
                typedef details::hash_table_base< BUCKET_TYPE, ITEM_COUNTER, ALLOCATOR >   base_class ;
                //@endcond
            public:
                //@cond
                typedef typename base_class::bucket_type bucket_type ;
                //@endcond

            public:
                /// Hash table constructor
                hash_table(
                    size_t nBucketCount     ///< Bucket count
                    )
                    : base_class( nBucketCount )
                {
                    CDS_STATIC_ASSERT( !bucket_type::type_traits::is_freelist_ref::value )    ;
                }
            };

            /// Hash table specialization for gc::tagged_gc GC
            /**
                This specialization is intended for supporting free list shared for all buckets
            */
            template <
                class BUCKET_TYPE,      ///< Bucket type
                class ITEM_COUNTER,     ///< Item counter type
                class ALLOCATOR         ///< Memory allocator
            >
            class hash_table< gc::tagged_gc, boost::true_type, BUCKET_TYPE, ITEM_COUNTER, ALLOCATOR >:
                public details::hash_table_base< BUCKET_TYPE, ITEM_COUNTER, ALLOCATOR >
            {
                //@cond
                typedef details::hash_table_base< BUCKET_TYPE, ITEM_COUNTER, ALLOCATOR >   base_class ;
                //@endcond
            public:
                //@cond
                typedef typename base_class::bucket_type                    bucket_type     ;
                typedef typename bucket_type::type_traits::free_list_type   freelist_type   ;
                //@endcond

            protected:
                //@cond
                typename bucket_type::type_traits::freelist_noref   m_FreeList  ;
                //@endcond

            public:
                /// Hash table ctor
                hash_table(
                    size_t nBucketCount     ///< Bucket count
                    )
                    : base_class( nBucketCount )
                {
                    CDS_STATIC_ASSERT( bucket_type::type_traits::is_freelist_ref::value )    ;
                }

                //@cond
                void create()
                {
                    typedef typename ALLOCATOR::template rebind< bucket_type >::other    table_allocator ;
                    table_allocator a   ;
                    base_class::m_arrBuckets = a.allocate( base_class::m_nSize )  ;
                    for ( size_t i = 0; i < base_class::m_nSize; ++i )
                        new( base_class::m_arrBuckets + i ) bucket_type( m_FreeList )   ;
                }
                //@endcond
            };
#endif  // #ifdef CDS_DWORD_CAS_SUPPORTED

            namespace details {

                /// Base of Michael's hash map
                template <
                    typename KEY,
                    typename VALUE,
                    typename PAIR_TRAITS,
                    typename TRAITS,
                    class ALLOCATOR
                >
                class michael_map_base: public map_base<KEY, VALUE, PAIR_TRAITS, TRAITS>
                {
                    //@cond
                    typedef map_base<KEY, VALUE, PAIR_TRAITS, TRAITS>    base_class    ;
                    //@endcond

                public:
                    typedef typename base_class::key_type        key_type    ;    ///< key type
                    typedef typename base_class::value_type        value_type    ;    ///< value_type
                    typedef typename base_class::pair_traits    pair_traits    ;    ///< pair traits

                protected:
                    //@cond
                    typedef typename michael_map::details::bucket_selector<
                        key_type,
                        value_type,
                        pair_traits,
                        typename base_class::type_traits,
                        ALLOCATOR
                    >       bucket_selector ;
                    //@endcond

                public:

                    /// Map traits
                    struct type_traits: public base_class::type_traits
                    {
                        typedef typename base_class::type_traits        original    ;   ///< original type traits

                        /// Bucket implementation (ordered list with GC)
                        typedef typename bucket_selector::bucket_type   bucket_type  ;

                        /// Item counter implementation. cds::atomics::empty_item_counter is invalid implementation for this hash map
                        typedef typename boost::mpl::if_<
                            boost::is_same< typename base_class::item_counter_type, atomics::empty_item_counter >,
                            atomics::item_counter<>,
                            typename base_class::item_counter_type
                        >::type                                     item_counter_type;
                    } ;

                    typedef typename type_traits::bucket_type   bucket_type         ;   ///< Bucket implementation (ordered list with GC)
                    typedef bucket_type                         ordered_list_type   ; ///< Ordered list implementation (same as bucket_type)
                    typedef typename bucket_selector::gc_schema gc_schema           ;    ///< Memory reclamation schema

                    typedef typename type_traits::item_counter_type item_counter_type   ;   ///< Item counter implementation

                    typedef typename type_traits::statistics    statistics    ;    ///< Internal statistics implementation

                protected:
                    /// Non-concurrent iterator.
                    /**
                        This iterator is not thread safe.
                        Template parameter \p VALUE_REF defines type of iterator return value (const or not)
                        Template parameter \p ITERATOR defines iterator class of underlined ordered list.
                    */
                    template <typename VALUE_REF, typename ITERATOR>
                    class NonConcurrentIterator: public concept::non_concurrent_iterator< value_type >
                    {
                        friend class        michael_map_base;
                    public:
                        typedef ITERATOR    list_nonconcurrent_iterator    ;   ///< iterator type
                        typedef typename list_nonconcurrent_iterator::node_type node_type       ; ///< internal node type
                        typedef typename michael_map_base::key_type             key_type        ; ///< key type
                        typedef typename michael_map_base::value_type           value_type      ; ///< value type
                        typedef VALUE_REF                                       value_reference ; ///< type of reference to value

                    protected:
                        //@cond
                        bucket_type *               m_pBucket           ;
                        list_nonconcurrent_iterator m_BucketIterator    ;
                        bucket_type *               m_pEndBucket        ;
                        //@endcond

                    protected:
                        //@cond
                        NonConcurrentIterator( bucket_type * pBucket, bucket_type * pEnd )
                            : m_pBucket( pBucket )
                            , m_pEndBucket( pEnd )
                        {
                            while ( m_pBucket != pEnd && m_pBucket->empty() )
                                ++m_pBucket   ;
                            if ( m_pBucket != pEnd )
                                m_BucketIterator = m_pBucket->nc_begin()    ;
                        }

                        void next()
                        {
                            if ( m_pBucket != m_pEndBucket ) {
                                if ( !m_pBucket->empty()) {
                                    if ( ++m_BucketIterator != m_pBucket->nc_end() )
                                        return  ;
                                }
                                do {
                                    ++m_pBucket ;
                                } while ( m_pBucket != m_pEndBucket && m_pBucket->empty()) ;
                                if ( m_pBucket != m_pEndBucket )
                                    m_BucketIterator = m_pBucket->nc_begin() ;
                            }
                        }
                        //@endcond

                    public:
                        /// Copy ctor
                        NonConcurrentIterator( const NonConcurrentIterator<VALUE_REF, ITERATOR>& it )
                            : m_pBucket( it.m_pBucket )
                            , m_BucketIterator( it.m_BucketIterator )
                            , m_pEndBucket( it.m_pEndBucket )
                        {}

                        /// Returns const reference to key of node
                        const key_type& key() const
                        {
                            assert( m_pBucket != m_pEndBucket )    ;
                            assert( m_BucketIterator != m_pBucket->nc_end())   ;
                            return m_BucketIterator.key() ;
                        }

                        /// Returns reference (const reference for const iterator) to value of node
                        VALUE_REF value() const
                        {
                            assert( m_pBucket != m_pEndBucket )    ;
                            assert( !m_pBucket->empty() )       ;
                            return m_BucketIterator.value()        ;
                        }

                        /// Return pointer to node. Debugging method
                        const node_type * getNode() const
                        {
                            assert( m_pBucket != m_pEndBucket )    ;
                            assert( !m_pBucket->empty() )       ;
                            return m_BucketIterator.getNode()        ;
                        }

                        /// Pre-increment
                        NonConcurrentIterator& operator ++()
                        {
                            assert( m_pBucket != m_pEndBucket )    ;
                            next()    ;
                            return *this                    ;
                        }

                        /// Post-increment
                        NonConcurrentIterator operator ++(int)
                        {
                            assert( m_pBucket != m_pEndBucket )    ;
                            NonConcurrentIterator<VALUE_REF, ITERATOR> it( *this )        ;
                            next()                        ;
                            return it                    ;
                        }

                        /// Iterator comparison
                        bool operator ==( const NonConcurrentIterator<VALUE_REF, ITERATOR>& it ) const
                        {
                            return m_pBucket == it.m_pBucket && ( m_pBucket == m_pEndBucket || m_BucketIterator == it.m_BucketIterator ) ;
                        }
                        /// Iterator comparison
                        bool operator !=( const NonConcurrentIterator<VALUE_REF, ITERATOR>& it ) const { return !( *this == it ) ;}
                    };

                private:
                    ///@cond none
                    // Give compiler a chance to select appropriate hash table specialization
                    typedef michael_map::hash_table< gc_schema,
                        typename boost::mpl::if_<
                            boost::is_same< gc_schema, gc::tagged_gc >,
                            typename boost::is_reference< typename bucket_type::type_traits::free_list_type >::type,
                            void
                        >::type,
                        bucket_type,
                        item_counter_type,
                        ALLOCATOR
                    >         hash_table_type ;
                    ///@endcond

                protected:
                    typename base_class::hash_functor    m_funcHash     ;    ///< hash function
                    const size_t                    m_nMask             ;    ///< mask for hash value ( 2**N - 1)
                    hash_table_type                 m_HashTable         ;    ///< hash table
                    statistics                      m_Statistics        ;    ///< Internal statistics

                protected:
                    /// Calculate mask for hash value
                    static size_t calcMask( size_t nItemCount, size_t nLoadFactor )
                    {
                        assert( nLoadFactor != 0 )    ;
                        if ( nLoadFactor == 0 )
                            nLoadFactor = 1 ;
                        return ( 1 << cds::beans::exp2Ceil( (size_t)( nItemCount / nLoadFactor )) ) - 1    ;
                    }

                    /// Returns hash value for \p key
                    size_t            hashValue( const KEY& key ) const
                    {
                        return m_funcHash( key ) & m_nMask ;
                    }

                public:
                    /// Explicit bucket count definition. \p nBucketCount must be power of 2.
                    michael_map_base( size_t nBucketCount = 1024 )
                        : m_nMask( nBucketCount - 1 )
                        , m_HashTable( nBucketCount )
                    {
                        // nBucketCount must be power of 2: nBucketCount == 2 ** N
                        assert( cds::beans::isExp2( nBucketCount ) ) ;
                        m_HashTable.create() ;
                    }

                    /// Calculate bucket count according to parameter passed
                    michael_map_base(
                        size_t nMaxItemCount,        ///< estimation of max item count in the hash map
                        size_t nLoadFactor            ///< load factor: estimation of max number of items in the bucket
                    ):     m_nMask( calcMask( nMaxItemCount, nLoadFactor ) )
                        , m_HashTable( m_nMask + 1 )
                    {
                        // m_nMask + 1 must be power of 2
                        assert( cds::beans::isExp2( m_nMask + 1 ) ) ;
                        m_HashTable.create() ;
                    }

                    /// Destructs the map
                    ~michael_map_base()
                    {
                        clear() ;
                        m_HashTable.destroy() ;
                    }

                    /// Returns current bucket count
                    /**
                        Michael's hash map cannot dynamically extend the size of hash array.
                        Therefore, bucket_count() == max_bucket_count()
                    */
                    size_t bucket_count() const            { return m_nMask + 1 ; }

                    /// Returns maximum bucket count
                    /**
                        Michael's hash map cannot dynamically extend the size of hash array.
                        Therefore, bucket_count() == max_bucket_count()
                    */
                    size_t max_bucket_count() const        { return m_nMask + 1 ; }

                public:
                    /// Inserts pair [\p key, \p val] into the map.
                    bool insert( const KEY& key, const VALUE& val )
                    {
                        bool bRet = m_HashTable[ hashValue( key ) ].insert( key, val ) ;
                        if ( bRet )
                            ++m_HashTable.m_ItemCounter ;
                        return bRet ;
                    }

                    template <typename T, typename FUNC>
                    bool insert( const KEY& key, const T& val, FUNC func )
                    {
                        bool bRet = m_HashTable[ hashValue( key ) ].insert( key, val, func ) ;
                        if ( bRet )
                            ++m_HashTable.m_ItemCounter ;
                        return bRet ;
                    }

                    /// Ensures that key \p key exists in the map
                    /**
                        If \p key exists in the map then changes the value of the item pointed by \p key to \p val.
                        The change is performed by calling user-defined function \p func with signature:
                        \code
                            void func( VALUE& itemValue, const VALUE& val ) ;
                        \endcode
                        The first argument \p itemValue of user-defined function \p func is the reference
                        to the map's item pointed by key \p key. The second argument \p val is the value passed
                        to \p ensure member function. User-defined function \p func must guarantee that during changing
                        key's value no any other changes could be made on this map item by concurrent threads.
                        The user-defined function is called only if the key exists in the map.

                        If \p key is not in map then add it.

                        Returns std::pair<bool, bool> where \p first is true if the operation is successfull,
                        \p second is true if new item has been added or \p false if the item with \p key
                        already in map.
                    */
                    template <typename T, typename FUNC>
                    std::pair<bool, bool> ensure( const KEY& key, const T& val, FUNC func )
                    {
                        std::pair<bool, bool> bRet = m_HashTable[ hashValue( key ) ].ensure( key, val, func ) ;
                        if ( bRet.first && bRet.second )
                            ++m_HashTable.m_ItemCounter ;
                        return bRet ;
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
                        to the map's item pointed by key \p key. The second argument \p val is the value passed
                        to \p emplace member function. User-defined function \p func must guarantee that during changing
                        key's value no any other changes could be made on this map's item by concurrent threads.
                        The list only guarantees that the item found by key cannot be deleted while \p func working

                        The function returns \p true if the key \p key exists in the map and \p false otherwise
                    */
                    template <typename T, typename FUNC>
                    bool emplace( const KEY& key, const T& val, FUNC func )
                    {
                        return m_HashTable[ hashValue( key ) ].emplace( key, val, func ) ;
                    }

                    /// Checks if the key \p key exists in the map
                    bool find( const KEY& key )
                    {
                        return m_HashTable[ hashValue( key ) ].find( key )    ;
                    }

                    /// Checks if the map is empty
                    /**
                        The map's emptiness is determined by the value of the item counter.
                        If dummy item counter is used (for example, cds::atomics::empty_item_counter)
                        the result of the function may be \p true for non-empty map.
                    */
                    bool empty() const
                    {
                        return m_HashTable.m_ItemCounter.value() == 0 ;
                    }

                    /// Returns item count in the map
                    /**
                        The return value of the method depends on item counter implementation.
                        The item counter is defined in TRAITS::item_counter. If TRAITS::item_counter_type is \p void
                        then dummy item counter cds::atomics::empty_item_counter is used  and this function returns
                        dummy value (0 usually).
                    */
                    size_t size() const
                    {
                        return m_HashTable.m_ItemCounter.value() ;
                    }

                    /// Deletes all items in the map. 
                    /**
                        This method is thread-safe but it is indeterminate in multi-threading environment.
                        The resulting map will be empty only if there are no parallel threads inserting into the map.
                    */
                    void clear()
                    {
                        for ( size_t i = 0; i < m_HashTable.size(); ++i )
                            m_HashTable[i].clear()    ;
                        m_HashTable.m_ItemCounter.reset()    ;
                    }

                public:
                    ///@name Non-concurrent iterators
                    //@{

                    /// Non-concurrent iterator
                    /**
                        This iterator is not thread-safe.
                    */
                    typedef NonConcurrentIterator<
                        value_type&,
                        typename ordered_list_type::nonconcurrent_iterator
                    >        nonconcurrent_iterator    ;

                    /// Non-concurrent iterator
                    /**
                        This iterator is not thread-safe.
                    */
                    typedef NonConcurrentIterator<
                        const value_type&,
                        typename ordered_list_type::nonconcurrent_const_iterator
                    >    nonconcurrent_const_iterator    ;

                    /// Returns non-concurrent forward start iterator
                    /**
                        For an empty list start iterator is equal to \a end().
                    */
                    nonconcurrent_iterator    nc_begin()
                    {
                        return nonconcurrent_iterator( m_HashTable.begin(), m_HashTable.end() )    ;
                    }

                    /// Returns non-concurrent forward end iterator
                    /**
                        End iterator points to tail dummy node. You may use end iterator for comparison only
                    */
                    nonconcurrent_iterator    nc_end()
                    {
                        return nonconcurrent_iterator( m_HashTable.end(), m_HashTable.end() )         ;
                    }

                    /// Returns non-concurrent const forward start iterator
                    /**
                        For an empty list start iterator is equal to \a end()
                    */
                    nonconcurrent_const_iterator    nc_begin() const
                    {
                        return nonconcurrent_const_iterator( m_HashTable.begin(), m_HashTable.end() )    ;
                    }

                    /// Returns non-concurrent const forward end iterator
                    /**
                        End iterator points to tail dummy node. You may use end iterator for comparison only
                    */
                    nonconcurrent_const_iterator    nc_end() const
                    {
                        return nonconcurrent_const_iterator( m_HashTable.end(), m_HashTable.end() )         ;
                    }

                    //@}

                public:
                    ///@name Debug methods. NOT thread safe!!!
                    //@{

                    /// Returns internal statistics
                    /**
                        Internal statistics struct is defined in \p TRAITS::statistics.
                    */
                    const statistics& getStatistics() const
                    {
                        return m_Statistics    ;
                    }

                    /// Dumping the internal content of split list to stream \p stm. Debugging method
                    /**
                        Not implemented
                    */
                    std::ostream&       dump( std::ostream& stm )
                    {
                        return stm ;
                    }

                    //@}
                };

                /// Michael's hash map implementation with support of item removing
                template <typename GC,
                    typename KEY,
                    typename VALUE,
                    typename PAIR_TRAITS,
                    typename TRAITS,
                    class ALLOCATOR
                >
                class michael_map_gc: public michael_map_base< KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                {
                    //@cond
                    typedef michael_map_base< KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR> base_class ;
                    //@endcond
                public:
                    /// Explicit bucket count definition. \p nBucketCount must be power of 2.
                    michael_map_gc( size_t nBucketCount = 1024 )
                        : base_class( nBucketCount )
                    {}

                    /// Calculate bucket count according to parameter passed
                    michael_map_gc(
                        size_t nMaxItemCount,        ///< estimation of max item count in the hash map
                        size_t nLoadFactor            ///< load factor: estimation of max number of items in the bucket
                        )
                        : base_class( nMaxItemCount, nLoadFactor )
                    {}

                public:

                    /// Erases \p key from the map
                    bool erase( const KEY& key )
                    {
                        int n   ;
                        ordered_list::details::trivial_erase_functor f ;
                        return erase( key, n, f )   ;
                        /*
                        bool bRet = base_class::m_HashTable[ base_class::hashValue( key ) ].erase( key )    ;
                        if ( bRet )
                            --base_class::m_HashTable.m_ItemCounter ;
                        return bRet    ;
                        */
                    }

                    /// Erases \p key from the map
                    template <typename T, typename FUNC>
                    bool erase( const KEY& key, T& dest, FUNC func )
                    {
                        bool bRet = base_class::m_HashTable[ base_class::hashValue( key ) ].erase( key, dest, func )    ;
                        if ( bRet )
                            --base_class::m_HashTable.m_ItemCounter ;
                        return bRet    ;
                    }

                    /// Checks if the key \p key exists in the map
                    bool find( const KEY& key )
                    {
                        return base_class::find( key )  ;
                    }

                    /// Finds the key \p key and returns its data in \p data
                    /**
                        If \p key found the function calls user-defined function \p func with parameters:
                        \code
                            void func( T& data, const VALUE& itemValue )
                        \endcode
                        where \p itemValue is the item found by \p key. The user-defined function
                        copies the item's value \p itemValue or its part to \p data. The map guarantees only
                        that the item found cannot be deleted while \p func works. The user-defined function \p func
                        should take into account that concurrent threads may change the item value.

                        Returns \p true if the key is found, \p false otherwise
                    */
                    template <typename T, typename FUNC>
                    bool find( const KEY& key, T& data, FUNC func )
                    {
                        return base_class::m_HashTable[ base_class::hashValue( key ) ].find( key, data, func )    ;
                    }
                };

                /// Michael's hash map specialization without support of item removing
                template < typename KEY,
                    typename VALUE,
                    typename PAIR_TRAITS,
                    typename TRAITS,
                    class ALLOCATOR
                >
                class michael_map_gc<gc::no_gc, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                    : public michael_map_base< KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                {
                    //@cond
                    typedef michael_map_base< KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR> base_class ;
                    //@endcond
                public:
                    /// Explicit bucket count definition. \p nBucketCount must be power of 2.
                    michael_map_gc( size_t nBucketCount = 1024 )
                        : base_class( nBucketCount )
                    {}

                    /// Calculate bucket count according to parameter passed
                    michael_map_gc(
                        size_t nMaxItemCount,        ///< estimation of max item count in the hash map
                        size_t nLoadFactor            ///< load factor: estimation of max number of items in the bucket
                        )
                        : base_class( nMaxItemCount, nLoadFactor )
                    {}

                public:
                    /// Returns pointer to the value of \p key, NULL if \p key is not found
                    VALUE * get( const KEY& key )
                    {
                        return base_class::m_HashTable[ base_class::hashValue( key ) ].get( key )    ;
                    }
                };

                //@cond
                template <
                    typename KEY,
                    typename VALUE,
                    typename PAIR_TRAITS,
                    typename TRAITS,
                    class ALLOCATOR
                >
                class michael_map_selector
                {
                    typedef typename michael_map::details::bucket_selector<
                        KEY,
                        VALUE,
                        PAIR_TRAITS,
                        TRAITS,
                        ALLOCATOR
                    >::gc_schema       gc_type ;

                public:
                    typedef michael_map_gc<
                        gc_type,
                        KEY, VALUE, PAIR_TRAITS,
                        TRAITS,
                        ALLOCATOR
                    > type  ;
                };
                //@endcond

            }   // namespace details
        }   // namespace michael_map

        /// Michael's hash map
        /**
            In this implementation the bucket count is defined as a ctor parameter and cannot be changed during
            object's life cycle. Thus, the number of object's buckets cannot be expanded dynamically.

            If TRAITS::ordered_list_type is one of lock-free ordered list implementation,
            maximum value of the load factor is recommended not greater than 100 because searching in the list is linear.

            \par Template parameters
                \li \p KEY        type of key stored in the map
                \li \p VALUE    type of value stored in the map
                \li \p PAIR_TRAITS (key, value) pair traits. The default is cds::map::pair_traits <\p KEY, \p VALUE>
                \li \p TRAITS    map traits. The default is cds::map::type_traits
                \li \p ALLOCATOR node allocator. The default is \ref CDS_DEFAULT_ALLOCATOR

            \par
            The class uses the following \p TRAITS members:
                \li \p TRAITS::item_counter_type The item counter. This type cannot be cds::atomics::empty_item_counter. Counting of items in the list
                    is an important part of MichaelHashMap algorithm. The default implementation cds::map::details::MapTraits
                    uses interlocked cds::atomics::item_counter class for this purpose.
                \li \p TRAITS::bucket_type - bucket implementation. This is one of ordered list class (see cds::ordered_list).

            \par Source:
                [2002] Maged Michael "High performance dynamic lock-free hash tables and list-based sets"

            \par Implementation notes
                The library automatically selects appropriate implementation depends on garbage collector used.
                See @ref map_common_interface for description of map interface.
        */
        template <
            typename KEY,
            typename VALUE,
            typename PAIR_TRAITS = pair_traits< KEY, VALUE >,
            typename TRAITS = type_traits,
        class ALLOCATOR = CDS_DEFAULT_ALLOCATOR
        >
        class MichaelHashMap:
            public michael_map::details::michael_map_selector<KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>::type
        {
            /// Base class selects appropriate implementation depends on GC used
            typedef typename michael_map::details::michael_map_selector<KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>::type base_class ;
        public:
            typedef typename base_class::key_type        key_type    ;    ///< key type
            typedef typename base_class::value_type        value_type    ;    ///< value_type
            typedef typename base_class::pair_traits    pair_traits    ;    ///< pair traits
            typedef typename base_class::type_traits    type_traits ;   ///< map type traits

        public:
            /// Explicit bucket count definition. \p nBucketCount must be power of 2.
            MichaelHashMap( size_t nBucketCount = 1024 )
                : base_class( nBucketCount )
            {}

            /// Calculate bucket count according to parameter passed
            MichaelHashMap(
                size_t nMaxItemCount,   ///< estimation of max item count in the hash map
                size_t nLoadFactor      ///< load factor: estimation of max number of items in the bucket
                )
                : base_class( nMaxItemCount, nLoadFactor )
            {}
        };
    }    // namespace map
}    // namespace cds


#endif // #ifndef __CDS_MAP_MICHAEL_HASH_MAP_H
