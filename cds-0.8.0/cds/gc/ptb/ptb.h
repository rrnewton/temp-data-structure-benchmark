/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_PTB_PASS_THE_BUCK_H
#define __CDS_GC_PTB_PASS_THE_BUCK_H

#include <cds/gc/ptb_gc.h>
#include <cds/gc/details/retired_ptr.h>
#include <cds/atomic/tagged_ptr.h>
#include <cds/details/markptr.h>
#include <cds/details/aligned_allocator.h>
#include <cds/details/allocator.h>

#include <cds/lock/spinlock.h>

#include <boost/noncopyable.hpp>

#if CDS_COMPILER == CDS_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4251)    // C4251: 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
#endif

namespace cds { namespace gc {

    /// Pass The Buck reclamation schema
    /**
        \par Sources:
        \li [2002] M. Herlihy, V. Luchangco, and M. Moir. The repeat offender problem: A mechanism for supporting
            dynamic-sized lockfree data structures. Technical Report TR-2002-112, Sun Microsystems Laboratories, 2002
        \li [2002] M. Herlihy, V. Luchangco, P. Martin, and M. Moir. Dynamic-sized Lockfree Data Structures.
            Technical Report TR-2002-110, Sun Microsystems Laboratories, 2002
        \li [2005] M. Herlihy, V. Luchangco, P. Martin, and M. Moir. Nonblocking Memory Management Support
            for Dynamic_Sized Data Structures. ACM Transactions on Computer Systems, Vol.23, No.2, May 2005

        @cond
        \par Usage
            Pass-the-Buck (PTB) garbage collector is the singleton. The main user-level part of PTB schema is 
            GC class and its nested classes. Before use any PTB-related class you must initialize PTB garbage collector 
            by contructing cds::gc::ptb::GC object in beginning of your main(). 
            See cds::gc::ptb::GC class for explanation.
        @endcond

        \par Implementation issues
            At present, the access to \p liberate function is serialized inside the PTB GC. There is no waiting: 
            if a thread detects that \p liberate is busy by some other ones then the thread skips the call of \p liberate 
            and continues to work. This solution allows to significantly reduce the complexity of \p liberate function.

            Unlike \p liberate function described in the paper [2005], current implementation may push the hand-off retired pointer
            trapped by the guard back to the buffer of retired pointer. Additional analysis is required to solve this problem.

            The global list of free guards (cds::gc::ptb::details::guard_allocator) is protected by spin-lock (i.e. serialized). 
            It seems that solution should not introduce significant performance bottleneck, because each thread has own set 
            of guards allocated from global list of free guards and access to global list is occurred only when 
            all thread's guard is busy. In this case the thread allocates next block of guards from global list.
            Guards allocated for the thread is push back to the global list only when the thread terminates.
    */
    namespace ptb {

        // Forward declarations
        class Guard ;
        template <size_t COUNT> class GuardArray    ;
        class ThreadGC  ;
        class GarbageCollector  ;

        /// Retired pointer type
        typedef cds::gc::details::retired_ptr retired_ptr   ;

        using cds::gc::details::free_retired_ptr_func   ;

        /// Details of Pass the Buck algorithm
        namespace details {

            // Forward declaration
            class liberate_set  ;

            /// Retired pointer buffer node
            struct retired_ptr_node {
                retired_ptr         m_ptr   ;   ///< retired pointer
                union {
                    retired_ptr_node *  m_pNext     ;   ///< next retired pointer in buffer
                    retired_ptr_node *  m_pNextFree ;   ///< next item in free list of retired_ptr_node
                } ;
            };

            /// Internal guard representation
            struct guard_data {
                typedef retired_ptr_node *  handoff_ptr ;   ///< trapped value type
                atomic<handoff_ptr>         pHandOff    ;   ///< trapped value

                typedef void *      value_type  ;   ///< type of value guarded
                atomic<value_type>  pPost       ;   ///< pointer guarded

                atomic<guard_data *>     pGlobalNext ;   ///< next item of global list of allocated guards
                atomic<guard_data *>     pNextFree   ;   ///< pointer to the next item in global or thread-local free-list

                guard_data *             pThreadNext ;   ///< next item of thread's local list of guards

