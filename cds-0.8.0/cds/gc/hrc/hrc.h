/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_HRC_SCHEMA_H
#define __CDS_GC_HRC_SCHEMA_H

/*
    Editions:
        2008.03.08    Maxim.Khiszinsky    Created
*/

#include <cds/gc/hrc_gc.h>
#include <cds/refcounter.h>
#include <cds/lock/spinlock.h>
#include <cds/details/markptr.h>

#include <cds/gc/hrc/details/hrc_fwd.h>
#include <cds/gc/hrc/details/hrc_retired.h>

#include <cds/gc/hzp/details/hp_alloc.h>

#include <boost/noncopyable.hpp>

namespace cds { namespace gc {


    /// Gidenstam's memory reclamation schema (HRC)
    /**
        Gidenstam's lock-free memory reclamation schema based on reference counting (HRC)

    \par Sources:
        \li [2006] A.Gidenstam "Algorithms for synchronization and consistency
                in concurrent system services", Chapter 5 "Lock-Free Memory Reclamation"
                Thesis for the degree of Doctor    of Philosophy
        \li [2005] Anders Gidenstam, Marina Papatriantafilou and Philippas Tsigas "Allocating
                memory in a lock-free manner", Proceedings of the 13th Annual European
                Symposium on Algorithms (ESA 2005), Lecture Notes in Computer
                Science Vol. 3669, pages 229 – 242, Springer-Verlag, 2005

    \par
        This reclamation schema combines Michael's Hazard Pointer schema (see cds::gc::hzp)
        for deferred safe reclamation of unused objects and the reference counting mechanism
        for controlling lifetime of the objects.

    @cond
    \par Usage:
        HRC garbage collector is the singleton. The main user-level part of HRC schema is 
        GC class and its nested classes. Before use any HRC-related class you must initialize HRC garbage collector 
        by contructing cds::gc::hrc::GC object in beginning of your main(). 
        See cds::gc::hrc::GC class for explanation.
    @endcond
    */
    namespace hrc {

        /// Base class for all HRC-based container's node
        /**
            This interface is placed to the separate class since in the presence of a garbage collector 
            the lifetime of the node is greater than lifetime of its container. 
            Reclaimed node may be physically deleted later than its container.
            So, the ContainerNode must be smarter than the usual.
        */
        class ContainerNode
        {
        protected:
            unsigned_ref_counter    m_RC        ;    ///< reference counter
            atomic<bool>            m_bTrace    ;    ///< @a true - node is tracing by HRC
            atomic<bool>            m_bDeleted  ;    ///< @ true - node is deleted

            friend class GarbageCollector    ;
            friend class ThreadGC            ;

        protected:
            //@cond
            ContainerNode() ;               // inline, see hrc_inline.h
            virtual ~ContainerNode()    ;   // inline, see hrc_inline.h
            //@endcond

        public:
            /// Returns count of reference for the node
            unsigned_ref_counter::ref_counter_type  getRefCount() const { return m_RC.value<membar_acquire>(); }

            /// Increments the reference counter of the node
            void            incRefCount()    { ++m_RC; }

            /// Decrements the reference counter of the node. Returns \p true if ref counter is 0.
            bool            decRefCount()    { return --m_RC == 0; }

            /// Returns the mark that the node is deleted or not
            bool            isDeleted() const { return m_bDeleted.load<membar_acquire>(); }

        protected:    // GC interface
            /**
                [Gidenstam 2006]: "The procedure \p CleanUpNode will make sure that all claimed references from
                the links of the given node will only point to active nodes, thus removing redundant
                passages through an arbitrary number of deleted nodes"

                The pseudocode of this method must be like following:
                \code
                void cleanUp( ThreadGC * pGC )
                    for all x where link[x] of node is reference-counted do
                retry:
                        node1 := link[x]    ;
                        if node1 != NULL and node1.m_bDeleted then
                            node2 := node1->link[x]    ;
                            pGC->CASRef( this->link[x], node1, node2 );
                            pGC->releaseRef( node2 );
                            pGC->releaseRef( node1 );
                            goto retry;
                        pGC->releaseRef(node1);
                \endcode

                Be aware to use hazard pointers inside implementation of this method. cleanUp is called from
                the container's method when deleting the nodes. However, some hazard pointers may be occupied
                by container's method. You should allocate new hazard pointers inside cleanUp method, for example:
                \code 
                    gc::hrc::AutoHPArray<2> hpArr( *pGC )    ; 
                \endcode
            */
            virtual void    cleanUp( ThreadGC * pGC ) = 0    ;

