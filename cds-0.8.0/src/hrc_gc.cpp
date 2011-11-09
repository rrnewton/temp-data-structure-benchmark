/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


/*
    File: hrc_gc.cpp

    Implementation of cds::gc::hrc::HRCGarbageCollector

    Editions:
        2008.03.10    Maxim.Khiszinsky    Created
*/

#include <cds/gc/hrc/hrc.h>

#include "hzp_const.h"
#include <vector>
#include <algorithm>    // std::sort

#define    CDS_HRC_STATISTIC( _x )    if ( m_bStatEnabled ) { _x; }

namespace cds { namespace gc {
    namespace hrc {

        GarbageCollector * GarbageCollector::m_pGC = NULL ;

        GarbageCollector::GarbageCollector(
            size_t nHazardPtrCount,
            size_t nMaxThreadCount,
            size_t nRetiredNodeArraySize
            )
            : m_pListHead(NULL),
            m_bStatEnabled( true ),
            m_nHazardPointerCount( nHazardPtrCount ),
            m_nMaxThreadCount( nMaxThreadCount ),
            m_nMaxRetiredPtrCount( nRetiredNodeArraySize )
        {}

        GarbageCollector::~GarbageCollector()
        {
            thread_list_node * pNode = atomics::load<membar_relaxed>( &m_pListHead ) ;
            while ( pNode ) {
                assert( pNode->m_idOwner.load<membar_relaxed>() == cds::OS::nullThreadId() ) ;
                clearHRCThreadDesc( pNode )    ;
                thread_list_node * pNext = pNode->m_pNext    ;
                deleteHRCThreadDesc( pNode )    ;
                pNode = pNext    ;
            }
        }

        void CDS_STDCALL GarbageCollector::Construct(
            size_t nHazardPtrCount,        // hazard pointers count
            size_t nMaxThreadCount,        // max thread count
            size_t nMaxNodeLinkCount,    // max HRC-pointer count in the HRC-container's item
            size_t nMaxTransientLinks    // max HRC-container's item count that can point to deleting item of container
            )
        {
            if ( !m_pGC ) {
                if ( nHazardPtrCount == 0 )
                    nHazardPtrCount = c_nHazardPointerPerThread    + c_nCleanUpHazardPointerPerThread ;
                if ( nMaxThreadCount == 0 )
                    nMaxThreadCount = c_nMaxThreadCount    ;
                if ( nMaxNodeLinkCount == 0 )
                    nMaxNodeLinkCount = c_nHRCMaxNodeLinkCount    ;
                if ( nMaxTransientLinks == 0 )
                    nMaxTransientLinks = c_nHRCMaxTransientLinks    ;

                size_t nRetiredNodeArraySize = nMaxThreadCount * ( nHazardPtrCount + nMaxNodeLinkCount + nMaxTransientLinks + 1 )    ;

                m_pGC = new GarbageCollector( nHazardPtrCount, nMaxThreadCount, nRetiredNodeArraySize ) ;
            }
        }

        void CDS_STDCALL GarbageCollector::Destruct()
        {
            if ( m_pGC ) {
                {
                    ThreadGC tgc    ;
                    tgc.init()      ;
                    m_pGC->HelpScan( &tgc ) ;
                    tgc.fini()      ;
                }

                delete m_pGC    ;
                m_pGC = NULL    ;
            }
        }

        inline GarbageCollector::thread_list_node * GarbageCollector::newHRCThreadDesc()
        {
            CDS_HRC_STATISTIC( ++m_Stat.m_AllocNewHRCThreadDesc )    ;
            return new thread_list_node( *this ) ;
        }

        inline void GarbageCollector::deleteHRCThreadDesc( thread_list_node * pNode )
        {
            assert( pNode->m_hzp.size() == pNode->m_hzp.capacity() )  ;
            CDS_HRC_STATISTIC( ++m_Stat.m_DeleteHRCThreadDesc )    ;
            delete pNode    ;
        }

        void GarbageCollector::clearHRCThreadDesc( thread_list_node * pNode )
        {
            assert( pNode->m_hzp.size() == pNode->m_hzp.capacity() )  ;
            ContainerNode * pItem   ;
            for ( size_t n = 0; n < pNode->m_arrRetired.capacity(); ++n ) {
                if ( (pItem = pNode->m_arrRetired[n].m_pNode.load<membar_relaxed>()) != NULL ) {
                    pItem->destroy()    ;
                    pNode->m_arrRetired[n].m_pNode.store<membar_relaxed>( NULL ) ;
                }
            }
            assert( pNode->m_hzp.size() == pNode->m_hzp.capacity() )  ;
        }