                //@cond
                CDS_CONSTEXPR guard_data()
                    : pGlobalNext( NULL )
                    , pNextFree( NULL )
                    , pThreadNext( NULL )
                {}

                void init()
                {
                    pPost.store<membar_relaxed>( (void *) NULL )   ;
                }
                //@endcond

                /// Checks if the guard is free, that is, it does not contain any pointer guarded
                bool isFree() const
                {
                    return pPost.load<membar_acquire>() == NULL ;
                }
            };

            /// Guard allocator
            template <class ALLOCATOR = CDS_DEFAULT_ALLOCATOR>
            class guard_allocator
            {
                cds::details::Allocator<details::guard_data>  m_GuardAllocator    ;   ///< guard allocator

                atomic<guard_data *>    m_GuardList ;       ///< Head of allocated guard list (linked by guard_data::pGlobalNext field)
                atomic<guard_data *>    m_FreeGuardList ;   ///< Head of free guard list (linked by guard_data::pNextFree field)
                SpinLock                m_freeListLock  ;   ///< Access to m_FreeGuardList

                /**
                    Unfortunately, access to the list of free guard is lock-based.
                    Lock-free manipulations with guard free-list are ABA-prone.
                    TODO: working with m_FreeGuardList in lock-free manner.
                */

            private:
                /// Allocates new guard from the heap. The function uses aligned allocator
                guard_data * allocNew()
                {
                    //TODO: the allocator should make block allocation

                    details::guard_data * pGuard = m_GuardAllocator.New() ;

                    // Link guard to the list
                    // m_GuardList is accumulated list and it cannot support concurrent deletion, 
                    // so, ABA problem is impossible for it
                    details::guard_data * pHead ;
                    do {
                        pGuard->pGlobalNext.store<membar_relaxed>(
                            pHead = m_GuardList.load<membar_acquire>()
                        );
                    } while ( !m_GuardList.cas<membar_release>( pHead, pGuard ))    ;

                    pGuard->init()  ;
                    return pGuard   ;
                }

            public:
                ~guard_allocator()
                {
                    guard_data * pNext  ;
                    for ( guard_data * pData = m_GuardList.load<membar_relaxed>(); pData != NULL; pData = pNext ) {
                        pNext = pData->pGlobalNext.load<membar_relaxed>()  ;
                        m_GuardAllocator.Delete( pData )    ;
                    }
                }

                /// Allocates a guard from free list or from heap if free list is empty
                guard_data * alloc()
                {
                    // Try to pop a guard from free-list
                    details::guard_data * pGuard ;

                    m_freeListLock.lock()   ;
                    pGuard = m_FreeGuardList.load<membar_relaxed>() ;
                    if ( !pGuard ) {
                        m_freeListLock.unlock() ;
                        return allocNew() ;
                    }
                    m_FreeGuardList.store<membar_relaxed>( pGuard->pNextFree.load<membar_relaxed>() )  ;
                    m_freeListLock.unlock() ;
                    pGuard->init()  ;
                    return pGuard   ;

                    /* naive lock-free approach. ABA-prone
                    do {
                        pGuard = m_FreeGuardList.load<membar_acquire>()    ;
                        if ( !pGuard )
                            return allocNew() ;
                    // hmm... It seems, access to pGuard->pNextFree is dangerous.
                    // If another thread allocated free guard and then freed it between pGuard->pNextFree.load and cas, 
                    // then it is possible that pGuard is not changed but pGuard->pNextFree is changed.
                    // By construction, the thread frees all of its guards on terminating, thus, probability of this event 
                    // is small but greater than zero.
                    // Possible solution: Michael's emulation of "ideal LL/SC". However, it is heavy-weight construction, I think.
                    } while ( !m_FreeGuardList.cas<membar_release>( pGuard, pGuard->pNextFree.load<membar_acquire>() )) ;
                    pGuard->init()  ;
                    return pGuard   ;
                    */
                }

