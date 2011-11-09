/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_MAP_SPLIT_ORDERED_LIST_IMPL_H
#define __CDS_MAP_SPLIT_ORDERED_LIST_IMPL_H

/*
    Editions:
        2007.09.01  Maxim.Khiszinsky    Created
        2007.10.28  Maxim.Khiszinsky    Added support for set::TagMichaelList
        2007.11.13  Maxim.Khiszinsky    NDSplitOrderedList - split ordered list without deletion of node
        2008.11.08  Maxim.Khiszinsky    Full refactoring and renaming to support different garbage collection schemas,
                                        new naming conventions, and different ordered lists implementation
        2009.12.01  Maxim.Khiszinsky    Full refactoring to support different node types for regular and dummy nodes
*/

#include <cds/map/_base.h>
#include <cds/atomic.h>
#include <cds/numtraits.h>
#include <cds/bitop.h>
#include <cds/ordered_list/michael_list_hzp.h>
#include <cds/gc/all.h>

#include <algorithm>    // std::fill
#include <ostream>      // for dumping
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_base_of.hpp>

namespace cds {
    namespace map {
        /// Split-ordered list implementation
        namespace split_list {

            /// Static bucket table
            /**
                Non-resizeable bucket table. The capacity of table (max bucket count) is defined in the constructor call.

                Template parameter @par T defines type of table node.
                @par ALLOCATOR - memory allocator template, default is \ref CDS_DEFAULT_ALLOCATOR
            */
            template <typename T, class ALLOCATOR = CDS_DEFAULT_ALLOCATOR>
            class static_bucket_table
            {
                const size_t    m_nLoadFactor    ;    ///< load factor (average of iitems per bucket)
                const size_t    m_nMaxSize        ;    ///< Bucket table capacity
                volatile T *    m_Table            ;    ///< Bucket table

                typedef    cds::details::Allocator< T, ALLOCATOR >    Allocator    ;   ///< Internal allocator type

            public:
                /// Rebind bucket table to other bucket type Q
                template <typename Q>
                struct rebind {
                    typedef static_bucket_table< Q, ALLOCATOR >    other    ;   ///< Result of bucket table rebinding
                };

            private:
            ///@cond
                size_t calcMaxSize( size_t nItemCount )
                {
                    nItemCount = (size_t) ( nItemCount / m_nLoadFactor )    ;
                    return ((size_t) 1) << beans::exp2Ceil( nItemCount )        ;
                }

                void allocateTable()
                {
                    Allocator alloc    ;
                    m_Table = alloc.NewArray( m_nMaxSize )    ;
                    std::fill( non_volatile(m_Table), non_volatile(m_Table + m_nMaxSize), (T) NULL )    ;
                }

                void destroyTable()
                {
                    Allocator alloc    ;
                    alloc.Delete( non_volatile( m_Table ), m_nMaxSize )    ;
                }
            ///@endcond
            public:
                /// Constructs bucket table for 512K buckets. Load factor is 1.
                static_bucket_table()
                    : m_nLoadFactor( 1 )
                    , m_nMaxSize( 512*1024 )
                {
                    allocateTable() ;
                }

                /// Constructs
                static_bucket_table(
                    size_t nItemCount,        ///< Max expected item count in split-ordered list
                    size_t nLoadFactor        ///< Load factor
                    ) : m_nLoadFactor( nLoadFactor > 0 ? nLoadFactor : (size_t) 1 ),
                    m_nMaxSize( calcMaxSize( nItemCount ) )
                {
                    // m_nMaxSize must be power of 2
                    assert( cds::beans::isExp2( m_nMaxSize ) )    ;
                    allocateTable()    ;
                }

                /// Dtor. Destroys bucket table
                ~static_bucket_table()
                {
                    destroyTable()    ;
                }

                ///@cond
                volatile T *        getBucketPtr( size_t nBucket ) volatile
                {
                    assert( nBucket < m_nMaxSize )    ;
                    return m_Table + nBucket    ;
                }
                ///@endcond

                /// Get bucket by index
                T         getBucket( size_t nBucket ) volatile
                {
                    return *non_volatile( getBucketPtr( nBucket ) )    ;
                }

                /// Set bucket by index
                void    setBucket( size_t nBucket, const T& bucket )
                {
                    assert( nBucket < m_nMaxSize )    ;
                    *const_cast< T * >( m_Table + nBucket ) = bucket ;
                }

                /// Table capacity
                size_t    capacity() const    { return m_nMaxSize; }

                /// Table load factor
                size_t    loadFactor() const    { return m_nLoadFactor; }

            public: // Debugging methods
                ///@cond
                template <typename LIST>
                std::ostream&    dump( std::ostream& stm, LIST& list )
                {
                    stm << "Type: static bucket table\n"
                        << "Metrics:\n"
                        << "     bucket table size: " << m_nMaxSize << "\n"
                        << "           load factor: " << m_nLoadFactor << "\n"
                        << "Buckets\n"
                        ;
                    for ( size_t i = 0; i < m_nMaxSize; ++i ) {
                        stm << "    [" << i << "] "  ;
                        T *  pBucket = const_cast<T *>( m_Table + i )    ;
                        typename LIST::regular_node * pNode = list.deref( pBucket )   ;
                        if ( pNode ) {
                            stm << " " << ( pNode->key().isDummy() ? "dummy" : "REGULAR (error)" )
                                << " hash=" << pNode->key().m_nHash
                                << "\n" ;
                        }
                        else
                            stm << " - empty\n" ;
                    }

                    return stm ;
                }
                ///@endcond
            };

            /// Dynamic bucket table
            /**
                Dynamic bucket table dynamically grows the size when necessary. The grow unit is a segment of constant size.

                \par Template parameter 
                \li \p T - type of table node.
                \li \p ALLOCATOR - memory allocator template, default is \ref CDS_DEFAULT_ALLOCATOR
            */
            template <typename T, class ALLOCATOR = CDS_DEFAULT_ALLOCATOR>
            class dynamic_bucket_table {
                typedef    T *    segment_type    ;    ///< Segment is array of buckets. The segment's size is const

                /// Bucket table metrics
                struct Metrics {
                    size_t    nSegmentCount        ;    ///< max count of segments in bucket table
                    size_t    nSegmentSize        ;    ///< the segment's capacity. The capacity must be power of two.
                    size_t    nSegmentSizeLog2    ;    ///< log2( m_nSegmentSize )
                    size_t    nLoadFactor            ;    ///< load factor
                    size_t    nCapacity            ;    ///< max capacity of bucket table

                    Metrics()
                        : nSegmentCount(1024)
                        , nSegmentSize(512)
                        , nSegmentSizeLog2( cds::beans::exponent2( nSegmentSize ) )
                        , nLoadFactor(1)
                        , nCapacity( nSegmentCount * nSegmentSize )
                    {}
                };

                const Metrics    m_metrics        ;    ///< metrics

                segment_type *        m_Segments    ;    ///< bucket table - array of segments

                typedef    cds::details::Allocator< segment_type, ALLOCATOR >        table_allocator        ;    ///< table allocator type
                typedef    cds::details::Allocator< T, ALLOCATOR >                    segment_allocator    ;    ///< segment allocator type