            /**
                [Gidenstam 2006]: "The procedure \p TerminateNode will make sure that none of the links in the
                given node will have any claim on any other node. TerminateNode is called on
                a deleted node when there are no claims from any other node or thread to the
                node"

                The pseudocode of this method must be like following:
                \code
                void terminate( ThreadGC * pGC, bool bConcurrent)
                    if !bConcurrent
                        for all this->link where link is reference-counted do
                            link := NULL ;
                    else
                        for all this->link where link is reference-counted do
                            repeat node1 := link ;
                            until pGC->CASRef(link,node1,NULL);
                \endcode
            */
            virtual void    terminate( ThreadGC * pGC, bool bConcurrent ) = 0 ;

        public:
            /// Method to destroy (deallocate) node. Depends on node's allocator
            virtual void    destroy() = 0 ;
        };

        /// HRC GC implementation details
        namespace details {

            /// Hazard pointer guard
            typedef gc::hzp::details::HPGuardT<ContainerNode *>    HPGuard ;

            /// Array of hazard pointers.
            /**
                This is wrapper for cds::gc::hzp::details::HPArray <COUNT> class
            */
            template <size_t COUNT>
            class HPArray: public gc::hzp::details::HPArrayT<ContainerNode *, COUNT>
            {};

            /// HP record of the thread
            /**
                This structure is single writer - multiple reader type. The writer is the thread owned the record
            */
            struct thread_descriptor {
                typedef ContainerNode *                     TEntry       ; ///< base type of hazard pointer
                typedef cds::details::BoundedArray<TEntry>  THRCArray    ; ///< type of array of hazard pointers (implicit \ref CDS_DEFAULT_ALLOCATOR dependence)

                hzp::details::HPAllocator<TEntry>   m_hzp           ;   ///< array of hazard pointers. Implicit \ref CDS_DEFAULT_ALLOCATOR dependence
                details::retired_vector             m_arrRetired    ;   ///< array of retired pointers

                //@cond
                thread_descriptor( const GarbageCollector& HzpMgr ) ;    // inline
                ~thread_descriptor()
                {}
                //@endcond

                /// clear all hazard pointers
                void clear()
                {
                    m_hzp.clear()    ;
                }
            };
        }    // namespace details

        /// Gidenstam's Garbage Collector
        /**
            This GC combines Hazard Pointers (HP) reclamation method by Michael's and the well-known reference counting
            reclamation schema. The HP method is light-weight algorithm guarding local references only. Reference counting
            schema is harder than HP with relation to the performance but can guard global references too.
            Using Gidenstam's GC it can be possible to safely introduce to the lock-free data structures
            very useful concepts like iterators.

            GarbageCollector is the singleton.
        */
        class CDS_EXPORT_API GarbageCollector
        {
        public:

            /// GC internal statistics
            struct internal_state {
                size_t              nHPCount                ;   ///< HP count per thread (const)
                size_t              nMaxThreadCount         ;   ///< Max thread count (const)
                size_t              nMaxRetiredPtrCount     ;   ///< Max retired pointer count per thread (const)
                size_t              nHRCRecSize             ;   ///< Size of HRC record, bytes (const)

                size_t              nHRCRecAllocated        ;   ///< Count of HRC record allocations
                size_t              nHRCRecUsed             ;   ///< Count of HRC record used
                size_t              nTotalRetiredPtrCount   ;   ///< Current total count of retired pointers
                size_t              nRetiredPtrInFreeHRCRecs;   ///< Count of retired pointer in free (unused) HP records

                /// Event counter type
                typedef atomics::event_counter::value_type  event_value_type ;

                event_value_type    evcAllocHRCRec        ; ///< Event count of @ref details::thread_descriptor allocation
                event_value_type    evcRetireHRCRec        ; ///< Event count of @ref details::thread_descriptor reclamation
                event_value_type    evcAllocNewHRCRec    ; ///< Event count of new @ref details::thread_descriptor allocation
                event_value_type    evcDeleteHRCRec        ; ///< Event count of deletion of @ref details::thread_descriptor
                event_value_type    evcScanCall            ; ///< Number of calls Scan
                event_value_type    evcHelpScanCalls    ; ///< Number of calls HelpScan
                event_value_type    evcCleanUpAllCalls  ; ///< Number of calls CleanUpAll
                event_value_type    evcDeletedNode        ; ///< Node deletion event counter
                event_value_type    evcScanGuarded      ; ///< Count of retired nodes that could not be deleted on Scan phase
                event_value_type    evcScanClaimGuarded ; ///< Count of retired node that could not be deleted on Scan phase because of m_nClaim != 0

#ifdef CDS_DEBUG
                event_value_type    evcNodeConstruct    ; ///< Count of constructed ContainerNode
                event_value_type    evcNodeDestruct     ; ///< Count of destructed ContainerNode
#endif
            };

