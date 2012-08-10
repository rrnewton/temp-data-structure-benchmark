/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0+
*/


#ifndef __CDS_BAG_SBAG_HRC_H
#define __CDS_BAG_SBAG_HRC_H

/*
    Editions:
        2011.10.20    Guilherme Fernandes [stallone3 at gmail dot com]    Created
*/

#include <cds/bag/sbag.h>
#include <cds/gc/hrc/container.h>
#include <cds/user_setup/cache_line.h>

namespace cds {

// XXX(fernandes): could be wrong.
#ifndef CDS_WORD_SIZE
    static const size_t c_nWordSizeBits = sizeof(void *) * 8 ;
#else
    static const size_t c_nWordSizeBits = CDS_WORD_SIZE * 8 ;
#endif

#ifndef CDS_SBAG_BLOCKSIZE_MULT
#    define CDS_SBAG_BLOCKSIZE_MULT 1
#endif

    namespace bag {

        /// Sundell et. al. lock-free concurrent bag implementation based on Gidenstam's memory reclamation schema (HRC)
        /**
            \par Source:
                \li [2011] H.Sundell, et. al. "A Lock-Free Algorithm for Concurrent Bags",
                        SPAA’11, June 4–6, 2011, San Jose, California, USA.
                \li [2006] A.Gidenstam "Algorithms for synchronization and consistency
                        in concurrent system services", Chapter 5 "Lock-Free Memory Reclamation"
                        Thesis for the degree of Doctor    of Philosophy

            \par Template parameters:
                \li \p T        Type of data saved in queue's node
                \li \p TRAITS    Traits class (see HRCQueueTraits)
                \li \p ALLOCATOR Memory allocator template. This implementation assumes that memory allocator is a wrapper of global
                object (singleton)

                This algorithm is the integration of Michael's hazard pointer scheme and reference counting method. Hazard pointers
                are used for guaranteeing the safety of local references and reference counts for guaranteeing the safety
                of internal links in the data structure. The reference count of each node should indicate the number of
                globally accessible links that reference that node. The reference count mechanism allows to introduce
                concurrent iterator concept into queue implementation.
        */
        template < typename T, int NR_THREADS, class ALLOCATOR >
        class SBag< gc::hrc_gc, T, NR_THREADS, ALLOCATOR >: public gc::hrc::Container
        {
            typedef gc::hrc::Container        base_class    ;    ///< Base type

        protected:
            typedef cds::gc::hrc::ThreadGC        TThreadGC    ;    ///< Memory allocation driver

            static const size_t c_nBlockSize = ( c_nCacheLineSize / sizeof(T) ) * CDS_SBAG_BLOCKSIZE_MULT;
            //static const size_t c_nBlockSize = c_nCacheLineSize / sizeof(T);
            static const size_t c_nThreadBitVectorSize = (NR_THREADS / c_nWordSizeBits) + 1;


            /// SBag block
            struct Block : public gc::hrc::ContainerNodeT<Block, ALLOCATOR>
            {
                typedef gc::hrc::ContainerNodeT<Block, ALLOCATOR>    base_class            ;    ///< Base type
                typedef typename base_class::node_allocator            TNodeAllocator    ;    ///< Node allocator type

                typedef typename cds::details::marked_ptr< Block, 3 >  marked_block    ;    ///< Marked node type

                marked_block m_pNext    ;    ///< pointer to next node
                // XXX(fernandes): T *? What about sizeof(T) > cache line or not a multiple.
                T m_data [c_nBlockSize]    ;    ///< block's data, cacheline sized
                long m_notifyAdd [c_nThreadBitVectorSize];

                /// Default ctor
                Block()
                {}

                /// Ctor to assign \p data to the node
                Block( const T& data )
                    : m_data( data )
                {}

            private:
                /// Implementation of @ref gc::hrc::ContainerNode::cleanUp.
                virtual void    cleanUp( TThreadGC * pGC )
                {
                    // XXX(fernandes): I think this is taken care by the algorithm.
                }

                /// Implementation of @ref gc::hrc::ContainerNode::terminate
                virtual void    terminate( TThreadGC * pGC, bool bConcurrent )
                {
                    // XXX(fernandes): I think this is taken care by the algorithm.
                }
            };

            typedef typename cds::details::marked_ptr< Block, 3 >  marked_block;

        public:
            typedef gc::hrc_gc                        gc_schema    ;    ///< Garbage collection schema
            typedef T                                value_type    ;    ///< Type of value saved in bag

            // FIXME(fernandes): random number until I figure it out.
            static const size_t        m_nHazardPointerCount = NR_THREADS + 5   ; ///< Maximum hazard pointer count required for bag algorithm

