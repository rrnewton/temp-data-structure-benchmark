/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include "cppunit/thread.h"
#include "stack/stack_type.h"

// Multi-threaded stack test for push operation
namespace stack {

#define TEST_CASE( Q, V ) void Q() { test< Types<V>::Q >(); }

    namespace {
        static size_t s_nThreadCount = 8  ;
        static size_t s_nStackSize = 10000000 ;

        struct SimpleValue {
            size_t      nNo ;
            size_t      nThread ;

            SimpleValue() {}
            SimpleValue( size_t n ): nNo(n) {}
            size_t getNo() const { return  nNo; }
        };
    }

    class Stack_Push_MT: public CppUnitMini::TestCase
    {
        template <class STACK>
        class Thread: public CppUnitMini::TestThread
        {
            virtual TestThread *    clone()
            {
                return new Thread( *this )  ;
            }
        public:
            STACK&              m_Stack         ;
            double              m_fTime         ;
            size_t              m_nStartItem    ;
            size_t              m_nEndItem      ;
            size_t              m_nPushError    ;

        public:
            Thread( CppUnitMini::ThreadPool& pool, STACK& s )
                : CppUnitMini::TestThread( pool )
                , m_Stack( s )
            {}
            Thread( Thread& src )
                : CppUnitMini::TestThread( src )
                , m_Stack( src.m_Stack )
            {}

            Stack_Push_MT&  getTest()
            {
                return reinterpret_cast<Stack_Push_MT&>( m_Pool.m_Test )   ;
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
                m_fTime = m_Timer.duration()    ;

                m_nPushError = 0    ;
                SimpleValue v   ;
                v.nThread = m_nThreadNo ;
                for ( v.nNo = m_nStartItem; v.nNo < m_nEndItem; ++v.nNo ) {
                    if ( !m_Stack.push( v ))
                        ++m_nPushError ;
                }

                m_fTime = m_Timer.duration() - m_fTime  ;
            }
        };

    protected:
        void setUpParams( const CppUnitMini::TestCfg& cfg ) {
            s_nThreadCount = cfg.getULong("ThreadCount", 8 ) ;
            s_nStackSize = cfg.getULong("StackSize", 10000000 );
        }

        template <class STACK>
        void analyze( CppUnitMini::ThreadPool& pool, STACK& testStack  )
        {
            size_t nThreadItems = s_nStackSize / s_nThreadCount ;
            std::vector<size_t> aThread ;
            aThread.resize(s_nThreadCount)  ;

            double fTime = 0    ;
            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it ) {
                Thread<STACK> * pThread = reinterpret_cast<Thread<STACK> *>(*it)  ;
                fTime += pThread->m_fTime   ;
                if ( pThread->m_nPushError != 0 )
                    CPPUNIT_MSG("     ERROR: thread push error count=" << pThread->m_nPushError ) ;
                aThread[ pThread->m_nThreadNo] = pThread->m_nEndItem - 1    ;
            }
            CPPUNIT_MSG( "     Duration=" << (fTime / s_nThreadCount) )   ;
            CPPUNIT_ASSERT( !testStack.empty() )

            size_t * arr = new size_t[ s_nStackSize ]           ;
            memset(arr, 0, sizeof(arr[0]) * s_nStackSize )      ;

            cds::OS::Timer      timer   ;
            CPPUNIT_MSG( "   Pop (single-threaded)..." )    ;
            size_t nPopped = 0      ;
            SimpleValue val         ;
            while ( testStack.pop( val )) {
                nPopped++   ;
                ++arr[ val.getNo() ]    ;
                CPPUNIT_ASSERT( val.nThread < s_nThreadCount)   ;
                CPPUNIT_ASSERT( aThread[val.nThread] == val.nNo )   ;
                aThread[val.nThread]--  ;
            }
            CPPUNIT_MSG( "     Duration=" << timer.duration() )     ;

            size_t nTotalItems = nThreadItems * s_nThreadCount      ;
            size_t nError = 0   ;
            for ( size_t i = 0; i < nTotalItems; ++i ) {
                if ( arr[i] != 1 ) {
                    CPPUNIT_MSG( "   ERROR: Item " << i << " has not been pushed" ) ;
                    CPPUNIT_ASSERT( ++nError > 10 ) ;
                }
            }

            delete [] arr ;
        }

        template <class STACK>
        void test()
        {
            STACK testStack ;

            CppUnitMini::ThreadPool pool( *this )   ;
            pool.add( new Thread<STACK>( pool, testStack ), s_nThreadCount )       ;

            size_t nStart = 0   ;
            size_t nThreadItemCount = s_nStackSize / s_nThreadCount ;
            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it ) {
                Thread<STACK> * pThread = reinterpret_cast<Thread<STACK> *>(*it)  ;
                pThread->m_nStartItem = nStart  ;
                nStart += nThreadItemCount      ;
                pThread->m_nEndItem = nStart    ;
            }

            CPPUNIT_MSG( "   Push test, thread count=" << s_nThreadCount
                << " items=" << (nThreadItemCount * s_nThreadCount)
                << "...")   ;
            pool.run()  ;

            analyze( pool, testStack )     ;
        }

    protected:
        TEST_CASE( Stack_HP,            SimpleValue )
        TEST_CASE( Stack_HP_yield,      SimpleValue )
        TEST_CASE( Stack_HP_pause,      SimpleValue )
#ifdef CDS_DWORD_CAS_SUPPORTED
        TEST_CASE( Stack_Tagged,        SimpleValue )
        TEST_CASE( Stack_Tagged_yield,  SimpleValue )
        TEST_CASE( Stack_Tagged_pause,  SimpleValue )
#endif

        CPPUNIT_TEST_SUITE(Stack_Push_MT)
            CPPUNIT_TEST(Stack_HP)              ;
            CPPUNIT_TEST(Stack_HP_yield)        ;
            CPPUNIT_TEST(Stack_HP_pause)        ;
#ifdef CDS_DWORD_CAS_SUPPORTED
            CPPUNIT_TEST(Stack_Tagged)          ;
            CPPUNIT_TEST(Stack_Tagged_yield)    ;
            CPPUNIT_TEST(Stack_Tagged_pause)    ;
#endif
        CPPUNIT_TEST_SUITE_END();
    };

} // namespace stack

CPPUNIT_TEST_SUITE_REGISTRATION(stack::Stack_Push_MT);
