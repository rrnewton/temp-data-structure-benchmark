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
        static size_t s_nReaderThreadCount = 1  ;
        static size_t s_nWriterThreadCount = 1  ;
        static size_t s_nQueueSize = 64000000   ;
        static size_t s_nRepetitionCount = 2  ;

        struct Value {
            long nNo;
            long e1;
            long e2;
            long e3;
            long e4;
            long e5;
            long e6;
            long e7;
        };
    }

    class Queue_Membench_Cacheline_MT: public CppUnitMini::TestCase
    {
        template <class QUEUE>
        class WriterThread: public CppUnitMini::TestThread
        {
            virtual TestThread *clone() { return new WriterThread( *this ); }
        public:
            QUEUE&              m_Queue;

            WriterThread( CppUnitMini::ThreadPool& pool, QUEUE& q )
                : CppUnitMini::TestThread( pool )
                , m_Queue( q )
            {}
            WriterThread( WriterThread& src )
                : CppUnitMini::TestThread( src )
                , m_Queue( src.m_Queue )
            {}

            Queue_Membench_Cacheline_MT&  getTest()
            {
                return reinterpret_cast<Queue_Membench_Cacheline_MT&>( m_Pool.m_Test )   ;
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
                size_t nPushCount = getTest().m_nThreadPushCount;
                Value v;

                for (v.nNo = 0; v.nNo < nPushCount; ++v.nNo)
                    while (! m_Queue.push( v ))
                        ;
            }
        };

        template <class QUEUE>
        class ReaderThread: public CppUnitMini::TestThread
        {
            virtual TestThread *clone() { return new ReaderThread( *this ); }
        public:
            QUEUE&              m_Queue ;

            ReaderThread( CppUnitMini::ThreadPool& pool, QUEUE& q )
                : CppUnitMini::TestThread( pool )
                , m_Queue( q )
            {}
            ReaderThread( ReaderThread& src )
                : CppUnitMini::TestThread( src )
                , m_Queue( src.m_Queue )
            {}

            Queue_Membench_Cacheline_MT&  getTest()
            {
                return reinterpret_cast<Queue_Membench_Cacheline_MT&>( m_Pool.m_Test )   ;
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
                size_t nPushCount = getTest().m_nThreadPushCount;
                Value v;

                for (size_t i = 0; i < nPushCount; i++)
                    while (! m_Queue.pop( v ) )
                      ;
            }
        };

    protected:
        size_t                  m_nThreadPushCount  ;

    protected:
        template <class QUEUE>
        void test()
        {
            double times[s_nRepetitionCount];

            m_nThreadPushCount = s_nQueueSize / 8;

            CPPUNIT_MSG( "Queue MEMBENCH test,\n    reader count=" << s_nReaderThreadCount << " writer count=" << s_nWriterThreadCount << " element count=" << m_nThreadPushCount << "..." )   ;

            QUEUE testQueue ;
            CppUnitMini::ThreadPool pool( *this )   ;

            // Writers must be first
            pool.add( new WriterThread<QUEUE>( pool, testQueue ), s_nWriterThreadCount );
            pool.add( new ReaderThread<QUEUE>( pool, testQueue ), s_nReaderThreadCount );

            for (long i = 0; i < s_nRepetitionCount; i++) {
              cds::OS::Timer m;
              double start = m.duration();
              pool.run();
              double end = m.duration();
              times[i] = end-start;
            }

            double mbs = s_nQueueSize*8*2*1e-06;
            double avgtime = 0;
            double mintime = std::numeric_limits<double>::max();
            double maxtime = 0;

            for (long i = 1; i < s_nRepetitionCount; i++) /* note -- skip first iteration */
            {
              avgtime = avgtime + times[i];
              mintime = std::min(mintime, times[i]);
              maxtime = std::max(maxtime, times[i]);
            }
            avgtime = avgtime/(double)(s_nRepetitionCount-1);

            CPPUNIT_MSG( "Throughput(MB/s) Max=" << (mbs/mintime)
                    << "  Avg=" << (mbs/avgtime)
                    << "  Min=" << (mbs/maxtime)
                    << std::endl );
        }

        void setUpParams( const CppUnitMini::TestCfg& cfg ) {
            s_nReaderThreadCount = cfg.getULong("ReaderCount", 1 ) ;
            s_nWriterThreadCount = cfg.getULong("WriterCount", 1 ) ;
            s_nQueueSize = cfg.getULong("QueueSize", 64000000 );
            s_nRepetitionCount = cfg.getULong("RepetitionCount", 2 );
        }

    protected:
        TEST_CASE( CSQueue, Value )
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

        CPPUNIT_TEST_SUITE(Queue_Membench_Cacheline_MT)
            CPPUNIT_TEST(CSQueue)              ;
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

CPPUNIT_TEST_SUITE_REGISTRATION(queue::Queue_Membench_Cacheline_MT);