        GarbageCollector::thread_list_node *  GarbageCollector::getHRCThreadDescForCurrentThread() const
        {
            thread_list_node * hprec   ;
            const cds::OS::ThreadId curThreadId  = cds::OS::getCurrentThreadId()    ;

            for ( hprec = atomics::load<membar_acquire>( &m_pListHead ); hprec; hprec = hprec->m_pNext ) {
                if ( hprec->m_idOwner.load<membar_acquire>() == curThreadId ) {
                    assert( !hprec->m_bFree )    ;
                    return hprec    ;
                }
            }
            return NULL ;
        }

        details::thread_descriptor * GarbageCollector::allocateHRCThreadDesc( ThreadGC * pThreadGC )
        {
            CDS_HRC_STATISTIC( ++m_Stat.m_AllocHRCThreadDesc )    ;

            thread_list_node * hprec   ;
            const cds::OS::ThreadId nullThreadId = cds::OS::nullThreadId() ;
            const cds::OS::ThreadId curThreadId  = cds::OS::getCurrentThreadId()    ;

            // First try to reuse a retired (non-active) HP record
            for ( hprec = atomics::load<membar_acquire>( &m_pListHead ); hprec; hprec = hprec->m_pNext ) {
                if ( !hprec->m_idOwner.cas<membar_acq_rel>( nullThreadId, curThreadId ))
                    continue    ;
                hprec->m_pOwner = pThreadGC        ;
                hprec->m_bFree = false            ;
                assert( hprec->m_hzp.size() == hprec->m_hzp.capacity() )  ;
                return hprec    ;
            }

            // No HP records available for reuse
            // Allocate and push a new HP record
            hprec = newHRCThreadDesc()  ;
            assert( hprec->m_hzp.size() == hprec->m_hzp.capacity() )  ;
            hprec->m_idOwner.store<membar_relaxed>( curThreadId )    ;
            hprec->m_pOwner = pThreadGC        ;
            hprec->m_bFree = false            ;
            thread_list_node * pOldHead        ;
            do {
                pOldHead = atomics::load<membar_acquire>( &m_pListHead ) ;
                hprec->m_pNext = pOldHead   ;
            } while ( !atomics::cas<membar_release>( &m_pListHead, pOldHead, hprec )) ;

            assert( hprec->m_hzp.size() == hprec->m_hzp.capacity() )  ;
            return hprec ;
        }

        void GarbageCollector::retireHRCThreadDesc( details::thread_descriptor * pRec )
        {
            CDS_HRC_STATISTIC( ++m_Stat.m_RetireHRCThreadDesc )    ;

            pRec->clear()   ;
            thread_list_node * pNode = static_cast<thread_list_node *>( pRec )  ;
            assert( pNode->m_hzp.size() == pNode->m_hzp.capacity() )  ;
            /*
                It is possible that
                    pNode->m_idOwner.value() != cds::OS::getCurrentThreadId()
                if the destruction of thread object is called by the destructor
                after thread termination
            */
            assert( pNode->m_idOwner.load<membar_relaxed>() != cds::OS::nullThreadId() )  ;
            pNode->m_pOwner = NULL    ;
            pNode->m_idOwner.store<membar_release>( cds::OS::nullThreadId() ) ;
            assert( pNode->m_hzp.size() == pNode->m_hzp.capacity() )  ;
        }