                /// Frees guard \p pGuard
                /**
                    The function places the guard \p pGuard into free-list
                */
                void free( guard_data * pGuard )
                {
                    pGuard->pPost.store<membar_relaxed>( (void *) NULL ) ;

                    m_freeListLock.lock()   ;
                    pGuard->pNextFree.store<membar_relaxed>( m_FreeGuardList.load<membar_relaxed>() )   ;
                    m_FreeGuardList.store<membar_relaxed>( pGuard ) ;
                    m_freeListLock.unlock() ;

                /* naive lock-free approach. ABA-prone
                    details::guard_data * pFree  ;
                    do {
                        pFree = m_FreeGuardList.load<membar_acquire>() ;
                        pGuard->pNextFree.store<membar_relaxed>( pFree )   ;
                    } while ( !m_FreeGuardList.cas<membar_release>( pFree, pGuard )) ;
                */
                }

                /// Allocates list of guard
                /**
                    The list returned is linked by guard's \p pThreadNext and \p pNextFree fields.

                    cds::gc::ptb::ThreadGC supporting method
                */
                guard_data * allocList( size_t nCount )
                {
                    assert( nCount != 0 )   ;

                    guard_data * pHead   ;
                    guard_data * pLast   ;

                    pHead =
                        pLast = alloc() ;

                    // The guard list allocated is private for the thread,
                    // so, we may use relaxed membar
                    while ( --nCount ) {
                        pLast =
                            pLast->pThreadNext =
                            pLast->pNextFree.store<membar_relaxed>( alloc() ) ;
                    }
                    pLast->pThreadNext =
                        pLast->pNextFree.store<membar_relaxed>( NULL ) ;

                    return pHead ;
                }

                /// Frees list of guards
                /**
                    The list \p pList is linked by guard's \p pThreadNext field.
                    cds::gc::ptb::ThreadGC supporting method
                */
                void freeList( guard_data * pList )
                {
                    assert( pList != NULL ) ;

                    guard_data * pLast = pList ;
                    while ( pLast->pThreadNext ) {
                        pLast->pPost.store<membar_relaxed>( NULL )  ;
                        pLast = 
                            pLast->pNextFree.store<membar_relaxed>( pLast->pThreadNext ) ;
                    }

                    m_freeListLock.lock()   ;
                    pLast->pNextFree.store<membar_relaxed>( m_FreeGuardList.load<membar_relaxed>() )    ;
                    m_FreeGuardList.store<membar_relaxed>( pList )  ;
                    m_freeListLock.unlock() ;

                    /* naive lock-free approach. ABA-prone
                    details::guard_data * pFree     ;
                    do {
                        pFree = m_FreeGuardList.load<membar_acquire>() ;
                        pLast->pNextFree.store<membar_relaxed>( pFree )   ;
                    } while ( !m_FreeGuardList.cas<membar_release>( pFree, pList )) ;
                    */
                }

                /// Returns the list's head of guards allocated
                guard_data * begin()
                {
                    return m_GuardList.template load<membar_acquire>()  ;
                }
            };

            /// Retired pointer buffer
            /**
                The buffer of retired nodes ready for liberating.
                When size of buffer exceeds a threshold the GC calls \p liberate procedure to free
                retired nodes.
            */
            class retired_ptr_buffer
            {
                atomic<retired_ptr_node *>  m_pHead     ;   ///< head of buffer
                atomic<size_t>              m_nItemCount;   ///< buffer's item count

            public:
                CDS_CONSTEXPR retired_ptr_buffer()
                    : m_pHead( NULL )
                    , m_nItemCount(0)
                {}

                /// Pushes new node into the buffer. Returns current buffer size
                size_t push( retired_ptr_node& node )
                {
                    retired_ptr_node * pHead  ;
                    do {
                        node.m_pNext =
                            pHead = m_pHead.load<membar_acquire>() ;
                    } while ( !m_pHead.cas<membar_release>( pHead, &node ))    ;

                    return m_nItemCount.inc<membar_release>() + 1  ;
                }

                /// Gets current list of retired pointer and clears the list
                retired_ptr_node *    steal()
                {
                    retired_ptr_node * pHead ;

                    do {
                        pHead = m_pHead.load<membar_acquire>() ;
                    } while ( !m_pHead.cas<membar_release>( pHead, reinterpret_cast<retired_ptr_node *>( NULL ) ))    ;

                    // Item counter is needed as a threshold for liberate procedure
                    // So, we may clear the item counter non-atomically without
                    // synchronization with the list
                    m_nItemCount.store<membar_release>( size_t(0) )    ;

                    return pHead    ;
                }

