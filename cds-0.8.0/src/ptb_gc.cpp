/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


// Pass The Buck (PTB) Memory manager implementation

#include <cds/gc/ptb/ptb.h>
#include <cds/details/markptr.h>

#include <boost/tr1/unordered_set.hpp>
#include <boost/intrusive/unordered_set.hpp>
namespace bi = boost::intrusive ;

namespace cds { namespace gc { namespace ptb {

    namespace {
        struct retired_equal_to {
            bool operator()( const details::retired_ptr_node& p1, const details::retired_ptr_node& p2 )
            {
                return p1.m_ptr == p2.m_ptr ;
            }
        };

        struct retired_hasher {
            size_t operator ()( const details::retired_ptr_node& p) const
            {
                return boost::hash<retired_ptr::pointer>()( p.m_ptr.m_p ) ;
            }
        };

        struct liberate_set_value_traits {
            struct node_traits {
                typedef details::retired_ptr_node   node            ;
                typedef node *                      node_ptr        ;
                typedef const node *                const_node_ptr  ;

                static node_ptr get_next(const_node_ptr n)          { return n->m_pNext; }
                static void set_next(node_ptr n, node_ptr next)     { n->m_pNext = next; }
                static size_t get_hash( node_ptr p )          { return retired_hasher()( *p ) ; }
            };
            typedef node_traits::node                                         node          ;
            typedef details::retired_ptr_node                                 value_type    ;
            typedef node_traits::node_ptr                                     node_ptr      ;
            typedef node_traits::const_node_ptr                               const_node_ptr;
            typedef boost::pointer_to_other<node_ptr, value_type>::type       pointer       ;
            typedef boost::pointer_to_other<node_ptr, const value_type>::type const_pointer ;

            static const bi::link_mode_type link_mode = bi::normal_link ;

            static node_ptr       to_node_ptr    (value_type &value)        { return &value; }
            static const_node_ptr to_node_ptr    (const value_type &value)  { return &value; }
            static pointer        to_value_ptr   (node_ptr n)               { return n; }
            static const_pointer  to_value_ptr   (const_node_ptr n)         { return n; }
        };

        typedef bi::unordered_set<
            details::retired_ptr_node,
            bi::hash< retired_hasher >,
            bi::constant_time_size<false>,
            bi::equal<retired_equal_to>,
            bi::power_2_buckets<true>,
            bi::value_traits<liberate_set_value_traits>
        > liberate_set_base ;
    }   // namespace

    namespace details {
        class liberate_set: public liberate_set_base
        {
        public:
            liberate_set( const liberate_set_base::bucket_traits& bt )
                : liberate_set_base( bt )
            {}
        };
    }   // namespace details

    GarbageCollector * GarbageCollector::m_pManager = NULL  ;

    void CDS_STDCALL GarbageCollector::Construct(
        size_t nLiberateThreshold
        , size_t nInitialThreadGuardCount
    )
    {
        if ( !m_pManager ) {
            m_pManager = new GarbageCollector( nLiberateThreshold, nInitialThreadGuardCount ) ;
        }
    }

    void CDS_STDCALL GarbageCollector::Destruct()
    {
        if ( m_pManager ) {
            delete m_pManager   ;
            m_pManager = NULL   ;
        }
    }

    GarbageCollector::GarbageCollector( size_t nLiberateThreshold, size_t nInitialThreadGuardCount )
        : m_nLiberateThreshold( nLiberateThreshold ? nLiberateThreshold : 256 )
        , m_nInitialThreadGuardCount( nInitialThreadGuardCount ? nInitialThreadGuardCount : 8 )
    {
    }

    GarbageCollector::~GarbageCollector()
    {
        liberate()  ;

        for ( details::guard_data * pGuard = m_GuardPool.begin(); pGuard; pGuard = pGuard->pGlobalNext.load<membar_relaxed>()) {
            if ( pGuard->pHandOff.load<membar_relaxed>() != NULL ) {
                m_RetiredAllocator.free( *pGuard->pHandOff.load<membar_relaxed>() ) ;
                pGuard->pHandOff.store<membar_relaxed>( reinterpret_cast<details::guard_data::handoff_ptr>(NULL) ) ;
            }
        }
    }