        protected:
            Block * volatile     m_pGlobalHeadBlock [NR_THREADS]   ;        ///< Initial bag head pointers per thread
            const value_type              SENTINEL;      ///< Sentinel used to signal empty node

            // XXX(fernandes): Is there a better way to have Thread Local data in this library?
            struct TLSData
            {
                Block * m_pThreadBlock;
                int m_nThreadHead;
                int m_nThreadId;
                Block * m_pStealBlock;
                Block * m_pStealPrev;
                int m_nStealHead;
                int m_nStealIndex;
                bool m_bFoundAdd;

                //@cond
                TLSData(int threadId, Block * threadBlock)
                   : m_pThreadBlock ( threadBlock ), m_nThreadHead ( c_nBlockSize ),
                     m_nThreadId ( threadId ), m_pStealBlock ( NULL ), m_pStealPrev ( NULL ),
                     m_nStealHead ( c_nBlockSize ), m_nStealIndex ( 0 )
                {}

                ~TLSData()
                {}
            };

        protected:
            /// Allocates a node
            Block *        allocNode()
            {
                typename Block::TNodeAllocator a    ;
                return a.New()    ;
            }

        private:
            static TLSData * _threadData()
            {
                typedef unsigned char  ThreadDataPlaceholder[ sizeof(TLSData) ]  ;
                static __thread ThreadDataPlaceholder CDS_DATA_ALIGNMENT(8) threadData        ;

                return reinterpret_cast<TLSData *>(threadData)   ;
            }

        public:
            /// Constructs empty bag
            SBag(T sentinel)
                : SENTINEL ( sentinel )
            {
                assert( m_nHazardPointerCount <= gc::hrc::GarbageCollector::instance().getHazardPointerCount() )    ;

                for (int i = 0; i < NR_THREADS; i++)
                    m_pGlobalHeadBlock[i] = NULL;
#ifndef CDS_THREADING_GCC
                throw cds::Exception( "Only CDS_THREADING_GCC model supported." );
#endif
            }

            /// Clears bag and destruct
            ~SBag()
            {
                // TODO(fernandes): clear bag.
            }

            /// Must be called by each thread before the thread performs operations on this bag.
            void initThread( int threadId )
            {
                new ( _threadData() ) TLSData (threadId, m_pGlobalHeadBlock[threadId])   ;
                assert ( _threadData() != NULL );
            }

            /// Must be called by each thread after thread finishes performing operations on this bag.
            void finiThread()
            {
                _threadData()->TLSData::~TLSData()  ;
            }


            // Begin methods adapted from paper.
            //////////////////////////////////////

            void add(value_type& item) {
                TLSData * m_tlsData = _threadData();
                int head = m_tlsData->m_nThreadHead;
                Block * block = m_tlsData->m_pThreadBlock;

                gc::hrc::AutoHPArray<1> hpArr( base_class::getGC() )  ;
                for (;;) {
                    if (head == c_nBlockSize) {
                        Block *oldblock = block;
                        block = NewBlock();
                        hpArr.set( 0, block ) ;
                        hpArr.getGC().storeRef(&block->m_pNext, oldblock);
                        hpArr.getGC().storeRef(&m_tlsData->m_pThreadBlock, block);
                        // FIXME(fernandes) For now this should be fine, gotta double check if concurrency is possible.
                        hpArr.getGC().storeRef(&m_pGlobalHeadBlock[m_tlsData->m_nThreadId], block);
                        head = 0;
                    } else if (block->m_data[head] == SENTINEL) {
                        NotifyAll(block);
                        block->m_data[head] = item;
                        m_tlsData->m_nThreadHead = head + 1;
                        return;
                    } else
                        head++;
                }
            }