                /// Returns current size of buffer (approximate)
                size_t size() const
                {
                    return m_nItemCount.load<membar_relaxed>() ;
                }
            };

            /// Pool of retired pointers
            /**
                The class acts as an allocator of retired node.

                Retired pointers are linked in the lock-free list. 
                Each thread allocates the retired pointers from the list when needed and pushes them back to the list only when the thread terminating.
                It seems, theoretically, well-known ABA-problem is possible when manipulating with the list.
                However, if threads are long-lived then ABA-problem is practically impossible.
            */
            template <class ALLOCATOR = CDS_DEFAULT_ALLOCATOR>
            class retired_ptr_pool {
                /// Pool item
                typedef retired_ptr_node    item    ;

                /// Count of items in block
                static const size_t m_nItemPerBlock = 1024 / sizeof(item) - 1   ;

                /// Pool block
                struct block {
                    block *     pNext   ;   ///< next block
                    item        items[m_nItemPerBlock]  ;   ///< item array
                };

                atomic<block *> m_pBlockListHead    ;   ///< head of of allocated block list
                atomic<item *>  m_pFreeHead         ;   ///< head of list of free items

                cds::details::Allocator< block, ALLOCATOR > m_BlockAllocator    ;   ///< block allocator

            private:
                //@cond
                void allocNewBlock()
                {
                    // allocate new block
                    block * pNew = m_BlockAllocator.New()   ;

                    // link items within the block
                    item * pLastItem = pNew->items + m_nItemPerBlock - 1    ;
                    for ( item * pItem = pNew->items; pItem != pLastItem; ++pItem ) {
                        pItem->m_pNextFree = pItem + 1  ;
                    }

                    // link new block to block list
                    {
                        block * pHead   ;
                        do {
                            pNew->pNext =
                                pHead = m_pBlockListHead.template load<membar_acquire>()    ;
                        } while ( !m_pBlockListHead.template cas<membar_release>( pHead, pNew ))    ;
                    }

                    // link block's items to free list
                    {
                        item * pHead    ;
                        do {
                            pLastItem->m_pNextFree =
                                pHead = m_pFreeHead.template load<membar_acquire>() ;
                        } while ( !m_pFreeHead.template cas<membar_release>( pHead, pNew->items )) ;
                    }
                }
                //@endcond

            public:
                //@cond
                retired_ptr_pool()
                    : m_pBlockListHead(NULL)
                    , m_pFreeHead( NULL )
                {
                    allocNewBlock() ;
                }

                ~retired_ptr_pool()
                {
                    block * p   ;
                    for ( block * pBlock = m_pBlockListHead.template load<membar_relaxed>(); pBlock; pBlock = p ) {
                        p = pBlock->pNext   ;
                        m_BlockAllocator.Delete( pBlock )   ;
                    }
                }
                //@endcond

                /// Allocates new retired pointer
                retired_ptr_node&  alloc()
                {
                    item * pItem ;
                    do {
                retry:
                        pItem = m_pFreeHead.template load<membar_acquire>() ;
                        if ( pItem == NULL ) {
                            allocNewBlock() ;
                            goto retry      ;
                        }
                    } while( !m_pFreeHead.template cas<membar_release>( pItem, pItem->m_pNextFree )) ;

                    return *pItem  ;
                }

                /// Allocates and initializes new retired pointer
                retired_ptr_node& alloc( const retired_ptr& p )
                {
                    retired_ptr_node& node = alloc()    ;
                    node.m_ptr = p  ;
                    return node     ;
                }

                /// Places retired pointer to pool (frees retired pointer)
                void free( retired_ptr_node& p )
                {
                    item * pItem = &p   ;
                    item * pHead        ;
                    do {
                        pItem->m_pNextFree =
                            pHead = m_pFreeHead.template load<membar_acquire>() ;
                    } while ( !m_pFreeHead.template cas<membar_release>( pHead, pItem ))    ;
                }