            /// "Global GC object is NULL" exception
            CDS_DECLARE_EXCEPTION( HRCGarbageCollectorEmpty, "Global cds::gc::hrc::GarbageCollector is NULL" )    ;

            /// Not enough required Hazard Pointer count
            CDS_DECLARE_EXCEPTION( HRCTooMany, "Not enough required Hazard Pointer count" )    ;

        private:
            /// Internal statistics by events
            struct statistics {
                atomics::event_counter  m_AllocHRCThreadDesc        ; ///< Event count of @ref details::thread_descriptor allocation
                atomics::event_counter  m_RetireHRCThreadDesc        ; ///< Event count of @ref details::thread_descriptor reclamation
                atomics::event_counter  m_AllocNewHRCThreadDesc        ; ///< Event count of new @ref details::thread_descriptor allocation
                atomics::event_counter  m_DeleteHRCThreadDesc        ; ///< Event count of deletion of @ref details::thread_descriptor
                atomics::event_counter  m_ScanCalls                    ; ///< Number of calls Scan
                atomics::event_counter  m_HelpScanCalls             ; ///< Number of calls HelpScan
                atomics::event_counter  m_CleanUpAllCalls           ; ///< Number of calls CleanUpAll

                atomics::event_counter  m_DeletedNode                ; ///< Node deletion event counter
                atomics::event_counter  m_ScanGuarded               ; ///< Count of retired nodes that could not be deleted on Scan phase
                atomics::event_counter  m_ScanClaimGuarded          ; ///< Count of retired node that could not be deleted on Scan phase because of m_nClaim != 0

#           ifdef CDS_DEBUG
                atomics::event_counter  m_NodeConstructed           ; ///< Count of ContainerNode constructed
                atomics::event_counter  m_NodeDestructed            ; ///< Count of ContainerNode destructed
#           endif
            };

            /// HRC control structure of global thread list
            struct thread_list_node: public details::thread_descriptor
            {
                thread_list_node *            m_pNext     ; ///< next list record
                ThreadGC *                    m_pOwner    ; ///< Owner of record
                atomic<cds::OS::ThreadId>   m_idOwner   ; ///< Id of thread owned; 0 - record is free
                bool                        m_bFree        ; ///< Node is help-scanned

                //@cond
                thread_list_node( const GarbageCollector& HzpMgr )
                    : thread_descriptor( HzpMgr ),
                    m_pNext(NULL),
                    m_pOwner( NULL ),
                    m_idOwner( cds::OS::nullThreadId() ),
                    m_bFree( false )
                {}

                ~thread_list_node()
                {
                    assert( m_pOwner == NULL )    ;
                    assert( m_idOwner.load<membar_relaxed>() == cds::OS::nullThreadId() )    ;
                }
                //@endcond
            };

        private:
            thread_list_node * volatile m_pListHead  ;  ///< Head of thread list

            static GarbageCollector *    m_pGC    ;    ///< HRC garbage collector instance

            statistics                m_Stat        ;    ///< Internal statistics
            bool                    m_bStatEnabled    ;    ///< @a true - accumulate internal statistics

            const size_t            m_nHazardPointerCount    ;    ///< max count of thread's hazard pointer
            const size_t            m_nMaxThreadCount        ;    ///< max count of thread
            const size_t            m_nMaxRetiredPtrCount    ;    ///< max count of retired ptr per thread

        private:
            //@cond
            GarbageCollector(
                size_t nHazardPtrCount,            ///< number of hazard pointers
                size_t nMaxThreadCount,            ///< max number of threads
                size_t nRetiredNodeArraySize    ///< size of array of retired node
            )    ;
            ~GarbageCollector()    ;
            //@endcond

            /// Allocates new HRC control structure from the heap (using operator new)
            thread_list_node *    newHRCThreadDesc()    ;