            bool tryRemoveAny(value_type& result) {
                TLSData * m_tlsData = _threadData();
                int head = m_tlsData->m_nThreadHead - 1;
                int round = 0;
                Block * block = m_tlsData->m_pThreadBlock;

                gc::hrc::AutoHPArray<2> hpArr( base_class::getGC() )  ;
                for (;;) {
                    if (block == NULL || (head < 0 && block->m_pNext.isNull())) {
                        do {
                            int i = 0;
                            do {
                                if ( TryStealBlock(round, result, m_tlsData) )
                                  return true;

                                if (m_tlsData->m_bFoundAdd) {
                                    round = 0;
                                    i = 0;
                                } else if (m_tlsData->m_pStealBlock == NULL)
                                  i++;
                            } while (i < NR_THREADS);
                        } while (++round <= NR_THREADS);

                        return false;
                    }

                    //For now we know this shouldn't happen in single producer-consumer.
                    assert (false);

                    if (head < 0) { // block != NULL and block->m_pNext != NULL
                        Mark1Block(block);

                        for (;;) {
                            marked_block next = hpArr.getGC().derefLink( &block->m_pNext, hpArr[1] );

                            if (next.isMarked(2))
                                Mark1Block((Block*)next.ptr());

                            if (next.isMarked(1)) {
                                if (!next.isNull())
                                    NotifyAll((Block*)next.ptr());

                                // XXX(fernandes): not sure why this would need to be
                                if (hpArr.getGC().CASRef(&m_pGlobalHeadBlock[m_tlsData->m_nThreadId], block, (Block*)next.ptr())) {
                                    block->m_pNext = (Block *) 1; // Sets mark 1 and NULL pointer
                                    DeleteNode(block);
                                    ReScan(next);
                                    block = next.ptr();
                                } else {
                                    block = hpArr.getGC().derefLink( &m_pGlobalHeadBlock[m_tlsData->m_nThreadId], hpArr[0] );
                                }
                            } else
                                break;
                        }

                        m_tlsData->m_pThreadBlock = block;
                        m_tlsData->m_nThreadHead = c_nBlockSize;
                        head = c_nBlockSize - 1;

                    } else {
                        result = block->m_data[head];

                        if (result == SENTINEL)
                            head--;
                        // FIXME(fernandes): for single producer-consumer it's ok not to cas this,
                        //   and it allows us to support pass by value data (i.e. not pointers).
                        //else if (atomics::cas<membar_release>(&block->m_data[head], data, SENTINEL)) {
                        else {
                            block->m_data[head] = SENTINEL;
                            m_tlsData->m_nThreadHead = head;
                            return true;
                        }
                    }
                }
                // NOT REACHABLE (make compiler happy)
                return false;
            }


        private:
            // Start methods adapted from paper.
            void NotifyAll(Block *block) {
                for (int i = 0; i < c_nThreadBitVectorSize; i++)
                    block->m_notifyAdd[i] = 0;
            }

            void NotifyStart(Block *block, int Id) {
                long old;
                do {
                    old = block->m_notifyAdd[Id/c_nWordSizeBits];
                } while(!atomics::cas<membar_release>(
                    &block->m_notifyAdd[Id/c_nWordSizeBits], old, old | (1<<(Id%c_nWordSizeBits))));
            }

            bool NotifyCheck(Block *block, int Id) {
                return (block->m_notifyAdd[Id / c_nWordSizeBits] & (1 << (Id % c_nWordSizeBits))) == 0;
            }

            Block * NewBlock() {
                Block * block = allocNode();
                block->m_pNext = NULL;
                NotifyAll(block);

                for (int i = 0; i < c_nBlockSize; i++)
                    block->m_data[i] = SENTINEL;
                return block;
            }

            void Mark1Block(Block *block) {
                for (;;) {
                    marked_block next = block->m_pNext;
                    if (next.isNull() || next.isMarked(1) ||
                            block->m_pNext.template cas<membar_release>(next, marked_block( next.ptr(), (1 | (int(next.isMarked(2)) << 1)))))
                        break;
                }
            }

            // XXX(fernandes): Could be wrong.
            void DeleteNode ( Block *block ) {
#ifndef CDS_SBAG_DISABLE_GC
                getGC().retireNode( block )    ;
#endif
            }

            void ReScan ( marked_block next ) {
                // TODO(fernandes): not sure what?
            }

            bool TryStealBlock(int round, value_type& result, TLSData * m_tlsData) {
                int head = m_tlsData->m_nStealHead;
                m_tlsData->m_bFoundAdd = false;

                // Includes the HPs needed by NextStealBlock (1).
                gc::hrc::AutoHPArray<2> hpArr( base_class::getGC() )    ;
                Block * block = hpArr.getGC().derefLink(&m_tlsData->m_pStealBlock, hpArr[0]);

                if (block == NULL) {
                    block = hpArr.getGC().derefLink(&m_pGlobalHeadBlock[m_tlsData->m_nStealIndex], hpArr[0]);
                    hpArr.getGC().storeRef(&m_tlsData->m_pStealBlock, block);
                    m_tlsData->m_nStealHead = head = 0;
                }

                if (head == c_nBlockSize) {
                    block = NextStealBlock(block, m_tlsData, hpArr);
                    hpArr.getGC().storeRef(&m_tlsData->m_pStealBlock, block);
                    head = 0;
                }

                if (block == NULL) {
                    m_tlsData->m_nStealIndex = (m_tlsData->m_nStealIndex + 1) % NR_THREADS;
                    m_tlsData->m_nStealHead = 0;
                    hpArr.getGC().storeRef(&m_tlsData->m_pStealBlock, (Block*)NULL);
                    hpArr.getGC().storeRef(&m_tlsData->m_pStealPrev, (Block*)NULL);
                    return false;
                }

                if (round == 1)
                    NotifyStart(block, m_tlsData->m_nThreadId);
                else if (round > 1 && NotifyCheck(block, m_tlsData->m_nThreadId))
                    m_tlsData->m_bFoundAdd = true;

                for (;;) {
                  if (head == c_nBlockSize) {
                      m_tlsData->m_nStealHead = head;
                      return false;
                  } else {
                      result = block->m_data[head];

                      if (result == SENTINEL)
                          head++;
                      // FIXME(fernandes): for single producer-consumer it's ok not to cas this,
                      //   and it allows us to support pass by value data (i.e. not pointers).
                      //else if (atomics::cas<membar_release>(&block->m_data[head], data, SENTINEL)) {
                      else {
                          block->m_data[head] = SENTINEL;
                          m_tlsData->m_nStealHead = head;
                          return true;
                      }
                  }
                }
                // NOT REACHABLE (make compiler happy)
                return false;
            }