                /// Places the list (pHead, pTail) of retired pointers to pool (frees retired pointers)
                /**
                    The list is linked to the m_pNextFree field
                */
                void freeList( retired_ptr_node * pHead, retired_ptr_node * pTail )
                {
                    assert( pHead != NULL ) ;
                    assert( pTail != NULL ) ;

                    item * pCurHead        ;
                    do {
                        pTail->m_pNextFree =
                            pCurHead = m_pFreeHead.template load<membar_acquire>() ;
                    } while ( !m_pFreeHead.template cas<membar_release>( pCurHead, pHead ))    ;
                }
            };

            /// Uninitialized guard
            class guard: public boost::noncopyable
            {
                friend class ThreadGC   ;
            protected:
                details::guard_data * m_pGuard ;    ///< Pointer to guard data
            public:
                /// Initialize empty guard.
                CDS_CONSTEXPR guard()
                    : m_pGuard( NULL )
                {}

                /// Object destructor, does nothing
                ~guard()
                {}

                /// Guards pointer \p p
                void set( void * p )
                {
                    assert( m_pGuard != NULL )      ;
                    m_pGuard->pPost.store<membar_release>( p ) ;
                    CDS_COMPILER_RW_BARRIER         ;
                }

                /// Clears the guard
                void clear()
                {
                    assert( m_pGuard != NULL )      ;
                    m_pGuard->pPost.store<membar_release>( (void *) NULL )    ;
                    CDS_COMPILER_RW_BARRIER         ;
                }

                /// Guards pointer \p p
                template <typename T>
                T * operator =( T * p )
                {
                    set( reinterpret_cast<void *>( const_cast<T *>(p) ))      ;
                    return p    ;
                }

            public: // for ThreadGC.
                /*
                    GCC cannot compile code for template versions of ThreasGC::allocGuard/freeGuard,
                    the compiler produces error: ‘cds::gc::ptb::details::guard_data* cds::gc::ptb::details::guard::m_pGuard’ is protected
                    despite the fact that ThreadGC is declared as friend for guard class.
                    We should not like to declare m_pGuard member as public one.
                    Therefore, we have to add set_guard/get_guard public functions
                */
                /// Set guard data
                void set_guard( details::guard_data * pGuard )
                {
                    assert( m_pGuard == NULL )  ;
                    m_pGuard = pGuard ;
                }

                /// Get current guard data
                details::guard_data * get_guard()
                {
                    return m_pGuard ;
                }
                /// Get current guard data
                details::guard_data * get_guard() const
                {
                    return m_pGuard ;
                }
            };

        } // namespace details

        /// Guard
        /**
            This class represents auto guard: ctor allocates a guard from guard pool,
            dtor returns the guard back to the pool of free guard.
        */
        class Guard: public details::guard
        {
            //@cond
            typedef details::guard    base_class    ;
            friend class ThreadGC    ;
            //@endcond

            ThreadGC&    m_gc    ;    ///< ThreadGC object of current thread
        public:
            /// Allocates a guard from \p gc GC. \p gc must be ThreadGC object of current thread
            Guard(ThreadGC& gc)    ;

            /// Returns guard allocated back to pool of free guards
            ~Guard();    // inline after GarbageCollector

            /// Returns PTB GC object
            ThreadGC& getGC()
            {
                return m_gc    ;
            }

            /// Guards pointer \p p
            template <typename T>
            T * operator =( T * p )
            {
                return base_class::operator =<T>( p )    ;
            }
        };

        /// Array of guards
        /**
            This class represents array of auto guards: ctor allocates COUNT guards from guard pool,
            dtor returns the guards allocated back to the pool.
        */
        template <size_t COUNT>
        class GuardArray: public boost::noncopyable
        {
            details::guard      m_arr[COUNT]    ;    ///< array of guard
            ThreadGC&            m_gc    ;            ///< ThreadGC object of current thread

        public:
            /// Rebind array for other size \p COUNT2
            template <size_t COUNT2>
            struct rebind {
                typedef GuardArray<COUNT2>  other   ;   ///< rebinding result
            };

        public:
            /// Allocates array of guards from \p gc which must be the ThreadGC object of current thread
            GuardArray( ThreadGC& gc )    ;    // inline below

            /// Returns guards allocated back to pool
            ~GuardArray()    ;    // inline below

            /// Returns the capacity of array
            size_t capacity() const
            {
                return COUNT    ;
            }

