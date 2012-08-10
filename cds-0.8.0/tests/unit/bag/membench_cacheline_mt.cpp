/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/

#define _GNU_SOURCE
#include "cppunit/thread.h"
#include "bag/bag_type.h"
#include <vector>
#include <algorithm>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

// Multi-threaded random bag test
namespace bag {

  void setAffinity(int tid, int cpuid) {
    cpu_set_t mask;
    int error;

    CPU_ZERO(&mask);
    CPU_SET(cpuid, &mask);
    error = sched_setaffinity(0, sizeof(mask), &mask);
    if (error == 0) {
      printf("Thread %d has been assigned to CPU %d\n", tid, cpuid);
      fflush(stdout);
    } else {
      printf("Thread %d failed to be assigned to CPU %d\n", tid, cpuid);
      fflush(stdout);
      exit(-1);
    }
}


#define TEST_CASE( B, V, N ) void B() { test< Types<V, N>::B >(); }

    namespace {
        static size_t s_nReaderThreadCount = 1  ;
        static size_t s_nWriterThreadCount = 1  ;
        static size_t s_nElementCount = 512000000   ;
        static size_t s_nRepetitionCount = 2  ;

        struct Cacheline {
            long nNo;
            long e1;
            long e2;
            long e3;
            long e4;
            long e5;
            long e6;
            long e7;

            inline bool operator==(const Cacheline &other) const {
              return nNo == other.nNo;
            }
        };
    }

    class Bag_Membench_Cacheline_MT: public CppUnitMini::TestCase
    {
        template <class BAG>
        class WriterThread: public CppUnitMini::TestThread
        {
            virtual TestThread *clone() { return new WriterThread( *this ); }
        public:
            BAG&              m_Bag;
            int               m_threadId;

            WriterThread( CppUnitMini::ThreadPool& pool, BAG& b, int threadId )
                : CppUnitMini::TestThread( pool )
                , m_Bag( b )
                , m_threadId ( threadId )
            {}
            WriterThread( WriterThread& src )
                : CppUnitMini::TestThread( src )
                , m_Bag( src.m_Bag )
                , m_threadId ( src.m_threadId )
            {}

            Bag_Membench_Cacheline_MT&  getTest()
            {
                return reinterpret_cast<Bag_Membench_Cacheline_MT&>( m_Pool.m_Test )   ;
            }

            virtual void init()
            {
                cds::threading::Manager::attachThread()     ;
                m_Bag.initThread( m_threadId );
                setAffinity(m_threadId, 2+m_threadId);
            }
            virtual void fini()
            {
                cds::threading::Manager::detachThread()   ;
            }

            virtual void test()
            {
                size_t nPushCount = getTest().m_nThreadPushCount;
                Cacheline v;

                for (v.nNo = 0; v.nNo < nPushCount; ++v.nNo) {
                    m_Bag.add( v );
#if _DEBUG
                    // Just so writer and reader have approximate "computation"
                    CPPUNIT_MSG( "Added " << v.nNo );
#endif
                }
            }
        };

        template <class BAG>
        class ReaderThread: public CppUnitMini::TestThread
        {
            virtual TestThread *clone() { return new ReaderThread( *this ); }
        public:
            BAG&              m_Bag;
            int               m_threadId;

            ReaderThread( CppUnitMini::ThreadPool& pool, BAG& b, int threadId )
                : CppUnitMini::TestThread( pool )
                , m_Bag( b )
                , m_threadId ( threadId )
            {}
            ReaderThread( ReaderThread& src )
                : CppUnitMini::TestThread( src )
                , m_Bag( src.m_Bag )
                , m_threadId ( src.m_threadId )
            {}

            Bag_Membench_Cacheline_MT&  getTest()
            {
                return reinterpret_cast<Bag_Membench_Cacheline_MT&>( m_Pool.m_Test )   ;
            }

            virtual void init()
            {
                cds::threading::Manager::attachThread()     ;
                m_Bag.initThread( m_threadId ) ;
                setAffinity(m_threadId, 2+m_threadId);
            }
            virtual void fini()
            {
                cds::threading::Manager::detachThread()   ;
            }

            virtual void test()
            {
                size_t nPushCount = getTest().m_nThreadPushCount;
                Cacheline v;

                for (size_t i = 0; i < nPushCount; i++) {
                    while (! m_Bag.tryRemoveAny( v ) )
                        ;
#ifdef _DEBUG
                    CPPUNIT_MSG( "Removed " << v.nNo );
#endif
                }

#ifdef _DEBUG
                    CPPUNIT_MSG( "Consumer finished." );
#endif
            }
        };

    protected:
        size_t                  m_nThreadPushCount  ;

    protected:
        template <class BAG>
        void test()
        {
            double times[s_nRepetitionCount];
            m_nThreadPushCount = s_nElementCount / 8   ; // 8 = sizeof(Cacheline)

            CPPUNIT_MSG( "Bag MEMBENCH Cacheline test,\n    reader count=" << s_nReaderThreadCount <<
                " writer count=" << s_nWriterThreadCount << " element count=" << m_nThreadPushCount
                << "..." << std::endl)   ;

            Cacheline sentinel = { -1 };
            BAG testBag ( sentinel ) ;
            CppUnitMini::ThreadPool pool( *this )   ;

            // For now we are hardcoding the number of threads because of NR_THREADS template param
            CPPUNIT_ASSERT ( s_nReaderThreadCount == 1 && s_nWriterThreadCount == 1 );
            // Writers must be first
            pool.add( new WriterThread<BAG>( pool, testBag, 0 ), 1 );
            pool.add( new ReaderThread<BAG>( pool, testBag, 1 ), 1 );

            for (long i = 0; i < s_nRepetitionCount; i++) {
              cds::OS::Timer m;
              double start = m.duration();
              pool.run();
              double end = m.duration();
              times[i] = end-start;
            }

            double mbs = (s_nElementCount*1e-06)*8*2;
            double avgtime = 0;
            double mintime = std::numeric_limits<double>::max();
            double maxtime = 0;

            if (s_nRepetitionCount == 1)
              avgtime = mintime = maxtime = *times;
            else {
              for (long i = 1; i < s_nRepetitionCount; i++) /* note -- skip first iteration */
              {
                avgtime = avgtime + times[i];
                mintime = std::min(mintime, times[i]);
                maxtime = std::max(maxtime, times[i]);
              }
              avgtime = avgtime/(double)(s_nRepetitionCount-1);
            }

            CPPUNIT_MSG( "Throughput(MB/s) Max=" << (mbs/mintime)
                    << "  Avg=" << (mbs/avgtime)
                    << "  Min=" << (mbs/maxtime)
                    << std::endl );
        }

        void setUpParams( const CppUnitMini::TestCfg& cfg ) {
            s_nReaderThreadCount = cfg.getULong("ReaderCount", 1 ) ;
            s_nWriterThreadCount = cfg.getULong("WriterCount", 1 ) ;
            s_nElementCount = cfg.getULong("ElementCount", 64000000 );
            s_nRepetitionCount = cfg.getULong("RepetitionCount", 2 );
        }

    protected:
        TEST_CASE( SBag_HRC, Cacheline, 2 )
        TEST_CASE( FFQueue, Cacheline, 2 )

        CPPUNIT_TEST_SUITE(Bag_Membench_Cacheline_MT)
            CPPUNIT_TEST(SBag_HRC)              ;
            CPPUNIT_TEST(FFQueue)              ;
        CPPUNIT_TEST_SUITE_END();
    };

} // namespace bag

CPPUNIT_TEST_SUITE_REGISTRATION(bag::Bag_Membench_Cacheline_MT);