            Block * NextStealBlock(Block *block, TLSData * m_tlsData, gc::hrc::AutoHPArray<2> &hpArr) {
                marked_block next;

                for (;;) {
                    if (block == NULL) {
                        block = hpArr.getGC().derefLink(&m_pGlobalHeadBlock[m_tlsData->m_nStealIndex], hpArr[0]);
                        break;
                    }

                    next = hpArr.getGC().derefLink(&block->m_pNext, hpArr[1]);

                    if (next.isMarked(2))
                        Mark1Block((Block*)next.ptr());

                    if (m_tlsData->m_pStealPrev == NULL || next.isNull()) {
                        if (next.isMarked(1)) {
                            if (!next.isNull())
                                NotifyAll((Block*)next.ptr());

                            //For now we know this shouldn't happen in single producer-consumer.
                            assert (false);

                            if (hpArr.getGC().CASRef(&m_pGlobalHeadBlock[m_tlsData->m_nStealIndex], block, (Block*)next.ptr())) {
                                // Set NULL pointer and mark 1
                                hpArr.getGC().storeRef(&block->m_pNext, (Block *) NULL);
                                block->m_pNext.mark(1);
                                DeleteNode(block);
                                ReScan(next);
                            }
                            else {
                                hpArr.getGC().storeRef(&m_tlsData->m_pStealPrev, (Block*)NULL);
                                block = hpArr.getGC().derefLink(&m_pGlobalHeadBlock[m_tlsData->m_nStealIndex], hpArr[0]);
                                continue;
                            }
                        }
                        else
                            hpArr.getGC().storeRef(&m_tlsData->m_pStealPrev, block);
                    }
                    else {
                        if (next.isMarked(1)) {
                            marked_block prevnext ( block , int(m_tlsData->m_pStealPrev->m_pNext.isMarked(2)) << 1) ;
                            // XXX(fernandes): bug in paper? not clear what the mask is.
                            marked_block nextptr ( next.ptr(), int(next.isMarked(2)) << 1);
                            if ( hpArr.getGC().CASRef(&m_tlsData->m_pStealPrev->m_pNext, prevnext, nextptr) ) {
                                // Set NULL pointer and mark 1
                                hpArr.getGC().storeRef(&block->m_pNext, (Block *) NULL);
                                block->m_pNext.mark(1);
                                DeleteNode(block);
                                ReScan(next);
                            }
                            else {
                                hpArr.getGC().storeRef(&m_tlsData->m_pStealPrev, (Block*)NULL);
                                block = hpArr.getGC().derefLink(&m_pGlobalHeadBlock[m_tlsData->m_nStealIndex], hpArr[0]);
                                continue;
                            }
                        }
                        else if(block == m_tlsData->m_pStealBlock) {
                          // XXX(fernandes): bug in paper? not clear what the mask is.
                            if(m_tlsData->m_pStealPrev->m_pNext.template cas<membar_release>(marked_block( block, 0 ), marked_block( block, 2 ))) {
                                Mark1Block(block);
                                continue;
                            }
                            else {
                                hpArr.getGC().storeRef(&m_tlsData->m_pStealPrev, (Block*)NULL);
                                block = hpArr.getGC().derefLink(&m_pGlobalHeadBlock[m_tlsData->m_nStealIndex], hpArr[0]);
                                continue;
                            }
                        }
                        else
                            hpArr.getGC().storeRef(&m_tlsData->m_pStealPrev, block);
                    }

                    if (block == m_tlsData->m_pStealBlock || next.ptr() == m_tlsData->m_pStealBlock) {
                        block = next.ptr();
                        hpArr.set(0, block);
                        break;
                    }

                    block = next.ptr();
                    hpArr.set(0, block);
                }

                return block;
            }
        };


    } // namespace bag
} // namespace cds

#endif    // #ifndef __CDS_BAG_SBAG_HRC_H