            /// Returns PTB ThreadGC object
            ThreadGC& getGC()
            {
                return m_gc ;
            }

            /// Returns reference to the guard of index \p nIndex (0 <= \p nIndex < \p COUNT)
            details::guard& operator []( size_t nIndex )
            {
                assert( nIndex < COUNT )    ;
                return m_arr[nIndex]        ;
            }

            /// Set the guard \p nIndex. 0 <= \p nIndex < \p COUNT
            template <typename T>
            void set( size_t nIndex, T * p )
            {
                assert( nIndex < COUNT )    ;
                m_arr[nIndex].set( p )      ;
            }

            /// Clears (sets to NULL) the guard \p nIndex
            void clear( size_t nIndex )
            {
                assert( nIndex < COUNT )    ;
                m_arr[nIndex].clear()       ;
            }

            /// Clears all guards in the array
            void clearAll()
            {
                for ( size_t i = 0; i < COUNT; ++i )
                    clear(i)    ;
            }
        };

        /// Memory manager (Garbage collector)
        class CDS_EXPORT_API GarbageCollector
        {
            friend class ThreadGC   ;
        public:
            /// Exception "No GarbageCollector object is created"
            CDS_DECLARE_EXCEPTION( PTBManagerEmpty, "Global PTB GarbageCollector is NULL" )    ;

            /// Internal GC statistics
            struct InternalState
            {
                atomic<size_t>  m_nGuardCount       ;   ///< Total guard count
                atomic<size_t>  m_nFreeGuardCount   ;   ///< Count of free guard

                //@cond
                InternalState()
                    : m_nGuardCount(0)
                    , m_nFreeGuardCount(0)
                {}

                InternalState( const InternalState& s )
                    : m_nGuardCount( s.m_nGuardCount.load<membar_relaxed>())
                    , m_nFreeGuardCount( s.m_nFreeGuardCount.load<membar_relaxed>())
                {}
                //@endcond
            };

        private:
            static GarbageCollector * m_pManager    ;   ///< GC global instance

            details::guard_allocator<>      m_GuardPool         ;   ///< Guard pool
            details::retired_ptr_pool<>     m_RetiredAllocator  ;   ///< Pool of free retired pointers
            details::retired_ptr_buffer     m_RetiredBuffer     ;   ///< Retired pointer buffer to liberate
            lock::Spin                      m_lockLiberate      ;   ///< Access to "liberate" procedure

            const size_t    m_nLiberateThreshold    ;   ///< Max size of retired pointer buffer to call liberate
            const size_t    m_nInitialThreadGuardCount; ///< Initial count of guards allocated for ThreadGC

            InternalState   m_stat  ;   ///< Internal statistics
            bool            m_bStatEnabled  ;   ///< Internal Statistics enabled

        public:
            /// Initializes PTB memory manager singleton
            /**
                This member function creates and initializes PTB global object.
                The function should be called before using CDS data structure based on cds::gc::ptb_gc GC. Usually,
                this member function is called in the \p main() function. See cds::gc::ptb for example.
                After calling of this function you may use CDS data structures based on PTB GC cds::gc::ptb_gc.

                \par Parameters
                \li \p nLiberateThreshold - the liberate threshold. When count of retired pointers reaches this value,
                    the \ref liberate member function would called for freeing retired pointers.
                    If \p nLiberateThreshold <= 1, \p liberate would called after each \ref retirePtr call.
                \li \p nInitialThreadGuardCount - initial count of guard allocated for ThreadGC. When a thread
                    is initialized the GC allocates local guard pool for the thread from common guard pool.
                    By perforce the local thread's guard pool is grown automatically from common pool. When
                    the thread terminated its guard pool is backed to common GC's pool.

            */
            static void CDS_STDCALL Construct(
                size_t nLiberateThreshold = 1024
                , size_t nInitialThreadGuardCount = 8
            ) ;

            /// Destroys PTB memory manager
            /**
                The member function destroys PTB global object. After calling of this function you may \b NOT
                use CDS data structures based on PTB GC cds::gc::ptb_gc. Usually, the \p Destruct function is called
                at the end of your \p main(). See cds::gc::ptb for example.
            */
            static void CDS_STDCALL Destruct()  ;