            public:
                /// Rebind bucket table to other bucket type Q
                template <typename Q>
                struct rebind {
                    typedef dynamic_bucket_table< Q, ALLOCATOR >    other    ;   ///< Result of bucket table rebinding
                };

            private:
                ///@cond
                Metrics calcMetrics( size_t nItemCount, size_t nLoadFactor )
                {
                    Metrics m    ;
                    // Calculate m_nSegmentSize and m_nSegmentCount  by nItemCount
                    m.nLoadFactor = nLoadFactor > 0 ? nLoadFactor : 1    ;

                    size_t nBucketCount = (size_t)( ((float) nItemCount) / m.nLoadFactor ) ;
                    if ( nBucketCount <= 2 ) {
                        m.nSegmentCount = 1    ;
                        m.nSegmentSize = 2    ;
                    }
                    else if ( nBucketCount <= 1024 ) {
                        m.nSegmentCount = 1    ;
                        m.nSegmentSize = ((size_t) 1) << beans::exp2Ceil( nBucketCount )  ;
                    }
                    else {
                        nBucketCount = beans::exp2Ceil( nBucketCount ) ;
                        m.nSegmentCount =
                            m.nSegmentSize = ((size_t) 1) << ( nBucketCount / 2 )    ;
                        if ( nBucketCount & 1 )
                            m.nSegmentSize *= 2    ;
                        if ( m.nSegmentCount * m.nSegmentSize * m.nLoadFactor < nItemCount )
                            m.nSegmentSize *= 2    ;
                    }
                    m.nCapacity = m.nSegmentCount * m.nSegmentSize    ;
                    m.nSegmentSizeLog2 = cds::beans::exponent2( m.nSegmentSize )    ;
                    return m    ;
                }

                segment_type *    allocateTable()
                {
                    typedef typename boost::remove_cv< segment_type >::type    TSegmentCV ;
                    table_allocator alloc    ;
                    segment_type * p =  alloc.NewArray( m_metrics.nSegmentCount )    ;
                    std::fill( non_volatile(p), non_volatile(p + m_metrics.nSegmentCount), TSegmentCV(NULL) )    ;
                    return p    ;
                }
                void destroyTable( segment_type * pTable )
                {
                    table_allocator alloc    ;
                    alloc.Delete( pTable, m_metrics.nSegmentCount )    ;
                }

                segment_type allocateSegment()
                {
                    segment_allocator alloc ;
                    segment_type p = alloc.NewArray( m_metrics.nSegmentSize )    ;
                    std::fill( non_volatile( p ), non_volatile( p + m_metrics.nSegmentSize ), T(NULL) )    ;
                    return p    ;
                }
                void destroySegment( T * pSegment )
                {
                    segment_allocator alloc ;
                    alloc.Delete( pSegment, m_metrics.nSegmentSize )    ;
                }
                ///@endcond

            public:
                /// Initializes bucket table
                dynamic_bucket_table()
                {
                    initSegments()    ;
                }

                /// Create bucket table
                dynamic_bucket_table(
                    size_t nItemCount       ///< max bucket count
                    , size_t nLoadFactor    ///< load factor desired
                )
                    : m_metrics( calcMetrics( nItemCount, nLoadFactor ))
                {
                    initSegments()    ;
                }

                /// Destroys bucket table
                ~dynamic_bucket_table()
                {
                    segment_type * pSegments = m_Segments   ;
                    for ( size_t i = 0; i < m_metrics.nSegmentCount; ++i ) {
                        if ( pSegments[i] != NULL )
                            destroySegment( pSegments[i] )    ;
                    }
                    destroyTable( pSegments ) ;
                }

                ///@cond
                T *    getBucketPtr( size_t nBucket ) volatile
                {
                    size_t nSegment = nBucket >> m_metrics.nSegmentSizeLog2 ;
                    assert( nSegment < m_metrics.nSegmentCount )    ;

                    segment_type pSegment = m_Segments[ nSegment ] ;
                    if ( pSegment == NULL )
                        return NULL ;    // uninitialized bucket
                    return pSegment + (nBucket & (m_metrics.nSegmentSize - 1))  ;
                }
                ///@endcond

                /// Get bucket by index
                T    getBucket( size_t nBucket )
                {
                    T * pBucket = getBucketPtr( nBucket )    ;
                    return pBucket ? *pBucket : T() ;
                }

                /// Set bucket by index
                void setBucket( size_t nBucket, const T& bucket )
                {
                    size_t nSegment = nBucket >> m_metrics.nSegmentSizeLog2    ;
                    assert( nSegment < m_metrics.nSegmentCount )    ;

                    if ( m_Segments[nSegment] == NULL ) {
                        segment_type pNewSegment = allocateSegment()    ;
                        if ( !atomics::cas<membar_acq_rel>( &m_Segments[nSegment], (segment_type) NULL, pNewSegment )) {
                            // Segment has already been made by another thread
                            destroySegment( non_volatile( pNewSegment ))    ;
                        }
                    }
                    atomics::store<membar_release>( &( m_Segments[ nSegment ][ nBucket & (m_metrics.nSegmentSize - 1) ] ), bucket ) ;
                }

                /// Table capacity
                size_t    capacity() const        { return m_metrics.nCapacity;        }
                /// Load factor
                size_t    loadFactor() const        { return m_metrics.nLoadFactor;        }
                /// Max count of segments of the table
                size_t  maxSegmentCount() const { return m_metrics.nSegmentCount ;    }
                /// Max size of the segment
                size_t  maxSegmentSize() const  { return m_metrics.nSegmentSize ;    }

            public: // Debugging methods
                ///@cond
                template <typename LIST>
                std::ostream&    dump( std::ostream& stm, LIST& list )
                {
                    stm << "Type: dynamic bucket table\n"
                        << "Metrics:\n"
                        << "              capacity: " << m_metrics.nCapacity << "\n"
                        << "           load factor: " << m_metrics.nLoadFactor << "\n"
                        << "         segment count: " << m_metrics.nSegmentCount << "\n"
                        << "         segment size:  " << m_metrics.nSegmentSize << "\n"
                        << "    log2(segment size): " << m_metrics.nSegmentSizeLog2 << "\n"
                        << "Segments\n"
                        ;
                    for ( size_t nSegment = 0; nSegment < m_metrics.nSegmentSize; ++nSegment ) {
                        stm << "  [" << nSegment << "]" ;
                        segment_type pSegment = m_Segments[ nSegment ]  ;
                        if ( pSegment ) {
                            for ( size_t i = 0; i < m_metrics.nSegmentSize; ++i ) {
                                stm << "\n    [" << i << "] "  ;
                                T * pBucket = const_cast<T *>( pSegment + i )  ;
                                typename LIST::regular_node * pNode = list.deref( pBucket)    ;
                                if ( pNode ) {
                                    stm << ( pNode->key().isDummy() ? "dummy" : "REGULAR (error)" )
                                        << " hash=" << pNode->key().m_nHash
                                        << "\n" ;
                                }
                                else
                                    stm << " - empty\n" ;
                            }
                        }
                        else
                            stm << " - empty\n" ;
                    }

                    return stm ;
                }
                ///@endcond