            /// Deletes \p pNode control structure
            void                deleteHRCThreadDesc( thread_list_node * pNode )    ;

            /// Clears retired nodes of \p pNode control structure
            void                clearHRCThreadDesc( thread_list_node * pNode )    ;

            /// Finds HRC control structure for current thread
            thread_list_node *    getHRCThreadDescForCurrentThread() const ;

        public:
            /// Create global instance of GarbageCollector
            static void    CDS_STDCALL    Construct(
                size_t nHazardPtrCount = 0,        ///< number of hazard pointers
                size_t nMaxThreadCount = 0,        ///< max threads count
                size_t nMaxNodeLinkCount = 0,    ///< max number of links a @ref ContainerNode can contain
                size_t nMaxTransientLinks = 0    ///< max number of links in live nodes that may transiently point to a deleted node
                );

            /// Destroy global instance of GarbageCollector
            static void    CDS_STDCALL        Destruct()    ;

            /// Get global instance of GarbageCollector
            static GarbageCollector&   instance()
            {
                if ( m_pGC == NULL )
                    throw HRCGarbageCollectorEmpty()    ;
                return *m_pGC   ;
            }

            /// Checks if global GC object is constructed and may be used
            static bool isUsed()
            {
                return m_pGC != NULL    ;
            }

            /// Get max count of hazard pointers as defined in @ref Construct call
            size_t            getHazardPointerCount() const        { return m_nHazardPointerCount; }
            /// Get max thread count as defined in @ref Construct call
            size_t            getMaxThreadCount() const             { return m_nMaxThreadCount; }
            /// Get max retired pointers count. It is calculated by the parameters of @ref Construct call
            size_t            getMaxRetiredPtrCount() const        { return m_nMaxRetiredPtrCount; }

            /// Get internal statistics
            internal_state& getInternalState( internal_state& stat) const ;

            /// Check if statistics enabled
            bool              isStatisticsEnabled() const { return m_bStatEnabled; }

            /// Enable internal statistics
            bool              enableStatistics( bool bEnable )
            {
                bool bCurEnabled = m_bStatEnabled    ;
                m_bStatEnabled = bEnable            ;
                return bCurEnabled                    ;
            }

            /// Checks that required hazard pointer count \p nRequiredCount is less or equal then max hazard pointer count
            /**
                If \p nRequiredCount > getHazardPointerCount() then the exception HZPTooMany is thrown
            */
            static void checkHPCount( unsigned int nRequiredCount )
            {
                if ( instance().getHazardPointerCount() < nRequiredCount )
                    throw HRCTooMany()  ;
            }

        public:    // Internals for threads

            /// Allocates HRC thread descriptor (thread interface)
            details::thread_descriptor * allocateHRCThreadDesc( ThreadGC * pThreadGC )    ;

            /// Retires HRC thread descriptor (thread interface)
            void retireHRCThreadDesc( details::thread_descriptor * pRec )    ;

            /// The main method of GC
            /**
                The procedure searches through all not yet reclaimed nodes deleted by this thread
                and reclaim only those that does not have any matching hazard pointers and do not have any
                counted references from any links inside of nodes.
                @a Scan is called in context of thread owned \p pRec.
            */
            void Scan( ThreadGC * pThreadGC )    ;

            /// Manage free thread_descriptor records and move all retired pointers to \p pThreadGC
            void HelpScan( ThreadGC * pThreadGC )    ;

            /// Global clean up
            /**
                The procedure try to remove redundant claimed references from links in deleted nodes
                that has been deleted by any thread. \p pThreadGC - ThreadGC of calling thread
            */
            void CleanUpAll( ThreadGC * pThreadGC )    ;

#   ifdef CDS_DEBUG
        public:
            //@cond
            void dbgNodeConstructed() { ++m_Stat.m_NodeConstructed; }
            void dbgNodeDestructed()  { ++m_Stat.m_NodeDestructed;  }
            //@endcond
#   endif

        };

        class AutoHPGuard   ;

        /// Thread's Garbage collector
        /**
            To use HRC reclamation schema each thread object must be linked with the object of ThreadGC class
            that interacts with GarbageCollector global object. The linkage is performed by calling cds::threading \p Manager::attachThread()
            on the start of each thread that uses HRC GC. Before terminating the thread linked to HRC GC it is necessary to call
            cds::threading \p Manager::detachThread().
        */
        class ThreadGC: boost::noncopyable
        {
            GarbageCollector&                m_gc    ;    ///< master garbage collector
            details::thread_descriptor *    m_pDesc    ;    ///< descriptor of GC data for the thread