        void GarbageCollector::Scan( ThreadGC * pThreadGC )
        {
            CDS_HRC_STATISTIC( ++m_Stat.m_ScanCalls )    ;

            typedef std::vector< ContainerNode * > PListType    ;

            details::thread_descriptor * pRec = pThreadGC->m_pDesc    ;
            assert( static_cast< thread_list_node *>( pRec )->m_idOwner.load<membar_relaxed>() == cds::OS::getCurrentThreadId() )    ;

            // Step 1: mark all pRec->m_arrRetired items as "traced"
            {
                details::retired_vector::const_iterator itEnd = pRec->m_arrRetired.end() ;

                for ( details::retired_vector::const_iterator it = pRec->m_arrRetired.begin() ; it != itEnd; ++it ) {
                    ContainerNode * pNode = it->m_pNode.load<membar_acquire>()  ;
                    if ( pNode ) {
                        if ( pNode->m_RC.value<membar_acquire>() == 0 ) {
                            if ( pNode->m_bTrace.cas<membar_release>( false, true )) {
                                if ( pNode->m_RC.value<membar_acquire>() != 0 )
                                    pNode->m_bTrace.store<membar_release>( false ) ;
                            }
                        }
                    }
                }
            }

            // Array of hazard pointers for all threads
            PListType   plist    ;
            plist.reserve( m_nMaxThreadCount * m_nHazardPointerCount )  ;
            assert( plist.size() == 0 ) ;

            // Stage 2: Scan HP list and insert non-null values to plist
            {
                thread_list_node * pNode = atomics::load<membar_acquire>( &m_pListHead ) ;

                while ( pNode ) {
                    for ( size_t i = 0; i < m_nHazardPointerCount; ++i ) {
                        ContainerNode * hptr = pNode->m_hzp[i]    ;
                        if ( hptr )
                            plist.push_back( hptr )        ;
                    }
                    pNode = pNode->m_pNext  ;
                }
            }

            // Sort plist to simplify search in
            std::sort( plist.begin(), plist.end() ) ;

            // Stage 3: Deletes all nodes for refCount == 0 and that do not declare as Hazard in other thread
            {
                details::retired_vector& arr =  pRec->m_arrRetired    ;

                PListType::iterator itHPBegin = plist.begin()    ;
                PListType::iterator itHPEnd = plist.end()    ;

                details::retired_vector::iterator itEnd = arr.end()  ;
                details::retired_vector::iterator it = arr.begin()   ;

                for ( size_t nRetired = 0; it != itEnd; ++nRetired, ++it ) {
                    details::retired_node& node = *it        ;
                    ContainerNode * pNode = node.m_pNode.load<membar_relaxed>()    ;
                    if ( pNode == NULL )
                        continue    ;

                    if ( pNode->m_RC.value<membar_acquire>() == 0 && pNode->m_bTrace.load<membar_acquire>() && !std::binary_search( itHPBegin, itHPEnd, pNode ) ) {
                        // pNode may be destructed safely
                        node.m_pNode.store<membar_relaxed>( NULL )  ;

                        if ( node.m_bDone.cas<membar_acquire>( false, true ) ) {
                            if ( node.m_nClaim.value<membar_acquire>() == 0 ) {
                                pNode->terminate( pThreadGC, false )    ;

                                pNode->destroy()        ;
                                arr.pop( nRetired )        ;
                                CDS_HRC_STATISTIC( ++m_Stat.m_DeletedNode ) ;
                                continue;
                            }
                            else
                                node.m_bDone.store<membar_release>( false ) ;
                        }
                        /*
                        if ( node.m_bDone.cas<membar_acquire>( false, true ) ) {
                            pNode->terminate( pThreadGC, true )         ;
                        }
                        */
                        pNode->m_bTrace.store<membar_relaxed>( false )    ;
                        node.m_pNode.store<membar_release>( pNode )       ;    // push back
                        CDS_HRC_STATISTIC( ++m_Stat.m_ScanClaimGuarded )  ;
                    }
                    else {
                        pNode->m_bTrace.store<membar_release>( false ) ;
                        CDS_HRC_STATISTIC( ++m_Stat.m_ScanGuarded ) ;
                    }
                }
            }
        }

        void GarbageCollector::HelpScan( ThreadGC * pThis )
        {
            if ( pThis->m_pDesc->m_arrRetired.isFull() )
                return ;

            CDS_HRC_STATISTIC( ++m_Stat.m_HelpScanCalls )    ;

            const cds::OS::ThreadId nullThreadId = cds::OS::nullThreadId() ;
            const cds::OS::ThreadId curThreadId  = cds::OS::getCurrentThreadId()    ;

            for ( thread_list_node * pRec = atomics::load<membar_acquire>( &m_pListHead ); pRec; pRec = pRec->m_pNext ) {

                // If threadDesc is free then own its
                if ( pRec->m_idOwner.load<membar_acquire>() != nullThreadId
                    || !pRec->m_idOwner.cas<membar_release>( nullThreadId, curThreadId ))
                {
                    continue    ;
                }

                // We own threadDesc.
                assert( pRec->m_pOwner == NULL )    ;

                if ( !pRec->m_bFree ) {

                    // All undeleted pointers is moved to pThis (it is private fot the current thread)

                    details::retired_vector& src = pRec->m_arrRetired    ;
                    details::retired_vector& dest = pThis->m_pDesc->m_arrRetired   ;
                    assert( !dest.isFull())    ;

                    details::retired_vector::iterator itEnd = src.end()  ;
                    details::retired_vector::iterator it = src.begin()   ;

                    for ( size_t nRetired = 0; it != itEnd; ++nRetired, ++it ) {
                        if ( it->m_pNode.load<membar_relaxed>() == NULL )
                            continue    ;

                        dest.push( it->m_pNode.load<membar_relaxed>() )    ;
                        src.pop( nRetired ) ;

                        while ( dest.isFull() ) {
                            pThis->cleanUpLocal()   ;
                            if ( dest.isFull() )
                                Scan( pThis )       ;
                            if ( dest.isFull() )
                                CleanUpAll( pThis ) ;
                            else
                                break    ;
                        }
                    }
                    pRec->m_bFree = true    ;
                }
                pRec->m_idOwner.store<membar_release>( nullThreadId ) ;
            }
        }