            private:
                ///@cond
                void initSegments()
                {
                    // m_nSegmentSize must be 2**N
                    assert( cds::beans::isExp2( m_metrics.nSegmentSize ))    ;
                    assert( ( ((size_t) 1) << m_metrics.nSegmentSizeLog2) == m_metrics.nSegmentSize )    ;

                    // m_nSegmentCount must be 2**K
                    assert( cds::beans::isExp2( m_metrics.nSegmentCount ))    ;

                    m_Segments = allocateTable() ;
                }
                ///@endcond
            };

            /// Split-ordered list statistics
            struct statistics {
                atomics::event_counter  m_InitBucket ;        ///< initBucket call count
                atomics::event_counter  m_InitParentBucket;    ///< initBucket for parent bucket call count
                atomics::event_counter  m_InitBucketRace;    ///< count of init bucket racing

                ///@cond
                void    onInitBucket()                { ++m_InitBucket; }
                void    onInitParentBucket()        { ++m_InitParentBucket; }
                void    onInitBucketRace()            { ++m_InitBucketRace; }
                ///@endcond
            } ;

            /// Empty split-ordered list statistics
            struct empty_statistics {
                ///@cond
                void    onInitBucket()                {}
                void    onInitParentBucket()        {}
                void    onInitBucketRace()            {}
                ///@endcond
            } ;

            /// Split-Ordered list type traits
            struct type_traits: public map::type_traits
            {
                typedef map::type_traits            original    ;   ///< Original type traits (base class)

                /// Bucket type
                /**
                    This typedef defines the type of map's bucket. It is one of ordered list implementation tag
                    (see cds::ordered_list namespace)
                    In place on ordered list it should be used an ordered list implementation tag
                    (\ref cds::ordered_list::michael_list_tag, \ref cds::ordered_list::lazy_list_tag)
                    selecting appropriate implementation of the list.

                    The default type is \ref cds::ordered_list::michael_list_tag<cds::gc::hzp_gc>.
                */
                typedef original::bucket_type        bucket_type        ;

                /// Ordered list type traits
                /**
                    This typedef defines special traits for the ordered list class used as the bucket table
                */
                typedef void                ordered_list_traits ;


                typedef atomics::item_counter<>         item_counter_type;    ///< Item counter type

                typedef split_list::empty_statistics    statistics        ;    ///< statistics

                typedef void                            backoff_strategy;    ///< back-off strategy

                /// Bucket table implementation
                /**
                    May be cds::map::split_list::static_bucket_table or cds::map::split_list::dynamic_bucket_table.
                    Default is cds::map::split_list::dynamic_bucket_table

                    The static_bucket_table (and dynamic_bucket_table too) class is the template that requires the template
                    argument - the type of node in the bucket table. This argument may be dummy (for example, simple int).
                    The implementation of split-ordered list tunes (by using static_bucket_table::rebind)
                    the type of node according to type of internal node. Therefore, in yourself @ref type_traits implementation
                    you may use static_bucket_table<int> specialization and you can not worry about real type
                    of bucket table's node.
                */
                typedef void                        bucket_table    ;
            };

            /// Details of split-ordered list implementation
            namespace details {

                //@cond
                /// Ordered list adapter for SplitOrderedList
                template < class ORDERED_LIST >
                class ord_list_adapter: public ORDERED_LIST {
                public:
                    typedef ORDERED_LIST                            ordered_list_type        ;
                    typedef typename ordered_list_type::key_type    key_type    ;
                    typedef typename ordered_list_type::value_type  value_type  ;

                    typedef typename ordered_list_type::node_ptr    node_ptr    ;
                    typedef typename ordered_list_type::node_ref    node_ref    ;

                public:
                    typedef typename ordered_list_type::dummy_node      dummy_node              ;
                    typedef typename ordered_list_type::regular_node    regular_node            ;

                    typedef typename dummy_node::key_type               dummy_key_type          ;

                public:
                    dummy_node * allocDummy( const dummy_key_type& key )
                    {
                        return ordered_list_type::allocDummy( key )  ;
                    }

                    bool insert( node_ref refHead, const key_type& key, const value_type& val )
                    {
                        assert( !key.isDummy() )    ;
                        return ordered_list_type::insert( refHead, key, val )    ;
                    }

                    template <typename T, typename FUNC>
                    bool insert( node_ref refHead, const key_type& key, const T& val, FUNC func )
                    {
                        assert( !key.isDummy() )    ;
                        return ordered_list_type::insert( refHead, key, val, func )    ;
                    }

                    node_ptr insertDummy( node_ref refHead, const dummy_key_type& key )
                    {
                        regular_node * pNode = reinterpret_cast<regular_node *>( allocDummy( key ) )    ;
                        assert( pNode->key().isDummy() )    ;
                        if ( ordered_list_type::insert( refHead, pNode ))
                            return node_ptr( pNode )    ;
                        freeNode( pNode )               ;
                        return node_ptr( NULL )         ;
                    }

                    node_ptr insertDummy( const dummy_key_type& key )
                    {
                        // This function is used only in initialization code

                        regular_node * pNode = reinterpret_cast<regular_node *>( allocDummy( key ) ) ;
                        assert( pNode->key().isDummy() )    ;
                        if ( ordered_list_type::insert( pNode  ) )
                            return node_ptr( pNode )    ;
                        assert( false )             ;
                        return node_ptr( NULL )     ;
                    }

                    template <typename T, typename FUNC>
                    std::pair<bool, bool> ensure( node_ref refHead, const key_type& key, const T& val, FUNC func )
                    {
                        return ordered_list_type::ensure( refHead, key, val, func )    ;
                    }

                    template <typename T, typename FUNC>
                    bool emplace( node_ref refHead, const key_type& key, const T& val, FUNC func )
                    {
                        return ordered_list_type::emplace( refHead, key, val, func ) ;
                    }

                public: // Debugging methods
                    std::ostream&   dump( std::ostream& stm )
                    {
                        typedef typename ordered_list_type::nonconcurrent_iterator iter   ;
                        iter itEnd = ordered_list_type::nc_end()    ;
                        for ( iter it = ordered_list_type::nc_begin(); it != itEnd; ++it ) {
                            stm << ( it.key().isDummy() ? "dummy   " : "regular " )
                                << " hash=" << it.key().m_nHash
                                ;
                            if ( !it.key().isDummy() )
                                stm << " key=" << it.key().m_Key ;
                            stm << "\n" ;
                        }

                        return stm  ;
                    }
                };
                //@endcond

                //@cond
                /// Ordered-list adapter for SplitOrderedList implementation
                /**
                    This template and its specializations provide appropriate interface
                    for SplitOrderedList depending of GC used
                */
                template < typename GC, class ORDERED_LIST >
                class ord_list: public ord_list_adapter< ORDERED_LIST >
                {
                    typedef ord_list_adapter< ORDERED_LIST > base_class   ;