            friend class GarbageCollector    ;

        public:
            //@cond
            ThreadGC()
                : m_gc( GarbageCollector::instance() )
                , m_pDesc( NULL )
            {}
            ~ThreadGC()
            {
                fini()    ;
            }
            //@endcond

            /// Checks if thread GC is initialized
            bool    isInitialized() const   { return m_pDesc != NULL ; }

            /// Initialization. Multiple calls is allowed
            void init()
            {
                if ( !m_pDesc )
                    m_pDesc = m_gc.allocateHRCThreadDesc( this ) ;
            }

            /// Finalization. Multiple calls is allowed
            void fini()
            {
                if ( m_pDesc ) {
                    cleanUpLocal()      ;
                    m_gc.Scan( this )   ;
                    details::thread_descriptor * pRec = m_pDesc    ;
                    m_pDesc = NULL    ;
                    if  ( pRec )
                        m_gc.retireHRCThreadDesc( pRec ) ;
                }
            }

        public:    // HRC garbage collector methods

            /// Initializes HP guard \p guard
            void allocGuard( details::HPGuard& guard )
            {
                assert( m_pDesc != NULL )    ;
                m_pDesc->m_hzp.alloc( guard )    ;
            }

            /// Frees HP guard \p guard
            void freeGuard( details::HPGuard& guard )
            {
                assert( m_pDesc != NULL )    ;
                m_pDesc->m_hzp.free( guard )    ;
            }

            /// Initializes HP guard array \p arr
            template <size_t COUNT>
            void allocGuard( details::HPArray<COUNT>& arr )
            {
                assert( m_pDesc != NULL )    ;
                m_pDesc->m_hzp.alloc( arr )    ;
            }

            /// Frees HP guard array \p arr
            template <size_t COUNT>
            void freeGuard( details::HPArray<COUNT>& arr )
            {
                assert( m_pDesc != NULL )    ;
                m_pDesc->m_hzp.free( arr )    ;
            }

            /// Safely de-references \p ppNode link
            /**
                Safely de-references \p ppNode link and sets \p hp hazard pointer to the de-referenced node
                \p TNODE - class derived from ContainerNode type
            */
            template <typename TNODE>
            static TNODE *    derefLink( TNODE * volatile * ppNode, ContainerNode *& hp )
            {
                TNODE * p    ;
                while ( true ) {
                    p = atomics::load<membar_relaxed>( ppNode ) ;
                    atomics::store<membar_release>( &hp, static_cast<ContainerNode *>( p ))    ;
                    if ( atomics::load<membar_acquire>( ppNode ) == p )
                        break    ;
                }
                return p    ;
            }

            /// Safely de-references \p ppNode link
            /**
                Safely de-references \p ppNode link and sets \p hp hazard pointer to the de-referenced node
                \p TNODE - class derived from ContainerNode type
            */
            template <typename TNODE>
            static TNODE *    derefLink( TNODE * volatile * ppNode, details::HPGuard& hp )
            {
                return derefLink( ppNode, hp.getHPRef() )   ;
            }
            //@cond
            template <typename T>
            static T *    derefLink( atomic<T *>& refNode, details::HPGuard& hp )
            {
                T * p    ;
                while ( true ) {
                    p = refNode.template load<membar_relaxed>() ;
                    atomics::store<membar_release>( &(hp.getHPRef()), static_cast<ContainerNode *>( p ))    ;
                    if ( refNode.template load<membar_acquire>() == p )
                        break    ;
                }
                return p    ;
            }
            //@endcond

            /// Safely de-reference \p ppNode marked_node
            /**
                Safely de-references \p ppNode link and sets \p hp hazard pointer to the de-referenced node.
                \p TNODE - class derived from ContainerNode type, wrapped by marked_ptr<TNODE, BITS>
            */
            template <typename TNODE, int BITS>
            static cds::details::marked_ptr<TNODE, BITS> derefLink( cds::details::marked_ptr<TNODE, BITS> * ppNode, ContainerNode *& hp )
            {
                cds::details::marked_ptr<TNODE, BITS> p    ;
                while ( true ) {
                    p = atomics::load<membar_relaxed>( ppNode )   ;
                    atomics::store<membar_release>( &hp, static_cast<ContainerNode *>( p.ptr() ))    ;
                    if ( atomics::load<membar_acquire>( ppNode ) == p )
                        break    ;
                }
                return p    ;
            }

