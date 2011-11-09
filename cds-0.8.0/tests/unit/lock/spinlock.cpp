/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include "cppunit/thread.h"

#include <cds/lock/spinlock.h>

// Multi-threaded stack test for push operation
namespace lock {

#define TEST_CASE( N, L )   void N() { test<L>(); }

    namespace {
        static size_t s_nThreadCount = 8        ;
        static size_t s_nLoopCount = 1000000    ;     // loop count per thread

        static size_t   s_nSharedInt  ;
    }

    class Spinlock_MT: public CppUnitMini::TestCase
    {
        template <class LOCK>
        class Thread: public CppUnitMini::TestThread
        {
            virtual TestThread *    clone()
            {
                return new Thread( *this )  ;
            }
        public:
            LOCK&               m_Lock          ;
            double              m_fTime         ;

        public:
            Thread( CppUnitMini::ThreadPool& pool, LOCK& l )
                : CppUnitMini::TestThread( pool )
                , m_Lock( l )
            {}
            Thread( Thread& src )
                : CppUnitMini::TestThread( src )
                , m_Lock( src.m_Lock )
            {}

            Spinlock_MT&  getTest()
            {
                return reinterpret_cast<Spinlock_MT&>( m_Pool.m_Test )   ;
            }

            virtual void init()
            {}
            virtual void fini()
            {}

            virtual void test()
            {
                m_fTime = m_Timer.duration()    ;

                for ( size_t i  = 0; i < s_nLoopCount; ++i ) {
                    m_Lock.lock()   ;
                    ++s_nSharedInt  ;
                    m_Lock.unlock() ;
                }

                m_fTime = m_Timer.duration() - m_fTime  ;
            }
        };

    protected:
        void setUpParams( const CppUnitMini::TestCfg& cfg ) {
            s_nThreadCount = cfg.getULong("ThreadCount", 8 ) ;
            s_nLoopCount = cfg.getULong("LoopCount", 1000000 );
        }

        template <class LOCK>
        void test()
        {
            LOCK    testLock    ;

            for ( size_t nThreadCount = 1; nThreadCount <= s_nThreadCount; nThreadCount *= 2 ) {
                s_nSharedInt = 0    ;

                CppUnitMini::ThreadPool pool( *this )   ;
                pool.add( new Thread<LOCK>( pool, testLock ), nThreadCount )  ;

                CPPUNIT_MSG( "   Lock test, thread count=" << nThreadCount
                    << " loop per thread=" << s_nLoopCount
                    << "...")   ;
                cds::OS::Timer      timer   ;
                pool.run()  ;
                CPPUNIT_MSG( "     Duration=" << timer.duration() )     ;

                CPPUNIT_ASSERT_EX( s_nSharedInt == nThreadCount * s_nLoopCount,
                    "Expected=" << nThreadCount * s_nLoopCount
                    << " real=" << s_nSharedInt ) ;
            }
        }

        typedef cds::lock::SpinT<cds::atomic32_t, cds::backoff::yield>  Spin32_yield ;
        typedef cds::lock::SpinT<cds::atomic32_t, cds::backoff::hint>   Spin32_hint ;
        typedef cds::lock::SpinT<cds::atomic64_t, cds::backoff::yield>  Spin64_yield ;
        typedef cds::lock::SpinT<cds::atomic64_t, cds::backoff::hint>   Spin64_hint ;

        TEST_CASE(spinLock_exp,         cds::lock::Spin             )    ;
        TEST_CASE(spinLock_yield,       cds::lock::Spinlock<cds::backoff::yield> ) ;
        TEST_CASE(spinLock_hint,        cds::lock::Spinlock<cds::backoff::hint> )  ;
        TEST_CASE(spinLock32_exp,       cds::lock::Spin32           )    ;
        TEST_CASE(spinLock32_yield,     Spin32_yield ) ;
        TEST_CASE(spinLock32_hint,      Spin32_hint  ) ;
        TEST_CASE(spinLock64_exp,       cds::lock::Spin64 )    ;
        TEST_CASE(spinLock64_yield,     Spin64_yield ) ;
        TEST_CASE(spinLock64_hint,      Spin64_hint  ) ;
        TEST_CASE(recursiveSpinLock,    cds::lock::ReentrantSpin    )    ;
        TEST_CASE(recursiveSpinLock32,  cds::lock::ReentrantSpin32  )    ;
        TEST_CASE(recursiveSpinLock64,  cds::lock::ReentrantSpin64  )    ;

    protected:
        CPPUNIT_TEST_SUITE(Spinlock_MT)
            CPPUNIT_TEST(spinLock_exp)          ;
            CPPUNIT_TEST(spinLock_yield)        ;
            CPPUNIT_TEST(spinLock_hint)         ;
            CPPUNIT_TEST(spinLock32_exp)        ;
            CPPUNIT_TEST(spinLock32_yield)      ;
            CPPUNIT_TEST(spinLock32_hint)       ;
            CPPUNIT_TEST(spinLock64_exp)        ;
            CPPUNIT_TEST(spinLock64_yield)      ;
            CPPUNIT_TEST(spinLock64_hint)       ;
            CPPUNIT_TEST(recursiveSpinLock)     ;
            CPPUNIT_TEST(recursiveSpinLock32)   ;
            CPPUNIT_TEST(recursiveSpinLock64)   ;
        CPPUNIT_TEST_SUITE_END();
    };

} // namespace lock

CPPUNIT_TEST_SUITE_REGISTRATION(lock::Spinlock_MT);