                public:
                    typedef typename base_class::key_type    key_type    ;
                    typedef typename base_class::value_type  value_type  ;
                    typedef typename base_class::node_ref    node_ref    ;
                    typedef typename base_class::nonconcurrent_iterator          nonconcurrent_iterator  ;
                    typedef typename base_class::nonconcurrent_const_iterator    nonconcurrent_const_iterator  ;
                public:
                    bool find( node_ref refHead, const key_type& key )
                    {
                        return base_class::find( refHead, key )    ;
                    }

                    template <typename T, typename FUNC>
                    bool find( node_ref refHead, const key_type& key, T& data, FUNC func )
                    {
                        return base_class::find( refHead, key, data, func )    ;
                    }

                    template <typename T, typename FUNC>
                    std::pair<bool, bool> ensure( node_ref refHead, const key_type& key, const T& val, FUNC func )
                    {
                        return base_class::ensure( refHead, key, val, func )    ;
                    }

                    template <typename T, typename FUNC>
                    bool emplace( node_ref refHead, const key_type& key, const T& val, FUNC func )
                    {
                        return base_class::emplace( refHead, key, val, func )   ;
                    }

                    bool erase( node_ref refHead, const key_type& key )
                    {
                        return base_class::erase( refHead, key )    ;
                    }

                    template <typename T, typename FUNC>
                    bool erase( node_ref refHead, const key_type& key, T& dest, FUNC func )
                    {
                        return base_class::erase( refHead, key, dest, func )    ;
                    }

                };
                //@endcond

                //@cond
                template < class ORDERED_LIST >
                class ord_list< gc::no_gc, ORDERED_LIST >:
                    public ord_list_adapter< ORDERED_LIST >
                {
                    typedef ord_list_adapter< ORDERED_LIST >    base_class    ;
                public:
                    typedef typename base_class::key_type    key_type    ;
                    typedef typename base_class::value_type  value_type  ;
                    typedef typename base_class::node_ref    node_ref    ;
                    typedef typename base_class::nonconcurrent_iterator          nonconcurrent_iterator  ;
                    typedef typename base_class::nonconcurrent_const_iterator    nonconcurrent_const_iterator  ;
                    typedef typename base_class::ordered_list_type               ordered_list_type       ;
                public:
                    value_type * get( node_ref refHead, const key_type& key )
                    {
                        return ordered_list_type::find( refHead, key )    ;
                    }

                    bool find( node_ref refHead, const key_type& key )
                    {
                        return get( refHead, key ) != NULL ;
                    }
                };
                //@endcond

                /// Dummy key of split-ordered list
                template <typename HASH_TYPE>
                struct dummy_key {
                    HASH_TYPE                m_nHash        ;   ///< Hash value

                    //@cond
                    dummy_key(): m_nHash(0) {}
                    dummy_key( HASH_TYPE nHash ): m_nHash( nHash ) {}
                    dummy_key( const dummy_key& key ): m_nHash( key.m_nHash ) {}

                    dummy_key& operator =( const dummy_key& key )
                    {
                        m_nHash = key.m_nHash    ;
                        return *this    ;
                    }
                    //@endcond

                    /// Check is the node is dummy one
                    bool isDummy() const
                    {
                        return (m_nHash & 1) == 0 ;
                    }
                };

                /// Regular key of split-ordered list
                template <typename KEY, typename HASH_TYPE>
                struct regular_key: public dummy_key<HASH_TYPE> {
                    KEY        m_Key    ;   ///< Key of the node

                    //@cond
                    regular_key(): dummy_key<HASH_TYPE>(0) {}
                    regular_key( HASH_TYPE nHash ): dummy_key<HASH_TYPE>( nHash ) {}
                    regular_key( const dummy_key<HASH_TYPE>& key ): dummy_key<HASH_TYPE>( key ) {}
                    regular_key( HASH_TYPE nHash, const KEY& key): dummy_key<HASH_TYPE>( nHash ), m_Key( key ) {}
                    regular_key( const regular_key& key ) { *this = key; }

                    regular_key& operator =( const regular_key& key )
                    {
                        dummy_key<HASH_TYPE>::operator=( key )    ;
                        if ( !key.isDummy() )
                            m_Key = key.m_Key    ;
                        return *this    ;
                    }
                    //@endcond
                };

