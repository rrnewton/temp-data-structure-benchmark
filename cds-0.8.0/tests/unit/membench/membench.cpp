
#include "cppunit/thread.h"
#include <cds/queue/csqueue.h>
#include <cds/queue/moir_queue_hzp.h>
#include <cds/queue/msqueue_hzp.h>
#include <cds/queue/lmsqueue_hzp.h>
#include "queue/std_queue.h"
#include <vector>
#include <algorithm>

namespace membench {

#define BENCH_CASE(N, T) void N() { test<T >(); }

    namespace {
        static size_t s_nReaderThreadCount = 1  ;
        static size_t s_nWriterThreadCount = 1  ;
        static size_t s_nQueueSize = 64000000   ;
        static size_t s_nRepetitionCount = 2  ;

        struct Cacheline { long e0, e1, e2, e3, e4, e5, e6, e7; };
    }

    class Membench_MT: public CppUnitMini::TestCase
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

            Membench_MT&  getTest()
            {
                return reinterpret_cast<Membench_MT&>( m_Pool.m_Test )   ;
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

            Membench_MT&  getTest()
            {
                return reinterpret_cast<Membench_MT&>( m_Pool.m_Test )   ;
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
        template <class T>
        void test()
        {
            double times[s_nRepetitionCount];

            m_nThreadPushCount = s_nQueueSize / 8;

            CPPUNIT_MSG( "Queue MEMBENCH test,\n    reader count=" << s_nReaderThreadCount << " writer count=" << s_nWriterThreadCount << " element count=" << m_nThreadPushCount << "..." )   ;

            T testQueue ;
            CppUnitMini::ThreadPool pool( *this )   ;

            // Writers must be first
            pool.add( new WriterThread<T>( pool, testQueue ), s_nWriterThreadCount );
            pool.add( new ReaderThread<T>( pool, testQueue ), s_nReaderThreadCount );

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
        typedef cds::queue::CircularFifo<long, 64000> CFLong;
        BENCH_CASE( Long_CircularFifo, CFLong )

        typedef cds::queue::MoirQueue<cds::gc::hzp_gc, long> MoirQueue;
        BENCH_CASE( Long_MoirQueueHP, MoirQueue )

        typedef cds::queue::MSQueue<cds::gc::hzp_gc, long> MSQueue;
        BENCH_CASE( Long_MSQueueHP, MSQueue )

        typedef cds::queue::LMSQueue<cds::gc::hzp_gc, long> LMSQueue;
        BENCH_CASE( Long_LMSQueueHP, LMSQueue )

        typedef queue::StdQueue_deque<long> StdDeque;
        BENCH_CASE( Long_StdQueue_deque_Spinlock, StdDeque )

        CPPUNIT_TEST_SUITE(Membench_MT)
            CPPUNIT_TEST(Long_CircularFifo);
            CPPUNIT_TEST(Long_MoirQueueHP);
            CPPUNIT_TEST(Long_MSQueueHP);
            CPPUNIT_TEST(Long_LMSQueueHP);
            CPPUNIT_TEST(Long_StdQueue_deque_Spinlock);
        CPPUNIT_TEST_SUITE_END();
    };

} // namespace queue

CPPUNIT_TEST_SUITE_REGISTRATION(membench::Membench_MT);