            /// Returns pointer to GarbageCollector instance
            /**
                If PTB GC is not initialized, \p PTBManagerEmpty exception is thrown
            */
            static GarbageCollector&   instance()
            {
                if ( m_pManager == NULL )
                    throw PTBManagerEmpty()    ;
                return *m_pManager   ;
            }

            /// Checks if global GC object is constructed and may be used
            static bool isUsed()
            {
                return m_pManager != NULL    ;
            }

        public:
            //@{
            /// Internal interface

            /// Allocates a guard
            details::guard_data * allocGuard()
            {
                return m_GuardPool.alloc()    ;
            }

            /// Frees guard \p g for reusing in future
            void freeGuard(details::guard_data * pGuard )
            {
                m_GuardPool.free( pGuard )   ;
            }

            /// Allocates guard list for a thread.
            details::guard_data * allocGuardList( size_t nCount )
            {
                return m_GuardPool.allocList( nCount )  ;
            }

            /// Frees thread's guard list pointed by \p pList
            void freeGuardList( details::guard_data * pList )
            {
                m_GuardPool.freeList( pList )    ;
            }

            /// Places retired pointer \p and its deleter \p pFunc into thread's array of retired pointer for deferred reclamation
            template <typename T>
            void retirePtr( T * p, void (* pFunc)(T *) )
            {
                retirePtr( retired_ptr( reinterpret_cast<void *>( p ), reinterpret_cast<free_retired_ptr_func>( pFunc ) ) )    ;
            }

            /// Places retired pointer \p into thread's array of retired pointer for deferred reclamation
            void retirePtr( const retired_ptr& p )
            {
                if ( m_RetiredBuffer.push( m_RetiredAllocator.alloc(p)) >= m_nLiberateThreshold )
                    try_liberate()  ;
            }

            /// Calls liberate function 
            /**
                This function tries to call \ref liberate procedure if it is not active.
                Returns \p true, if calling of \ref liberate is succeeded, \p false otherwise.

                This function may be called directly if needed.
            */
            bool try_liberate() 
            {
                if ( m_lockLiberate.tryLock() ) {
                    lock::AutoUnlock<lock::Spin>    aus(m_lockLiberate) ;
                    liberate()  ;
                    return true ;
                }
                return false    ;
            }

        protected:
            /// Liberate function
            /**
                The main function of Pass The Buck algorithm. It tries to free retired pointers if they are not
                trapped by any guard.
            */
            void liberate() ;

            //@}

        private:
            //@cond
            void liberate( details::liberate_set& set )    ;
            void free_liberated( details::liberate_set& set )    ;
            //@endcond

        public:
            //@{
            /// Internal statistics

            /// Get internal statistics
            InternalState& getInternalState(InternalState& stat) const
            {
                stat = m_stat   ;
                return stat   ;
            }

            /// Checks if internal statistics enabled
            bool              isStatisticsEnabled() const
            {
                return m_bStatEnabled   ;
            }

            /// Enables/disables internal statistics
            bool              enableStatistics( bool bEnable )
            {
                bool bEnabled = m_bStatEnabled    ;
                m_bStatEnabled = bEnable        ;
                return bEnabled                    ;
            }

            //@}

        private:
            //@cond none
            GarbageCollector( size_t nLiberateThreshold, size_t nInitialThreadGuardCount )  ;
            ~GarbageCollector() ;
            //@endcond
        };

        /// Thread GC
        /**
            To use Pass The Buck reclamation schema each thread object must be linked with the object of ThreadGC class
            that interacts with GarbageCollector global object. The linkage is performed by calling cds::threading \p ::Manager::attachThread()
            on the start of each thread that uses PTB GC. Before terminating the thread linked to PTB GC it is necessary to call
            cds::threading \p ::Manager::detachThread().

            The ThreadGC object maintains two list:
            \li Thread guard list: the list of thread-local guards (linked by \p pThreadNext field)
            \li Free guard list: the list of thread-local free guards (linked by \p pNextFree field)
            Free guard list is a subset of thread guard list.
        */
        class ThreadGC: boost::noncopyable
        {
            GarbageCollector&   m_gc    ;   ///< reference to GC singleton
            details::guard_data *    m_pList ;   ///< Local list of guards owned by the thread
            details::guard_data *    m_pFree ;   ///< The list of free guard from m_pList

