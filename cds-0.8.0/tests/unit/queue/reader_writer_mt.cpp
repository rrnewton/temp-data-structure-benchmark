/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include "cppunit/thread.h"
#include "queue/queue_type.h"
#include <vector>
#include <algorithm>

// Multi-threaded random queue test
namespace queue {

#define TEST_CASE( Q, V ) void Q() { test< Types<V>::Q >(); }

    namespace {
        static size_t s_nReaderThreadCount = 4  ;
        static size_t s_nWriterThreadCount = 4  ;
        static size_t s_nQueueSize = 4000000   ;

        struct Value {
            size_t      nNo         ;
            size_t      nWriterNo   ;
        };
    }

    class Queue_ReaderWriter_MT: public CppUnitMini::TestCase
    {
        template <class QUEUE>
        class WriterThread: public CppUnitMini::TestThread
        {
            virtual TestThread *    clone()
            {
                return new WriterThread( *this )  ;
            }
        public:
            QUEUE&              m_Queue ;
            double              m_fTime ;
            size_t              m_nPushFailed   ;

        public:
            WriterThread( CppUnitMini::ThreadPool& pool, QUEUE& q )
                : CppUnitMini::TestThread( pool )
                , m_Queue( q )
            {}
            WriterThread( WriterThread& src )
                : CppUnitMini::TestThread( src )
                , m_Queue( src.m_Queue )
            {}

            Queue_ReaderWriter_MT&  getTest()
            {
                return reinterpret_cast<Queue_ReaderWriter_MT&>( m_Pool.m_Test )   ;
            }

            virtual void init()
            {
                cds::threading::Manager::attachThread()     ;
            }
            virtual void fini()
            {
                cds::threading::Manager::detachThread()   ;
            }

            virtual void test()
            {
                size_t nPushCount = getTest().m_nThreadPushCount  ;
                Value v             ;
                v.nWriterNo = m_nThreadNo   ;
                v.nNo = 0           ;
                m_nPushFailed = 0   ;

                m_fTime = m_Timer.duration()        ;

                while ( v.nNo < nPushCount ) {
                    if ( m_Queue.push( v ))
                        ++v.nNo             ;
                    else
                        ++m_nPushFailed     ;
                }

                m_fTime = m_Timer.duration() - m_fTime  ;
                getTest().m_nWriterDone.inc( cds::membar_acq_rel::order )   ;
            }
        };

        template <class QUEUE>
        class ReaderThread: public CppUnitMini::TestThread
        {
            virtual TestThread *    clone()
            {
                return new ReaderThread( *this )  ;
            }
        public:
            QUEUE&              m_Queue ;
            double              m_fTime ;
            size_t              m_nPopEmpty     ;
            size_t              m_nPopped       ;
            size_t              m_nBadWriter      ;

            typedef std::vector<size_t> TPoppedData ;
            typedef std::vector<size_t>::iterator       data_iterator ;
            typedef std::vector<size_t>::const_iterator const_data_iterator ;

            std::vector<TPoppedData>        m_WriterData        ;

        private:
            void initPoppedData()
            {
                const size_t nWriterCount = s_nWriterThreadCount                ;
                const size_t nWriterPushCount = getTest().m_nThreadPushCount    ;
                m_WriterData.resize( nWriterCount ) ;
                for ( size_t i = 0; i < nWriterCount; ++i )
                    m_WriterData[i].reserve( nWriterPushCount )  ;
            }

        public:
            ReaderThread( CppUnitMini::ThreadPool& pool, QUEUE& q )
                : CppUnitMini::TestThread( pool )
                , m_Queue( q )
            {
                initPoppedData()    ;
            }
            ReaderThread( ReaderThread& src )
                : CppUnitMini::TestThread( src )
                , m_Queue( src.m_Queue )
            {
                initPoppedData()    ;
            }

            Queue_ReaderWriter_MT&  getTest()
            {
                return reinterpret_cast<Queue_ReaderWriter_MT&>( m_Pool.m_Test )   ;
            }