    void GarbageCollector::liberate()
    {
        const size_t nBucketCount = 1 << ( beans::exp2Ceil( m_RetiredBuffer.size() ) + 1 ) ;

        cds::details::Allocator<details::liberate_set::bucket_type, CDS_DEFAULT_ALLOCATOR> al ;
        details::liberate_set::bucket_type * buckets = al.NewArray( nBucketCount )    ;

        {
            details::liberate_set    set( details::liberate_set::bucket_traits( buckets, nBucketCount))   ;

            // Get list of retired pointers
            details::retired_ptr_node * pHead = m_RetiredBuffer.steal() ;
            while ( pHead ) {
                details::retired_ptr_node * pNext = pHead->m_pNext  ;
                set.insert( *pHead )    ;
                pHead = pNext    ;
            }

            // The heart of PTB algo - liberate procedure
            liberate( set ) ;

            // Free retired pointers that is liberated
            // After free_liberated items of set is invalid (removed)
            free_liberated( set )   ;
        }
        al.Delete( buckets, nBucketCount )  ;
    }

    void GarbageCollector::liberate( details::liberate_set& set )
    {
        // This "node" needs for seeking only - only key field m_ptr.m_p is significant
        details::retired_ptr_node tmpNode ;

        // set of examined guard 
        std::tr1::unordered_set<details::guard_data::value_type>   doneGuardSet ;

        for ( details::guard_data * pGuard = m_GuardPool.begin(); pGuard; pGuard = pGuard->pGlobalNext.load<membar_acquire>() ) {

            // get guarded pointer
            details::guard_data::value_type  valGuarded = pGuard->pPost.load<membar_acquire>()    ;
            details::liberate_set::iterator itFound ;
            tmpNode.m_ptr.m_p = valGuarded     ;

            if ( valGuarded != NULL ) {
                if  ( (itFound = set.find( tmpNode )) != set.end() ) {

                    // Guarded pointer is found in retired pointer list
                    set.erase( itFound )    ;

                    // Get trapped pointer for the guard
                    details::guard_data::handoff_ptr h = pGuard->pHandOff.load<membar_relaxed>() ;
                    if ( h != NULL ) {
                        if ( h->m_ptr.m_p != valGuarded ) {
                            // hand-off value h may be protected by a guard that has already been tested
                            if ( doneGuardSet.find( h->m_ptr.m_p ) == doneGuardSet.end() )
                                set.insert( *h )    ;
                            else
                                m_RetiredBuffer.push( *h )   ;  // re-push trapped pointer
                        }
                    }
                    pGuard->pHandOff.store<membar_relaxed>( &*itFound )     ;
                }
                else {
                    // Guard is not pointed to any retired pointer
                    // Check if we can retire the hand-off value of the guard

                    details::guard_data::handoff_ptr h = pGuard->pHandOff.load<membar_relaxed>() ;
                    if ( h != NULL ) {
                        if ( valGuarded != h->m_ptr.m_p && doneGuardSet.find( h->m_ptr.m_p ) == doneGuardSet.end() ) {
                            set.insert( *h )  ;
                            pGuard->pHandOff.store<membar_relaxed>( (details::guard_data::handoff_ptr) NULL )  ;
                        }
                    }
                }
            }
            else {
                // Guard is NULL
                details::guard_data::handoff_ptr h = pGuard->pHandOff.load<membar_relaxed>() ;
                if ( h != NULL && doneGuardSet.find( h->m_ptr.m_p ) == doneGuardSet.end() ) {
                    set.insert( *h )  ;
                    pGuard->pHandOff.store<membar_relaxed>( (details::guard_data::handoff_ptr) NULL )  ;
                }
            }

            if ( valGuarded != NULL )
                doneGuardSet.insert( valGuarded )    ;
        }
    }

    void GarbageCollector::free_liberated( details::liberate_set& set )
    {
        details::retired_ptr_node * pHead ;
        details::retired_ptr_node * pTail ;

        details::liberate_set::iterator it = set.begin()    ;
        details::liberate_set::iterator itEnd = set.end()   ;
        if ( it != itEnd ) {
            details::retired_ptr_node& node = *it        ;
            node.m_ptr.free()               ;
            pHead = 
                pTail = &node   ;

            ++it ;
            while ( it != itEnd ) {
                details::retired_ptr_node& node = *it        ;
                node.m_ptr.free()               ;
                details::liberate_set::iterator itNext = it  ;
                ++itNext    ;
                // set is linked in m_pNext field that is the same as m_pNextFree field.
                // So, incrementing of it after pTail->m_pNextFree = &node is buggy
                pTail = 
                    pTail->m_pNextFree = &node ;
                it = itNext ;
            }
            m_RetiredAllocator.freeList( pHead, pTail ) ;
        }
    }

}}} // namespace cds::gc::ptb