                /// Split-ordered list implementation base
                template <
                    typename KEY,
                    typename VALUE,
                    typename PAIR_TRAITS = pair_traits< KEY, VALUE >,
                    typename TRAITS = split_list::type_traits,
                    class ALLOCATOR = CDS_DEFAULT_ALLOCATOR
                >
                class split_list_base: public map_base<KEY, VALUE,
                    typename cds::details::void_selector<PAIR_TRAITS, map::pair_traits<KEY, VALUE> >::type,
                    typename cds::details::void_selector<TRAITS, split_list::type_traits>::type
                >
                {
                    //@cond
                    typedef map_base<
                        KEY,
                        VALUE,
                        typename cds::details::void_selector<PAIR_TRAITS, map::pair_traits<KEY, VALUE> >::type,
                        typename cds::details::void_selector<TRAITS, split_list::type_traits>::type
                    >    base_class ;
                    //@endcond

                public:
                    typedef typename base_class::key_type                       key_type    ;    ///< key type
                    typedef typename base_class::value_type                     value_type  ;    ///< value_type
                    typedef typename base_class::pair_traits                    pair_traits ;    ///< pair traits
                    typedef typename base_class::key_traits                     key_traits  ;    ///< key traits
                    typedef typename base_class::value_traits                   value_traits;    ///< value traits

                    typedef typename base_class::hash_functor                   hash_functor    ;    ///< hash function
                    typedef typename base_class::hash_type                      hash_type       ;    ///< hash function result type
                    typedef typename base_class::hash_comparator                hash_comparator ;    ///< hash comparator

                    /// Split-list traits
                    struct type_traits: public base_class::type_traits {
                        typedef typename base_class::type_traits     original    ;  ///< original type traits

                        /// Item counter implementation. cds::atomics::empty_item_counter is invalid implementation for this hash map
                        typedef typename boost::mpl::if_<
                            boost::is_same< typename original::item_counter_type, atomics::empty_item_counter >,
                            atomics::item_counter<>,
                            typename original::item_counter_type
                        >::type                                         item_counter_type;

                        /// Bucket (ordered list) implementation tag
                        typedef typename cds::details::void_selector<
                            typename original::bucket_type,
                            ordered_list::michael_list_tag<gc::hzp_gc>
                        >::type         bucket_type ;

                        /// statistics implementation
                        typedef typename cds::details::void_selector<
                            typename original::statistics,
                            split_list::empty_statistics
                        >::type        statistics    ;

                    } ;


                public: // declare exceptions
                    /// List initialization error exception
                    CDS_DECLARE_EXCEPTION( ExInit, "SplitOrderedList initialization error" )    ;

                    /// New bucket initialization error exception
                    CDS_DECLARE_EXCEPTION( ExInitBucket, "SplitOrderedList error: cannot initialize new bucket" )    ;

                protected:
                    typedef dummy_key<hash_type>                dummy_key_type  ;   ///< dummy key type
                    typedef regular_key<key_type, hash_type>    regular_key_type;   ///< regular key type

                    /// Regular key comparator
                    struct Comparator
                    {
                        /// Compares two keys. Returns -1 if p1 < p2, 1 if p1 > p2, 0 if p1 == p2
                        static int compare( const regular_key_type& p1, const regular_key_type& p2 )
                        {
                            hash_comparator    cmpHash    ;

                            int nCmp = cmpHash( p1.m_nHash, p2.m_nHash ) ;
                            if ( nCmp || p1.isDummy() || p2.isDummy() )
                                return nCmp ;

                            // If hash values are equal then p1 and p2 have equal type (dummy or regular together)
                            typename key_traits::key_comparator cmpKey    ;
                            return cmpKey( p1.m_Key, p2.m_Key )    ;
                        }

                        /// Comparison operator
                        int operator()( const regular_key_type& p1, const regular_key_type& p2 ) const
                        {
                            return compare( p1, p2 )    ;
                        }
                    };

                    /// Special split-ordered list key traits for ordered list implementation
                    struct ordlist_key_traits: public key_traits
                    {
                        typedef regular_key_type    key_type ;          ///< key type
                        typedef Comparator            key_comparator    ;   ///< key comparator
                    };

                    /// Value traits for ordered list implementation
                    typedef value_traits            ordlist_value_traits  ;

                    /// Special split-ordered list pair traits for ordered list implementation
                    struct ordlist_pair_traits: public pair_traits
                    {
                        typedef ordlist_key_traits        key_traits      ;   ///< key type traits
                        typedef ordlist_value_traits    value_traits    ;   ///< value type traits
                    };

                private:
                    //@cond
                    struct ordered_list_traits:
                        public cds::details::void_selector<
                            typename type_traits::ordered_list_traits,
                            cds::ordered_list::type_traits
                        >::type
                    {
                        template <typename REGULAR_NODE, typename DUMMY_NODE, typename REGULAR_NODE_ALLOCATOR, typename DUMMY_NODE_ALLOCATOR>
                        struct split_list_node_deleter_functor {
                            static void free( REGULAR_NODE * p ) {
                                if ( p->key().isDummy() ) {
                                    DUMMY_NODE_ALLOCATOR a      ;
                                    a.Delete( reinterpret_cast<DUMMY_NODE *>(p) )   ;
                                }
                                else {
                                    REGULAR_NODE_ALLOCATOR a    ;
                                    a.Delete( p )   ;
                                }
                            }
                        };
                    } ;
                    ///@endcond

                    //@cond
                    /// Implementation of ordered list
                    typedef typename ordered_list::split_list::impl_selector<
                        typename type_traits::bucket_type
                        , regular_key_type
                        , dummy_key_type
                        , value_type
                        , ordlist_pair_traits
                        , ordered_list_traits
                        , ALLOCATOR
                    >::type     ordered_list_impl   ;
                    //@endcond

                protected:
                    /// Ordered list type
                    typedef ord_list< typename ordered_list_impl::gc_schema, ordered_list_impl >        ordered_list_type    ;

                    typedef typename ordered_list_type::regular_node        regular_node    ;   ///< Regular node type
                    typedef typename ordered_list_type::dummy_node          dummy_node      ;   ///< Dummy node type

                    /// Node type of the ordered list used as the head of the bucket
                    typedef typename ordered_list_type::node_ptr            bucket_head_ptr    ;

                public:
                    typedef typename ordered_list_type::gc_schema            gc_schema    ;    ///< Memory reclamation schema

                protected:
                    /// Bucket table implementation. One of cds::map::split_list::static_bucket_table or cds::map::split_list::dynamic_bucket_table
                    typedef typename cds::details::void_selector<
                        typename type_traits::bucket_table,
                        split_list::dynamic_bucket_table< bucket_head_ptr, ALLOCATOR >
                    >::type::template rebind<bucket_head_ptr>::other        bucket_table    ;

                public:
                    /// Non-concurrent iterator.
                    /**
                        This iterator is not thread safe.
                        Template parameter \p VALUE_REF defines type of iterator return value (const or not)
                        Template parameter \p ITERATOR defines iterator class of underlined ordered list.
                    */
                    template <typename VALUE_REF, typename ITERATOR>
                    class NonConcurrentIterator: public concept::non_concurrent_iterator< value_type >
                    {
                        friend class        split_list_base    ;
                    public:
                        typedef ITERATOR    list_nonconcurrent_iterator    ;   ///< iterator type
                        typedef typename list_nonconcurrent_iterator::node_type node_type    ;    ///< internal node type
                        typedef typename split_list_base::key_type                key_type    ;    ///< key type
                        typedef typename split_list_base::value_type            value_type    ;    ///< value type
                        typedef VALUE_REF                                        value_reference    ;    ///< type of reference to value

                    protected:
                        //@cond
                        list_nonconcurrent_iterator    m_Iterator    ;
                        list_nonconcurrent_iterator m_End        ;
                        //@endcond

                    protected:
                        //@cond
                        NonConcurrentIterator() {}
                        NonConcurrentIterator( list_nonconcurrent_iterator& it, list_nonconcurrent_iterator& itEnd )
                            : m_Iterator( it )
                            , m_End( itEnd )
                        {
                            skipDummy()    ;
                        }

                        void skipDummy()
                        {
                            while ( m_Iterator != m_End ) {
                                if ( !m_Iterator.key().isDummy() )
                                    break    ;
                                ++m_Iterator    ;
                            }
                        }

                        void next()
                        {
                            if ( m_Iterator != m_End ) {
                                ++m_Iterator    ;
                                skipDummy()        ;
                            }
                        }
                        //@endcond

                    public:
                        /// Copy constructor
                        NonConcurrentIterator( const NonConcurrentIterator& it )
                            : m_Iterator( it.m_Iterator )
                            , m_End( it.m_End )
                        {}

                        /// Returns const reference to key of node
                        const key_type& key() const
                        {
                            assert( m_Iterator != m_End )    ;
                            const typename list_nonconcurrent_iterator::key_type& refKey = m_Iterator.key() ;
                            assert( !refKey.isDummy() )    ;
                            return refKey.m_Key            ;
                        }

                        /// Returns reference (const reference for const iterator) to value of node
                        VALUE_REF value() const
                        {
                            assert( m_Iterator != m_End )    ;
                            return m_Iterator.value()        ;
                        }

                        /// Return pointer to node. Debugging method
                        const node_type * getNode() const
                        {
                            assert( m_Iterator != m_End )    ;
                            return m_Iterator.getNode()        ;
                        }

                        /// Pre-increment
                        NonConcurrentIterator& operator ++()
                        {
                            assert( m_Iterator != m_End )    ;
                            next()    ;
                            return *this                    ;
                        }

                        /// Post-increment
                        NonConcurrentIterator operator ++(int)
                        {
                            assert( m_Iterator != m_End )    ;
                            NonConcurrentIterator<VALUE_REF, ITERATOR> it( *this )        ;
                            next()                        ;
                            return it                    ;
                        }

                        /// Iterator comparison
                        bool operator ==( const NonConcurrentIterator<VALUE_REF, ITERATOR>& it ) const { return m_Iterator == it.m_Iterator; }
                        /// Iterator comparison
                        bool operator !=( const NonConcurrentIterator<VALUE_REF, ITERATOR>& it ) const { return !( *this == it ) ;}
                    };

                protected:
                    size_t                                    m_nBucketCountExp2    ;    ///< log2( current bucket count )
                    ordered_list_type                        m_List            ;        ///< Ordered list
                    bucket_table                            m_BucketTable    ;        ///< bucket table
                    hash_functor                            m_funcHash        ;        ///< hash functor
                    typename type_traits::item_counter_type    m_nItemCount    ;        ///< count of regular keys

                    typename type_traits::statistics        m_Stat    ;                ///< Internal statistics

                public:
                    /// Default ctor
                    split_list_base(): m_nBucketCountExp2(1)
                    {
                        init()  ;
                    }

                    /// Ctor
                    split_list_base(
                        size_t nItemCount       ///< max item count (max capacity)
                        , size_t fLoadFactor    ///< load factor desired
                        )
                        : m_nBucketCountExp2(1),
                        m_BucketTable( nItemCount, fLoadFactor )
                    {
                        init()  ;
                    }

                public:
                    /// Returns item count
                    size_t size() const         { return m_nItemCount    ; }

                    /// Checks if the map is empty
                    bool empty() const          { return m_nItemCount == 0 ; }

                    /// Inserts new key into list
                    bool insert( const KEY& key, const VALUE& val )
                    {
                        hash_type nHash = getHashKey( key )    ;
                        size_t nBucket = getBucketNo( nHash )    ;

                        bucket_head_ptr bucketHead = m_BucketTable.getBucket( nBucket )    ;
                        if ( bucketHead == NULL )
                            bucketHead = initBucket( nBucket )    ;

                        assert( bucketHead->key().isDummy() )   ;

                        if ( !m_List.insert( bucketHead, regular_key_type( getRegularKey( nHash ), key ), val ))
                            return false    ;

                        incItemCount()    ;

                        return true    ;
                    }

                    template <typename T, typename FUNC>
                    bool insert( const KEY& key, const T& val, FUNC func )
                    {
                        hash_type nHash = getHashKey( key )    ;
                        size_t nBucket = getBucketNo( nHash )    ;

                        bucket_head_ptr bucketHead = m_BucketTable.getBucket( nBucket )    ;
                        if ( bucketHead == NULL )
                            bucketHead = initBucket( nBucket )    ;

                        assert( bucketHead->key().isDummy() )   ;

                        if ( !m_List.insert( bucketHead, regular_key_type( getRegularKey( nHash ), key ), val, func ))
                            return false    ;

                        incItemCount()    ;

                        return true    ;
                    }

                    /// Finds \p key in the split-ordered list
                    bool find( const KEY& key )
                    {
                        hash_type nHash = getHashKey( key )    ;
                        size_t nBucket = getBucketNo( nHash )    ;

                        bucket_head_ptr bucketHead = m_BucketTable.getBucket( nBucket )    ;
                        if ( bucketHead == NULL ) {
                            // We do not return false: absence of the bucket does not mean that the key
                            // is missing in the map. Growing the hash table is simple increment
                            // of m_nBucketCountExp2 member and only for first access to the new bucket
                            // its parent bucket is divided and new bucket is initialized. So, missing
                            // of the goal bucket means that its parent bucket is not divided yet.
                            bucketHead = initBucket( nBucket )    ;
                        }

                        return m_List.find( bucketHead, regular_key_type( getRegularKey( nHash ), key ) ) ;
                    }

                    /// Ensures that key \p key exists in the list
                    /**
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
                    std::pair<bool, bool> ensure( const KEY& key, const T& val, FUNC func )
                    {
                        hash_type nHash = getHashKey( key )    ;
                        size_t nBucket = getBucketNo( nHash )    ;

                        bucket_head_ptr bucketHead = m_BucketTable.getBucket( nBucket )    ;
                        if ( bucketHead == NULL )
                            bucketHead = initBucket( nBucket )    ;

                        std::pair<bool, bool> bRet = m_List.ensure( bucketHead, regular_key_type( getRegularKey( nHash ), key ), val, func ) ;
                        if ( bRet.first && bRet.second ) {
                            // Item has been added
                            incItemCount()    ;
                        }

                        return bRet    ;
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
                        hash_type nHash = getHashKey( key )    ;
                        size_t nBucket = getBucketNo( nHash )    ;

                        bucket_head_ptr bucketHead = m_BucketTable.getBucket( nBucket )    ;
                        if ( bucketHead == NULL )
                            bucketHead = initBucket( nBucket )    ;

                        return m_List.emplace( bucketHead, regular_key_type( getRegularKey( nHash ), key ), val, func ) ;
                    }

                public:
                    ///@name  Non-concurrent iterators
                    //@{

                    /// Non-concurrent iterator
                    /**
                        This iterator is not thread-safe.
                    */
                    typedef NonConcurrentIterator<
                        value_type&,
                        typename ordered_list_type::nonconcurrent_iterator
                    >        nonconcurrent_iterator    ;

                    /// Non-concurrent const iterator
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
                        typename nonconcurrent_iterator::list_nonconcurrent_iterator itBegin( m_List.nc_begin() )   ;
                        typename nonconcurrent_iterator::list_nonconcurrent_iterator itEnd( m_List.nc_end() )  ;
                        return nonconcurrent_iterator( itBegin, itEnd )    ;
                    }