            virtual void init()
            {
                cds::threading::Manager::attachThread()     ;
            }
            virtual void fini()
            {
                cds::threading::Manager::detachThread()   ;
            }

            virtual void test()
            {
                m_nPopEmpty = 0     ;
                m_nPopped = 0       ;
                m_nBadWriter = 0      ;
                const size_t nTotalWriters = s_nWriterThreadCount ;
                Value v       ;

                m_fTime = m_Timer.duration()        ;

                while ( true ) {
                    if ( m_Queue.pop( v ) ) {
                        ++m_nPopped ;
                        if ( v.nWriterNo >= 0 && v.nWriterNo < nTotalWriters )
                            m_WriterData[ v.nWriterNo ].push_back( v.nNo )    ;
                        else
                            ++m_nBadWriter      ;
                    }
                    else
                        ++m_nPopEmpty           ;

                    if ( m_Queue.empty() )
                        if ( getTest().m_nWriterDone.load(cds::membar_acquire::order) >= nTotalWriters )
                            break   ;
                }

                m_fTime = m_Timer.duration() - m_fTime ;
            }
        };

    protected:
        size_t                  m_nThreadPushCount  ;
        cds::atomic<size_t>     m_nWriterDone       ;

    protected:
        template <class QUEUE>
        void analyze( CppUnitMini::ThreadPool& pool, QUEUE& testQueue  )
        {
            typedef ReaderThread<QUEUE> Reader  ;
            typedef WriterThread<QUEUE> Writer  ;
            typedef typename Reader::const_data_iterator    ReaderIterator ;

            double fTimeWriter = 0      ;
            double fTimeReader = 0      ;
            size_t nTotalPops = 0       ;
            size_t nPopFalse = 0        ;

            std::vector< Reader * > arrReaders ;

            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it ) {
                Reader * pReader = dynamic_cast<Reader *>( *it ) ;
                if ( pReader ) {
                    fTimeReader += pReader->m_fTime     ;
                    nTotalPops += pReader->m_nPopped    ;
                    nPopFalse += pReader->m_nPopEmpty   ;
                    arrReaders.push_back( pReader )     ;
                    CPPUNIT_ASSERT_MSG( pReader->m_nBadWriter == 0, "reader " << pReader->m_nThreadNo << " bad writer event count=" << pReader->m_nBadWriter ) ;
                }
                else {
                    Writer * pWriter = dynamic_cast<Writer *>( *it ) ;
                    CPPUNIT_ASSERT( pWriter != NULL )   ;
                    fTimeWriter += pWriter->m_fTime     ;
                    if ( !boost::is_base_of<cds::concept::bounded_container, QUEUE>::value ) {
                        CPPUNIT_ASSERT_MSG( pWriter->m_nPushFailed == 0,
                            "writer " << pWriter->m_nThreadNo << " push failed count=" << pWriter->m_nPushFailed ) ;
                    }
                }
            }
            CPPUNIT_MSG( "     Duration: readers=" << (fTimeReader / s_nReaderThreadCount)
                << ", writer=" << (fTimeWriter / s_nWriterThreadCount)
                << ", pop false=" << nPopFalse
            )   ;
            size_t nQueueSize = m_nThreadPushCount * s_nWriterThreadCount ;
            CPPUNIT_ASSERT_MSG( nTotalPops == nQueueSize, "popped=" << nTotalPops << " must be " << nQueueSize ) ;
            CPPUNIT_ASSERT( testQueue.empty() )             ;