        public:
            ThreadGC()
                : m_gc( GarbageCollector::instance() )
                , m_pList( NULL )
                , m_pFree( NULL )
            {}

            /// Dtor calls fini()
            ~ThreadGC()
            {
                fini()  ;
            }

            /// Initialization. Repeat call is available
            void init()
            {
                if ( !m_pList ) {
                    m_pList =
                        m_pFree = m_gc.allocGuardList( m_gc.m_nInitialThreadGuardCount )   ;
                }
            }

            /// Finalization. Repeat call is available
            void fini()
            {
                if ( m_pList ) {
                    m_gc.freeGuardList( m_pList )   ;
                    m_pList =
                        m_pFree = NULL ;
                }
            }

        public:
            /// Initializes guard \p g
            void allocGuard( Guard& g )
            {
                assert( m_pList != NULL )   ;
                if ( m_pFree ) {
                    g.m_pGuard = m_pFree    ;
                    m_pFree = m_pFree->pNextFree.load<membar_relaxed>()    ;
                }
                else {
                    g.m_pGuard = m_gc.allocGuard()        ;
                    g.m_pGuard->pThreadNext = m_pList    ;
                    m_pList = g.m_pGuard    ;
                }
            }

            /// Frees guard \p g
            void freeGuard( Guard& g )
            {
                assert( m_pList != NULL )   ;
                g.m_pGuard->pNextFree.store<membar_relaxed>( m_pFree ) ;
                m_pFree = g.m_pGuard    ;
            }

            /// Initializes guard array \p arr
            template <size_t COUNT>
            void allocGuard( GuardArray<COUNT>& arr )
            {
                assert( m_pList != NULL )   ;
                size_t nCount = 0    ;

                while ( m_pFree && nCount < COUNT ) {
                    arr[nCount].set_guard( m_pFree )    ;
                    m_pFree = m_pFree->pNextFree.load<membar_relaxed>()    ;
                    ++nCount    ;
                }

                while ( nCount < COUNT ) {
                    details::guard& g = arr[nCount++]    ;
                    g.set_guard( m_gc.allocGuard() )   ;
                    g.get_guard()->pThreadNext = m_pList    ;
                    m_pList = g.get_guard()    ;
                }
            }

            /// Frees guard array \p arr
            template <size_t COUNT>
            void freeGuard( GuardArray<COUNT>& arr )
            {
                assert( m_pList != NULL )   ;

                for ( size_t i = 0; i < COUNT - 1; ++i ) {
                    arr[i].get_guard()->pNextFree.template store<membar_relaxed>( arr[i+1].get_guard() ) ;
                }
                arr[COUNT-1].get_guard()->pNextFree.template store<membar_relaxed>( m_pFree ) ;
                m_pFree = arr[0].get_guard()    ;
            }

            /// Places retired pointer \p and its deleter \p pFunc into list of retired pointer for deferred reclamation
            template <typename T>
            void retirePtr( T * p, void (* pFunc)(T *) )
            {
                m_gc.retirePtr( p, pFunc )  ;
            }

            /// Places retired pointer \p into list of retired pointer for deferred reclamation
            /*
            void retirePtr( const retired_ptr& p )
            {
                m_gc.retirePtr( p ) ;
            }
            */
        };

        //////////////////////////////////////////////////////////
        // Inlines

        inline Guard::Guard(ThreadGC& gc)
            : m_gc( gc )
        {
            getGC().allocGuard( *this )    ;
        }
        inline Guard::~Guard()
        {
            getGC().freeGuard( *this )  ;
        }

        template <size_t COUNT>
        inline GuardArray<COUNT>::GuardArray( ThreadGC& gc )
            : m_gc( gc )
        {
            getGC().allocGuard( *this ) ;
        }
        template <size_t COUNT>
        inline GuardArray<COUNT>::~GuardArray()
        {
            getGC().freeGuard( *this )  ;
        }

    }   // namespace ptb
}}  // namespace cds::gc

#if CDS_COMPILER == CDS_COMPILER_MSVC
#   pragma warning(pop)
#endif


#endif // #ifndef __CDS_GC_PTB_PASS_THE_BUCK_H