            /// Releases reference
            /**
                The procedure should be called when a given node
                will not be accessed by the current thread anymore. It simply clears \p hp hazard pointer

                Note that not all of containers that uses hrc GC schema calls this method.
            */
            static void releaseRef( ContainerNode *& hp )
            {
                atomics::store<membar_relaxed>( &hp, (ContainerNode *) NULL )   ;
            }

            /// Updates \p *ppLink node from current value \p pOld to new value \p pNew.
            /**
                May be used when concurrent updates is possible

                \p TNODE - class derived from ContainerNode type
            */
            template <typename TNODE>
            static bool    CASRef( TNODE * volatile * ppLink, TNODE * pOld, TNODE * pNew )
            {
                if ( atomics::cas<membar_acquire>( ppLink, pOld, pNew )) {
                    if ( pNew != NULL ) {
                        ++pNew->m_RC    ;
                        pNew->m_bTrace.template store<membar_release>( false ) ;
                    }
                    if ( pOld != NULL )
                        --pOld->m_RC    ;
                    return true    ;
                }
                return false    ;
            }

            //@cond
            template <typename TNODE, int BITS>
            static bool    CASRef( cds::details::marked_ptr<TNODE, BITS> volatile * ppLink,
                cds::details::marked_ptr<TNODE, BITS>& pOld,
                cds::details::marked_ptr<TNODE, BITS>& pNew )
            {
                if ( ppLink->template cas<membar_acquire>( pOld, pNew )) {
                    if ( !pNew.isNull() ) {
                        ++pNew->m_RC    ;
                        pNew->m_bTrace.template store<membar_release>( false ) ;
                    }
                    if ( !pOld.isNull() )
                        --pOld->m_RC    ;
                    return true    ;
                }
                return false    ;
            }
            //@endcond

            /// Updates \p *ppLink node from any value to new value \p pNew. Returns old value of \p *ppLink
            /**
                May be used when concurrent updates is possible

                \p TNODE - class derived from ContainerNode type
            */
            template <typename TNODE>
            static TNODE * xchgRef( TNODE * volatile * ppLink, TNODE * pNew )
            {
                TNODE * pOld = atomics::exchange<membar_acq_rel>( ppLink, pNew )  ;
                if ( pNew != NULL ) {
                    ++pNew->m_RC    ;
                    pNew->m_bTrace.template store<membar_release>( false ) ;
                }
                if ( pOld != NULL )
                    --pOld->m_RC    ;
                return pOld         ;
            }

            /// Updates \p *ppLink to new value \p pNew when no concurrent updates will be possible.
            /**
                \p TNODE - class derived from ContainerNode type
            */
            template <typename TNODE>
            static void storeRef( TNODE * volatile * ppLink, TNODE * pNew )
            {
                ContainerNode * pOld = *ppLink    ;
                atomics::store<membar_release>( ppLink, pNew )    ;
                if ( pNew != NULL ) {
                    ++pNew->m_RC    ;
                    pNew->m_bTrace.template store<membar_release>( false ) ;
                }
                if ( pOld != NULL )
                    --pOld->m_RC    ;
            }

            //@cond
            template <typename TNODE, int BITS>
            static void storeRef( cds::details::marked_ptr<TNODE, BITS> * ppLink, TNODE * pNew )
            {
                cds::details::marked_ptr<TNODE, BITS> pOld( *ppLink    ) ;
                ppLink->set( pNew, pOld.bits() )    ;
                if ( pNew != NULL ) {
                    ++pNew->m_RC    ;
                    pNew->m_bTrace.template store<membar_release>( false ) ;
                }
                if ( !pOld.isNull() )
                    --pOld->m_RC    ;
            }
            template <typename TNODE, int BITS>
            static void storeRef( cds::details::marked_ptr<TNODE, BITS> * ppLink, cds::details::marked_ptr<TNODE, BITS>& pNew )
            {
                cds::details::marked_ptr<TNODE, BITS> pOld = *ppLink    ;
                atomics::store<membar_release>( ppLink, pNew )    ;
                if ( !pNew.isNull() ) {
                    ++pNew->m_RC    ;
                    pNew->m_bTrace.template store<membar_release>( false ) ;
                }
                if ( !pOld.isNull() ) {
                    assert( pOld->m_RC != 0 )   ;
                    --pOld->m_RC    ;
                }
            }
            //@endcond

