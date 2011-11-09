/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include "cppunit/thread.h"
#include "bag/bag_type.h"
#include <vector>
#include <algorithm>

// Multi-threaded random bag test
namespace bag {

#define TEST_CASE( B, V, N ) void B() { test< Types<V, N>::B >(); }

    namespace {
        static size_t s_nReaderThreadCount = 1  ;
        static size_t s_nWriterThreadCount = 1  ;
        static size_t s_nElementCount = 64000000   ;
    }

    class Bag_Membench_Long_MT: public CppUnitMini::TestCase
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

            Bag_Membench_Long_MT&  getTest()
            {
                return reinterpret_cast<Bag_Membench_Long_MT&>( m_Pool.m_Test )   ;
            }

            virtual void init()
            {
                cds::threading::Manager::attachThread()     ;
                m_Bag.initThread( m_threadId );
            }
            virtual void fini()
            {
                cds::threading::Manager::detachThread()   ;
            }

            virtual void test()
            {
                size_t nPushCount = getTest().m_nThreadPushCount;
                long v;

                for (v = 0; v < nPushCount; ++v) {
                    m_Bag.add( v );
#if _DEBUG
                    // Just so writer and reader have approximate "computation"
                    CPPUNIT_MSG( "Added " << v );
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

            Bag_Membench_Long_MT&  getTest()
            {
                return reinterpret_cast<Bag_Membench_Long_MT&>( m_Pool.m_Test )   ;
            }

            virtual void init()
            {
                cds::threading::Manager::attachThread()     ;
                m_Bag.initThread( m_threadId ) ;
            }
            virtual void fini()
            {
                cds::threading::Manager::detachThread()   ;
            }

            virtual void test()
            {
                size_t nPushCount = getTest().m_nThreadPushCount;
                long v;

                for (size_t i = 0; i < nPushCount; i++) {
                    while (! m_Bag.tryRemoveAny( v ) )
                        ;
#ifdef _DEBUG
                    CPPUNIT_MSG( "Removed " << v );
#endif
                }

#ifdef _DEBUG
                    CPPUNIT_MSG( "Consumer finished." );
#endif
            }
        };

    protected:
        size_t                  m_nThreadPushCount   ;

    protected:
        template <class BAG>
        void test()
        {
            m_nThreadPushCount = s_nElementCount;

            CPPUNIT_MSG( "Bag MEMBENCH test,\n    reader count=" << s_nReaderThreadCount <<
                " writer count=" << s_nWriterThreadCount << " element count=" << m_nThreadPushCount
                << "..." << std::endl)   ;

            long sentinel = { -1 };
            BAG testBag ( sentinel ) ;
            CppUnitMini::ThreadPool pool( *this )   ;

            // For now we are hardcoding the number of threads because of NR_THREADS template param
            CPPUNIT_ASSERT ( s_nReaderThreadCount == 1 && s_nWriterThreadCount == 1 );
            // Writers must be first
            pool.add( new WriterThread<BAG>( pool, testBag, 0 ), 1 );
            pool.add( new ReaderThread<BAG>( pool, testBag, 1 ), 1 );

            cds::OS::Timer m;
            double start = m.duration();
            pool.run();
            double end = m.duration();

            double mbs = s_nElementCount*8*2*1e-06;
            CPPUNIT_MSG( "Throughput: " << (mbs/(end-start)) << " MBs" << std::endl );
            //analyze( pool, testQueue )     ;
        }

        void setUpParams( const CppUnitMini::TestCfg& cfg ) {
            s_nReaderThreadCount = cfg.getULong("ReaderCount", 1 ) ;
            s_nWriterThreadCount = cfg.getULong("WriterCount", 1 ) ;
            s_nElementCount = cfg.getULong("ElementCount", 64000000 );
        }

    protected:
        TEST_CASE( SBag_HRC, long, 2 )

        CPPUNIT_TEST_SUITE(Bag_Membench_Long_MT)
            CPPUNIT_TEST(SBag_HRC)              ;
        CPPUNIT_TEST_SUITE_END();
    };

} // namespace bag

CPPUNIT_TEST_SUITE_REGISTRATION(bag::Bag_Membench_Long_MT);