                    /// Returns non-concurrent forward end iterator
                    /**
                        End iterator points to tail dummy node. You may use end iterator for comparison only
                    */
                    nonconcurrent_iterator    nc_end()
                    {
                        typename nonconcurrent_iterator::list_nonconcurrent_iterator it( m_List.nc_end() )  ;
                        return nonconcurrent_iterator( it, it )         ;
                    }

                    /// Returns non-concurrent const forward start iterator
                    /**
                        For an empty list start iterator is equal to \a end()
                    */
                    nonconcurrent_const_iterator    nc_begin() const
                    {
                        typename nonconcurrent_const_iterator::list_nonconcurrent_iterator itBegin( m_List.nc_begin() )   ;
                        typename nonconcurrent_const_iterator::list_nonconcurrent_iterator itEnd( m_List.nc_end() )  ;
                        return nonconcurrent_const_iterator( itBegin, itEnd )    ;
                    }

                    /// Returns non-concurrent const forward end iterator
                    /**
                        End iterator points to tail dummy node. You may use end iterator for comparison only
                    */
                    nonconcurrent_const_iterator    nc_end() const
                    {
                        typename nonconcurrent_const_iterator::list_nonconcurrent_iterator it( m_List.nc_end() )  ;
                        return nonconcurrent_const_iterator( it, it )         ;
                    }

                    //@}

                public:
                    ///@name  Debug methods. NOT THREAD-SAFE!
                    //@{

                    /// Returns the reference to internal map statistics
                    const typename type_traits::statistics&    getStatistics() const    { return m_Stat; }

                    /// Dumping the internal content of split list to stream \p stm. Debugging method
                    std::ostream&       dump( std::ostream& stm )
                    {
                        stm << "Item count: " << m_nItemCount  << "\n"
                        //    << "Bucket table:\n"
                            ;
                        //m_BucketTable.dump( stm, m_List )     ;
                        stm << "\n Ordered list:\n" ;
                        m_List.dump( stm )          ;

                        return stm  ;
                    }