            /// Retire (deferred delete) node \p pNode guarded by \p hp hazard pointer
            void retireNode( ContainerNode * pNode, details::HPGuard& hp )
            {
                retireNode( pNode, hp.getHPRef() )  ;
            }

            /// Retire (deferred delete) node \p pNode placed in \p hp hazard pointer
            void retireNode( ContainerNode * pNode, ContainerNode *& hp )
            {
                assert( !pNode->m_bDeleted.load<membar_relaxed>() )    ;
                assert( pNode == hp )           ;

                retireNode( pNode ) ;

                releaseRef( hp )    ;
            }

            /// Retire (deferred delete) node \p pNode. Do not use this function directly!
            void retireNode( ContainerNode * pNode )
            {
                assert( !pNode->m_bDeleted.load<membar_relaxed>() )    ;

                atomics::fence<membar_acquire>()         ;
                pNode->m_bDeleted.store<membar_relaxed>( true )   ;   
                pNode->m_bTrace.store<membar_relaxed>( false )    ;
                atomics::fence<membar_release>()        ;
                m_pDesc->m_arrRetired.push( pNode )     ;

                while ( m_pDesc->m_arrRetired.isFull() ) {
                    cleanUpLocal()            ;

                    if ( m_pDesc->m_arrRetired.isFull() ) {
                        m_gc.Scan( this )        ;
                        m_gc.HelpScan( this )        ;
                    }
                    if ( m_pDesc->m_arrRetired.isFull() ) {
                        m_gc.CleanUpAll( this )        ;
                        //assert( !m_pDesc->m_arrRetired.isFull() )    ;    // infinite loop?
                    }
                    else {
                        break    ;
                    }
                }
            }

        protected:
            /// The procedure will try to remove redundant claimed references from link in deleted nodes that has been deleted by this thread
            void cleanUpLocal()
            {
                details::retired_vector::iterator itEnd = m_pDesc->m_arrRetired.end()    ;
                for ( details::retired_vector::iterator it = m_pDesc->m_arrRetired.begin(); it != itEnd; ++it ) {
                    details::retired_node& node = *it ;
                    ContainerNode * pNode = node.m_pNode.load<membar_acquire>() ;
                    if ( pNode != NULL && !node.m_bDone.load<membar_acquire>() )
                        pNode->cleanUp( this )    ;
                }
            }
        };

        /// Auto HPGuard.
        class AutoHPGuard: public details::HPGuard
        {
            ThreadGC&    m_mgr    ;    ///< Thread GC.
        public:
            /// Allocates HP guard from \p mgr
            AutoHPGuard( ThreadGC& mgr )
                : m_mgr( mgr )
            {
                mgr.allocGuard( *this )    ;
            }

            /// Allocates HP guard from \p mgr and protects the pointer \p p of type \p T
            template <typename T>
            AutoHPGuard( ThreadGC& mgr, T * p  )
                : m_mgr( mgr )
            {
                mgr.allocGuard( *this )    ;
                *this = p               ;
            }

            /// Frees HP guard
            ~AutoHPGuard()
            {
                m_mgr.freeGuard( *this )    ;
            }

            /// Returns thread GC
            ThreadGC&    getGC() const { return m_mgr; }

            //@cond
            template <typename T>
            T * operator =( T * p )
            {
                return details::HPGuard::operator =( p )    ;
            }
            //@endcond
        };

        /// Auto-managed array of hazard pointers
        /**
            This class is wrapper around gc::hzp::details::HPArray class.
        */
        template <size_t COUNT>
        class AutoHPArray: public details::HPArray<COUNT>
        {
            ThreadGC&    m_mgr    ;    ///< Thread GC

        public:
            /// Allocates array of HP guard from \p mgr
            AutoHPArray( ThreadGC& mgr )
                : m_mgr( mgr )
            {
                mgr.allocGuard( *this )    ;
            }

            /// Frees array of HP guard
            ~AutoHPArray()
            {
                m_mgr.freeGuard( *this )    ;
            }

            /// Returns thread GC
            ThreadGC&    getGC() const { return m_mgr; }
        };


    }    // namespace hrc
}} // namespace cds::gc

#include <cds/gc/hrc/details/hrc_inline.h>

#endif // #ifndef __CDS_GC_HRC_SCHEMA_H