            // Tests that all items have been popped
            CPPUNIT_MSG( "   Test consistency of popped sequence..." ) ;
            size_t nErrors = 0  ;
            for ( size_t nWriter = 0; nWriter < s_nWriterThreadCount; ++nWriter ) {
                std::vector<size_t> arrData ;
                arrData.reserve( m_nThreadPushCount )   ;
                nErrors = 0  ;
                for ( size_t nReader = 0; nReader < arrReaders.size(); ++nReader ) {
                    ReaderIterator it = arrReaders[nReader]->m_WriterData[nWriter].begin()     ;
                    ReaderIterator itEnd = arrReaders[nReader]->m_WriterData[nWriter].end()    ;
                    if ( it != itEnd ) {
                        ReaderIterator itPrev = it ;
                        for ( ++it; it != itEnd; ++it ) {
                            CPPUNIT_ASSERT_MSG( *itPrev < *it, "Reader " << nReader << ", Writer " << nWriter << ": prev=" << *itPrev << ", cur=" << *it ) ;
                            if ( ++nErrors > 10 )
                                return ;
                            itPrev = it ;
                        }
                    }

                    for ( it = arrReaders[nReader]->m_WriterData[nWriter].begin(); it != itEnd; ++it )
                        arrData.push_back( *it )    ;
                }
                std::sort( arrData.begin(), arrData.end() )  ;
                nErrors = 0  ;
                for ( size_t i=1; i < arrData.size(); ++i ) {
                    if ( arrData[i-1] + 1 != arrData[i] ) {
                        CPPUNIT_ASSERT_MSG( arrData[i-1] + 1 == arrData[i], "Writer " << nWriter << ": [" << (i-1) << "]=" << arrData[i-1] << ", [" << i << "]=" << arrData[i] ) ;
                        if ( ++nErrors > 10 )
                            return   ;
                    }
                }
            }
        }

        template <class QUEUE>
        void test()
        {

            CPPUNIT_MSG( "Queue Reader/Writer test,\n    reader count=" << s_nReaderThreadCount << " writer count=" << s_nWriterThreadCount << "..." )   ;

            m_nThreadPushCount = s_nQueueSize / s_nWriterThreadCount    ;

            QUEUE testQueue ;
            CppUnitMini::ThreadPool pool( *this )   ;

            m_nWriterDone.store<cds::membar_relaxed>( 0 )  ;

            // Writers must be first
            pool.add( new WriterThread<QUEUE>( pool, testQueue ), s_nWriterThreadCount )       ;
            pool.add( new ReaderThread<QUEUE>( pool, testQueue ), s_nReaderThreadCount )       ;

            //CPPUNIT_MSG( "   Reader/Writer test, reader count=" << s_nReaderThreadCount << " writer count=" << s_nWriterThreadCount << "..." )   ;
            pool.run()  ;

            analyze( pool, testQueue )     ;
        }

        void setUpParams( const CppUnitMini::TestCfg& cfg ) {
            s_nReaderThreadCount = cfg.getULong("ReaderCount", 4 ) ;
            s_nWriterThreadCount = cfg.getULong("WriterCount", 4 ) ;
            s_nQueueSize = cfg.getULong("QueueSize", 10000000 );
        }

    protected:
        TEST_CASE( MoirQueue_HP, Value )
        TEST_CASE( MoirQueue_HRC, Value )
        TEST_CASE( MoirQueue_PTB, Value )
#ifdef CDS_DWORD_CAS_SUPPORTED
        TEST_CASE( MoirQueue_Tagged, Value )
#endif
        TEST_CASE( MSQueue_HP, Value  )
        TEST_CASE( MSQueue_HRC, Value )
        TEST_CASE( MSQueue_PTB, Value )
#ifdef CDS_DWORD_CAS_SUPPORTED
        TEST_CASE( MSQueue_Tagged, Value )
#endif
        TEST_CASE( LMSQueue_HP, Value  )
        TEST_CASE( LMSQueue_PTB, Value )

        TEST_CASE( MoirQueue_HP_Counted, Value )
        TEST_CASE( MoirQueue_HRC_Counted, Value )
        TEST_CASE( MoirQueue_PTB_Counted, Value )
#ifdef CDS_DWORD_CAS_SUPPORTED
        TEST_CASE( MoirQueue_Tagged_Counted, Value )
#endif
        TEST_CASE( MSQueue_HP_Counted, Value )
        TEST_CASE( MSQueue_HRC_Counted, Value )
        TEST_CASE( MSQueue_PTB_Counted, Value )
#ifdef CDS_DWORD_CAS_SUPPORTED
        TEST_CASE( MSQueue_Tagged_Counted, Value )
#endif
        TEST_CASE( LMSQueue_HP_Counted, Value )
        TEST_CASE( LMSQueue_PTB_Counted, Value)