                    //@}

                protected:
                    /// Reverses bit order in \p nHash
                    static hash_type    reverseBitOrder( hash_type nHash ) { return bitop::RBO( nHash )    ; }

                    /// Returns the hash value of \p key
                    hash_type    getHashKey( const KEY& key ) const    { return m_funcHash( key )    ; }

                    /// Converts \p nHash to regular hash value
                    hash_type    getRegularKey( hash_type nHash ) const
                    {
                        // regular key: MSB (LSB in reversed hash) must be 1
                        return reverseBitOrder( nHash ) | ((hash_type) 1) ;
                        //return reverseBitOrder( nHash | ( ((hash_type) 1) << (sizeof(hash_type) * 8 - 1)) )    ;
                    }

                    /// Converts \p nHash to dummy hash value
                    hash_type    getDummyKey( hash_type nHash ) const    
                    { 
                        // dummy key: MSB (LSB in reversed hash) must be 0
                        return reverseBitOrder( nHash ) & ~((hash_type) 1) ;
                        //return reverseBitOrder( nHash & ~(( ((hash_type) 1) << (sizeof(hash_type) * 8 - 1) ) - 1) )    ; 
                    }

                    /// Returns bucket index of hash value \p nHash
                    size_t getBucketNo( hash_type nHash ) const
                    {
                        // = nHash % m_BucketTable.capacity(); m_BucketTable.capacity() == 2**N
                        return nHash & ( (1 << m_nBucketCountExp2) - 1 )    ;
                    }

                    /// Returns bucket index of parent of bucket \p nBucket
                    size_t getParentBucket( size_t nBucket ) const
                    {
                        assert( nBucket > 0 )    ;
                        return nBucket & ~( 1 << bitop::MSBnz( nBucket ) ) ;
                    }

                    /// Initializes bucket \p nBucket
                    bucket_head_ptr initBucket( size_t nBucket )
                    {
                        assert( nBucket > 0 )    ;
                        size_t nParent = getParentBucket( nBucket )    ;

                        m_Stat.onInitBucket()    ;

                        bucket_head_ptr parentBucket = m_BucketTable.getBucket( nParent )    ;
                        if ( parentBucket == bucket_head_ptr(NULL) ) {
                            m_Stat.onInitParentBucket()    ;
                            parentBucket = initBucket( nParent )    ;
                        }

                        if ( parentBucket == bucket_head_ptr(NULL) )
                            throw ExInitBucket()    ;

                        bucket_head_ptr bucketHead = m_List.insertDummy( parentBucket, getDummyKey( nBucket ) )    ;
                        if ( bucketHead != bucket_head_ptr(NULL) ) {
                            m_BucketTable.setBucket( nBucket, bucketHead )    ;

                            return bucketHead    ;
                        }

                        // Another thread set the bucket. Wait while it done

                        // In this point, we must wait while nBucket is empty.
                        // The compiler can decide that waiting loop can be "optimized" (stripped)
                        // To prevent this situation, we use waiting on volatile bucket_head_ptr pointer.
                        //
                        m_Stat.onInitBucketRace()    ;

                        typename cds::details::void_selector<
                            typename type_traits::backoff_strategy,
                            backoff::Default
                        >::type bkoff ;

                        while ( true ) {
                            volatile bucket_head_ptr * pBucket = m_BucketTable.getBucketPtr( nBucket ) ;
                            if ( pBucket && *non_volatile( pBucket ) != bucket_head_ptr(NULL) )
                                return *non_volatile( pBucket )    ;
                            bkoff()    ;
                        }
                    }

                    /// Increments item count
                    void    incItemCount()
                    {
                        size_t sz = m_nBucketCountExp2    ;
                        if ( ( ++m_nItemCount >> sz ) > m_BucketTable.loadFactor() && ((size_t)(1 << sz )) < m_BucketTable.capacity() )
                        {
                            atomics::cas<membar_acq_rel>( &m_nBucketCountExp2, sz, sz + 1 ) ;
                        }
                    }

                private:
                    //@cond
                    void init()
                    {
                        // Initialize bucket 0
                        bucket_head_ptr bucketHead = m_List.insertDummy( getDummyKey( 0 ) )    ;
                        if ( bucketHead == NULL  )
                            throw ExInit()    ;
                        m_BucketTable.setBucket( 0, bucketHead )    ;
                    }
                    //@endcond
                };

                template <
                    bool     CAN_DELETE_ITEMS,
                    typename KEY,
                    typename VALUE,
                    typename PAIR_TRAITS = pair_traits< KEY, VALUE >,
                    typename TRAITS = split_list::type_traits,
                class ALLOCATOR = CDS_DEFAULT_ALLOCATOR
                >
                class split_list_impl ;


                /// Split ordered list without support of node deletion
                /**
                    You must not use this class directly. This implementation is automatically selected by SplitOrderedList
                    if garbage collector schema of underlying ordered list \p TRAITS::ordered_list_type is gc::no_gc.

                    See \ref split_list_base for further description of map interface
                */
                template <
                    typename KEY,
                    typename VALUE,
                    typename PAIR_TRAITS,
                    typename TRAITS,
                    class ALLOCATOR
                >
                class split_list_impl< false, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                    : public split_list_base< KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >
                {
                    //@cond
                    typedef split_list_base< KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >    base_class    ;
                protected:
                    typedef typename base_class::hash_type          hash_type       ;
                    typedef typename base_class::bucket_head_ptr    bucket_head_ptr ;
                    typedef typename base_class::regular_key_type   regular_key_type;
                public:
                    split_list_impl(): base_class()
                    {}

                    split_list_impl( size_t nItemCount, size_t nLoadFactor )
                        : base_class( nItemCount, nLoadFactor )
                    {}
                    //@endcond

                public:

                    /// Returns pointer to the value of \p key, NULL if \p key is not found
                    VALUE * get( const KEY& key )
                    {
                        hash_type nHash = base_class::getHashKey( key )    ;
                        size_t nBucket = base_class::getBucketNo( nHash )    ;

                        bucket_head_ptr bucketHead = base_class::m_BucketTable.getBucket( nBucket )    ;
                        if ( bucketHead == NULL ) {
                            // We must initialize the bucket before continue
                            bucketHead = base_class::initBucket( nBucket )    ;
                        }

                        return base_class::m_List.get( bucketHead, regular_key_type( base_class::getRegularKey( nHash ), key ) ) ;
                    }
                } ;