        void GarbageCollector::CleanUpAll( ThreadGC * pThis )
        {
            CDS_HRC_STATISTIC( ++m_Stat.m_CleanUpAllCalls )    ;

            //const cds::OS::ThreadId nullThreadId = cds::OS::nullThreadId() ;
            thread_list_node * pThread = atomics::load<membar_acquire>( &m_pListHead ) ;
            while ( pThread ) {
                for ( size_t i = 0; i < pThread->m_arrRetired.capacity(); ++i ) {
                    details::retired_node& rRetiredNode = pThread->m_arrRetired[i]    ;
                    ContainerNode * pNode = rRetiredNode.m_pNode.load<membar_relaxed>()     ;
                    if ( pNode != NULL && !rRetiredNode.m_bDone.load<membar_relaxed>() ) {
                        ++rRetiredNode.m_nClaim    ;
                        if ( !rRetiredNode.m_bDone.load<membar_relaxed>() && pNode == rRetiredNode.m_pNode.load<membar_relaxed>() )
                            pNode->cleanUp( pThis )    ;
                        --rRetiredNode.m_nClaim    ;
                    }
                }
                pThread = pThread->m_pNext  ;
            }
        }

        GarbageCollector::internal_state& GarbageCollector::getInternalState( GarbageCollector::internal_state& stat) const
        {
            // Const
            stat.nHPCount               = m_nHazardPointerCount    ;
            stat.nMaxThreadCount        = m_nMaxThreadCount        ;
            stat.nMaxRetiredPtrCount    = m_nMaxRetiredPtrCount    ;
            stat.nHRCRecSize            = sizeof( thread_list_node )
                                            + sizeof( details::retired_node) * m_nMaxRetiredPtrCount ;
            stat.nHRCRecAllocated            =
                stat.nHRCRecUsed             =
                stat.nTotalRetiredPtrCount   =
                stat.nRetiredPtrInFreeHRCRecs = 0 ;

            // Walk through HRC records
            for ( thread_list_node *hprec = atomics::load<membar_acquire>( &m_pListHead ); hprec; hprec = hprec->m_pNext ) {
                ++stat.nHRCRecAllocated ;
                size_t nRetiredNodeCount = hprec->m_arrRetired.getRetiredNodeCount()    ;
                if ( hprec->m_bFree ) {
                    stat.nRetiredPtrInFreeHRCRecs += nRetiredNodeCount  ;
                }
                else {
                    ++stat.nHRCRecUsed  ;
                }
                stat.nTotalRetiredPtrCount += nRetiredNodeCount  ;
            }

            // Events
            stat.evcAllocHRCRec            = m_Stat.m_AllocHRCThreadDesc   ;
            stat.evcRetireHRCRec        = m_Stat.m_RetireHRCThreadDesc  ;
            stat.evcAllocNewHRCRec        = m_Stat.m_AllocNewHRCThreadDesc;
            stat.evcDeleteHRCRec        = m_Stat.m_DeleteHRCThreadDesc  ;
            stat.evcScanCall            = m_Stat.m_ScanCalls            ;
            stat.evcHelpScanCalls       = m_Stat.m_HelpScanCalls        ;
            stat.evcCleanUpAllCalls     = m_Stat.m_CleanUpAllCalls      ;
            stat.evcDeletedNode         = m_Stat.m_DeletedNode          ;
            stat.evcScanGuarded         = m_Stat.m_ScanGuarded          ;
            stat.evcScanClaimGuarded    = m_Stat.m_ScanClaimGuarded     ;

#       ifdef CDS_DEBUG
            stat.evcNodeConstruct       = m_Stat.m_NodeConstructed      ;
            stat.evcNodeDestruct        = m_Stat.m_NodeDestructed       ;
#       endif

            return stat ;
        }

        void ContainerNode::cleanUp( ThreadGC * /*pGC*/ )
        {
            CDS_PURE_VIRTUAL_FUNCTION_CALLED  ;
        }
        void ContainerNode::terminate( ThreadGC * /*pGC*/, bool /*bConcurrent*/ )
        {
            CDS_PURE_VIRTUAL_FUNCTION_CALLED  ;
        }
        void ContainerNode::destroy()
        {
            CDS_PURE_VIRTUAL_FUNCTION_CALLED  ;
        }

    }    // namespace hrc
} /* namespace gc */ } /* namespace cds */