        TEST_CASE( TZCyclicQueue, Value )
        TEST_CASE( TZCyclicQueue_Counted, Value )

        TEST_CASE( RWQueue_Spinlock, Value )
        TEST_CASE( RWQueue_Spinlock_Counted, Value )

        TEST_CASE( StdQueue_deque_Spinlock, Value )
        TEST_CASE( StdQueue_list_Spinlock, Value )
        TEST_CASE( StdQueue_deque_BoostMutex, Value )
        TEST_CASE( StdQueue_list_BoostMutex, Value )
#ifdef UNIT_LOCK_WIN_CS
        TEST_CASE( StdQueue_deque_WinCS, Value )
        TEST_CASE( StdQueue_list_WinCS, Value )
        TEST_CASE( StdQueue_deque_WinMutex, Value )
        TEST_CASE( StdQueue_list_WinMutex, Value )
#endif
        TEST_CASE( HASQueue_Spinlock, Value )

        CPPUNIT_TEST_SUITE(Queue_ReaderWriter_MT)
            CPPUNIT_TEST(MoirQueue_HP)              ;
            CPPUNIT_TEST(MoirQueue_HP_Counted)      ;
            CPPUNIT_TEST(MoirQueue_HRC)              ;
            CPPUNIT_TEST(MoirQueue_HRC_Counted)      ;
            CPPUNIT_TEST(MoirQueue_PTB)              ;
            CPPUNIT_TEST(MoirQueue_PTB_Counted)      ;
#ifdef CDS_DWORD_CAS_SUPPORTED
            CPPUNIT_TEST(MoirQueue_Tagged)          ;
            CPPUNIT_TEST(MoirQueue_Tagged_Counted)  ;
#endif

            CPPUNIT_TEST(MSQueue_HP)                ;
            CPPUNIT_TEST(MSQueue_HP_Counted)        ;
            CPPUNIT_TEST(MSQueue_HRC)               ;
            CPPUNIT_TEST(MSQueue_HRC_Counted)       ;
            CPPUNIT_TEST(MSQueue_PTB)               ;
            CPPUNIT_TEST(MSQueue_PTB_Counted)       ;
#ifdef CDS_DWORD_CAS_SUPPORTED
            CPPUNIT_TEST(MSQueue_Tagged)            ;
            CPPUNIT_TEST(MSQueue_Tagged_Counted)    ;
#endif
            CPPUNIT_TEST(LMSQueue_HP)               ;
            CPPUNIT_TEST(LMSQueue_HP_Counted)       ;
            CPPUNIT_TEST(LMSQueue_PTB)              ;
            CPPUNIT_TEST(LMSQueue_PTB_Counted)      ;

            //CPPUNIT_TEST(TZCyclicQueue)             ;
            //CPPUNIT_TEST(TZCyclicQueue_Counted)     ;

            CPPUNIT_TEST(RWQueue_Spinlock)          ;
            CPPUNIT_TEST(RWQueue_Spinlock_Counted)  ;

            CPPUNIT_TEST(StdQueue_deque_Spinlock)   ;
            CPPUNIT_TEST(StdQueue_list_Spinlock)    ;
            CPPUNIT_TEST(StdQueue_deque_BoostMutex) ;
            CPPUNIT_TEST(StdQueue_list_BoostMutex)  ;
#ifdef UNIT_LOCK_WIN_CS
            CPPUNIT_TEST(StdQueue_deque_WinCS) ;
            CPPUNIT_TEST(StdQueue_list_WinCS)  ;
            //CPPUNIT_TEST(StdQueue_deque_WinMutex) ;
            //CPPUNIT_TEST(StdQueue_list_WinMutex)  ;
#endif
            CPPUNIT_TEST( HASQueue_Spinlock )       ;
        CPPUNIT_TEST_SUITE_END();
    };

} // namespace queue

CPPUNIT_TEST_SUITE_REGISTRATION(queue::Queue_ReaderWriter_MT);