                /// Split ordered list with support of node deletion
                /**
                    You must not use this class directly. This implementation is automatically selected by SplitOrderedList
                    if garbage collector schema of underlying ordered list \p TRAITS::ordered_list_type is not gc::no_gc.

                    See \ref split_list_base for further description of map interface
                */
                template <
                    typename KEY,
                    typename VALUE,
                    typename PAIR_TRAITS,
                    typename TRAITS,
                    class ALLOCATOR
                >
                class split_list_impl< true, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR>
                    : public split_list_base< KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >
                {
                    //@cond
                    typedef split_list_base< KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >    base_class    ;

                protected:
                    typedef typename base_class::regular_key_type   regular_key_type    ;
                    typedef typename base_class::hash_type          hash_type           ;
                    typedef typename base_class::bucket_head_ptr    bucket_head_ptr     ;

                public:
                    split_list_impl(): base_class()
                    {}

                    split_list_impl( size_t nItemCount, size_t nLoadFactor )
                        : base_class( nItemCount, nLoadFactor )
                    {}
                    //@endcond

                public:
                    /// Finds \p key in the split-ordered list
                    bool find( const KEY& key )
                    {
                        return base_class::find( key )  ;
                    }

                    /// Finds \p key in the split-ordered list. If key is found, returns copy of its value in \p val
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
                    bool find( const KEY& key, T& val, FUNC func )
                    {
                        hash_type nHash = base_class::getHashKey( key )    ;
                        size_t nBucket = base_class::getBucketNo( nHash )    ;

                        bucket_head_ptr bucketHead = base_class::m_BucketTable.getBucket( nBucket )    ;
                        if ( bucketHead == NULL ) {
                            // We must initialize the bucket before continue
                            bucketHead = base_class::initBucket( nBucket )    ;
                        }

                        return base_class::m_List.find( bucketHead, regular_key_type( base_class::getRegularKey( nHash ), key ), val, func ) ;
                    }

                    /// Erases \p key from the list
                    bool erase( const KEY& key )
                    {
                        int n   ;
                        ordered_list::details::trivial_erase_functor f ;
                        return erase( key, n, f )   ;
                    }

                    /// Erases \p key from the list
                    template <typename T, typename FUNC>
                    bool erase( const KEY& key, T& dest, FUNC func )
                    {
                        hash_type nHash = base_class::getHashKey( key )    ;
                        size_t nBucket = base_class::getBucketNo( nHash )    ;
                        bucket_head_ptr bucketHead = base_class::m_BucketTable.getBucket( nBucket )        ;

                        if ( bucketHead == NULL ) {
                            // We must initialize the bucket
                            bucketHead = base_class::initBucket( nBucket )    ;
                        }

                        if ( !base_class::m_List.erase( bucketHead, regular_key_type( base_class::getRegularKey( nHash ), key ), dest, func )) {
                            return false    ;
                        }

                        --base_class::m_nItemCount        ;

                        return true            ;
                    }
                } ;

                //@cond
                template <typename GC,
                    typename KEY,
                    typename VALUE,
                    typename PAIR_TRAITS = pair_traits< KEY, VALUE >,
                    typename TRAITS = split_list::type_traits,
                    class ALLOCATOR = CDS_DEFAULT_ALLOCATOR
                >
                struct split_list_gc_selector {
                    typedef split_list_impl<true, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >    list_impl    ;
                } ;
                //@endcond

                //@cond
                template <
                    typename KEY,
                    typename VALUE,
                    typename PAIR_TRAITS,
                    typename TRAITS,
                    class ALLOCATOR
                >
                struct split_list_gc_selector< gc::no_gc, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR > {
                    typedef split_list_impl< false, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >    list_impl    ;
                };
                //@endcond

                /// Split-ordered list implementation selector
                /**
                    Split-ordered list may or may not support the deletion of items. It depends on garbage collection schema
                    of underlying ordered list implementation.
                    This class selects appropriate split ordered list implementation
                    based on garbage collection schema of underlying ordered list.
                */
                template <
                    typename KEY,
                    typename VALUE,
                    typename PAIR_TRAITS = pair_traits< KEY, VALUE >,
                    typename TRAITS = split_list::type_traits,
                    class ALLOCATOR = CDS_DEFAULT_ALLOCATOR
                >
                class split_list_selector
                {
                    /// GC selector
                    typedef typename split_list_base< KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >::gc_schema    gc_schema ;
                public:
                    /// Split-ordered list implementation type based on GC schema of underlying ordered list
                    typedef typename split_list_gc_selector< gc_schema, KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >::list_impl        type    ;
                };
            }    // namespace details
        }    // namespace split_list


        /// Split-ordered list
        /**
            Hash table implementation based on split-ordered list

            \par Template parameters:
                \li \p KEY        Key type stored in list
                \li \p VALUE    Value type stored in list
                \li \p PAIR_TRAITS    <KEY, VALUE> pair traits. See @ref cds::map::pair_traits template for detail
                \li \p TRAITS    Split-ordered list traits. See @ref cds::map::split_list::type_traits for detail
                \li \p ALLOCATOR Memory allocator. Default is \ref CDS_DEFAULT_ALLOCATOR

            \par Source:
                [2003] Ori Shalev, Nir Shavit "Split-Ordered Lists - Lock-free Resizable Hash Tables"

            \par
            Split-ordered list can be resizeable or not.
            If TRAITS::bucket_table is @ref cds::map::split_list::dynamic_bucket_table that is default then the list is resizeable
            and may increase dynamically. The constructor calculates upper limit of dynamic bucket table based on
            its arguments - maximum item count expected and load factor.

            If TRAITS::bucket_table is @ref cds::map::split_list::static_bucket_table then the list's bucket table cannot grow dynamically. 
            The size of bucket table is calculated during construction time based on maximum item count expected and load factor.

            The item counter (\p TRAITS::item_counter_type) type cannot be @ref cds::atomics::empty_item_counter. Counting of items in the list
            is important part of split-ordered list algorithm. The default implementation @ref cds::map::split_list::type_traits
            uses @ref cds::atomics::item_counter class for this purpose.

            Depend on garbage collection (GC) schema of underlying ordered list implementation (\p TRAITS::ordered_list_type)
            split-ordered list may support item removing or may not. If GC of underlying ordered list is cds::gc::no_gc,
            then the split-ordered list is not support item removing, for any other GC the list is support item removing.

            See cds::map::split_list::details split_list_impl specializations for description of map interface.

            \b Note: for ordered list based on @ref cds::gc::tagged_gc memory reclamation schema the shared free list is not supported.

            \par Implementation notes
            The library automatically selects appropriate implementation depends on garbage collector used.
            See @ref map_common_interface for description of map interface.
        */
        template <
            typename KEY,
            typename VALUE,
            typename PAIR_TRAITS = pair_traits< KEY, VALUE >,
            typename TRAITS = split_list::type_traits,
            class ALLOCATOR = CDS_DEFAULT_ALLOCATOR
        >
        class SplitOrderedList:
            public split_list::details::split_list_selector< KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >::type
        {
            //@cond
            typedef typename split_list::details::split_list_selector< KEY, VALUE, PAIR_TRAITS, TRAITS, ALLOCATOR >::type    base_class    ;
            //@endcond
        public:
            /// Constructs the map. The expected item count and load factor set by default
            SplitOrderedList(): base_class()
            {}

            /// Constructs the map.
            SplitOrderedList(
                size_t nItemCount       ///< the maximum expected item count in the list
                , size_t nLoadFactor    ///< Load factor (the average item count per bucket)
            )
            : base_class( nItemCount, nLoadFactor )
            {}
        } ;

    }    // namespace map
}    // namespace cds

#endif // #ifndef __CDS_MAP_SPLIT_ORDERED_LIST_IMPL_H
