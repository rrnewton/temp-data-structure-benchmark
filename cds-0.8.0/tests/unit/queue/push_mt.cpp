/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include "cppunit/thread.h"
#include "queue/queue_type.h"

// Multi-threaded queue test for push operation
namespace queue {

#define TEST_CASE( Q, V ) void Q() { test< Types<V>::Q >(); }
#define TEST_BOUNDED( Q, V ) void Q() { test_bounded< Types<V>::Q >(); }

    namespace ns_Queue_Push_MT {
        static size_t s_nThreadCount = 8  ;
        static size_t s_nQueueSize = 20000000 ;   // no more than 20 million records

        struct SimpleValue {
            size_t      nNo ;

            SimpleValue() {}
            SimpleValue( size_t n ): nNo(n) {}
            size_t getNo() const { return  nNo; }
        };
    }
    using namespace ns_Queue_Push_MT ;

    class Queue_Push_MT: public CppUnitMini::TestCase
    {
        template <class QUEUE>
        class Thread: public CppUnitMini::TestThread
        {
            virtual TestThread *    clone()
            {
                return new Thread( *this )  ;
            }
        public:
            QUEUE&              m_Queue         ;
            double              m_fTime         ;
            size_t              m_nStartItem    ;
            size_t              m_nEndItem      ;
            size_t              m_nPushError    ;

        public:
            Thread( CppUnitMini::ThreadPool& pool, QUEUE& q )
                : CppUnitMini::TestThread( pool )
                , m_Queue( q )
            {}
            Thread( Thread& src )
                : CppUnitMini::TestThread( src )
                , m_Queue( src.m_Queue )
            {}

            Queue_Push_MT&  getTest()
            {
                return reinterpret_cast<Queue_Push_MT&>( m_Pool.m_Test )   ;
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
                for ( size_t nItem = m_nStartItem; nItem < m_nEndItem; ++nItem ) {
                    if ( !m_Queue.push( nItem ))
                        ++m_nPushError ;
                }

                m_fTime = m_Timer.duration() - m_fTime  ;
            }
        };

    protected:
        template <class QUEUE>
        void analyze( CppUnitMini::ThreadPool& pool, QUEUE& testQueue  )
        {
            size_t nThreadItems = s_nQueueSize / s_nThreadCount ;
            double fTime = 0    ;
            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it ) {
                Thread<QUEUE> * pThread = reinterpret_cast<Thread<QUEUE> *>(*it)  ;
                fTime += pThread->m_fTime   ;
                if ( pThread->m_nPushError != 0 )
                    CPPUNIT_MSG("     ERROR: thread push error count=" << pThread->m_nPushError ) ;
            }
            CPPUNIT_MSG( "     Duration=" << (fTime / s_nThreadCount) )   ;
            CPPUNIT_ASSERT( !testQueue.empty() )

            size_t * arr = new size_t[ s_nQueueSize ]           ;
            memset(arr, 0, sizeof(arr[0]) * s_nQueueSize )      ;

            cds::OS::Timer      timer   ;
            CPPUNIT_MSG( "   Pop (single-threaded)..." )    ;
            size_t nPopped = 0      ;
            SimpleValue val = SimpleValue() ;
            while ( testQueue.pop( val )) {
                nPopped++   ;
                ++arr[ val.getNo() ]    ;
            }
            CPPUNIT_MSG( "     Duration=" << timer.duration() )     ;

            size_t nTotalItems = nThreadItems * s_nThreadCount      ;
            size_t nError = 0   ;
            for ( size_t i = 0; i < nTotalItems; ++i ) {
                if ( arr[i] != 1 ) {
                    CPPUNIT_MSG( "   ERROR: Item " << i << " has not been pushed" ) ;
                    CPPUNIT_ASSERT( ++nError <= 10 ) ;
                }
            }

            delete [] arr ;
        }

        template <class QUEUE>
        void test()
        {
            QUEUE testQueue ;

            CppUnitMini::ThreadPool pool( *this )   ;
            pool.add( new Thread<QUEUE>( pool, testQueue ), s_nThreadCount )       ;

            size_t nStart = 0   ;
            size_t nThreadItemCount = s_nQueueSize / s_nThreadCount ;
            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it ) {
                Thread<QUEUE> * pThread = reinterpret_cast<Thread<QUEUE> *>(*it)  ;
                pThread->m_nStartItem = nStart  ;
                nStart += nThreadItemCount      ;
                pThread->m_nEndItem = nStart    ;
            }

            CPPUNIT_MSG( "   Push test, thread count=" << s_nThreadCount << " ...")   ;
            pool.run()  ;

            analyze( pool, testQueue )     ;
        }

        template <class QUEUE>
        void test_bounded()
        {
            size_t nStart = 0   ;
            size_t nThreadItemCount = s_nQueueSize / s_nThreadCount ;

            QUEUE testQueue( (unsigned int) cds::beans::exp2Ceil(s_nQueueSize) + 1 ) ;

            CppUnitMini::ThreadPool pool( *this )   ;
            pool.add( new Thread<QUEUE>( pool, testQueue ), s_nThreadCount )       ;

            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it ) {
                Thread<QUEUE> * pThread = reinterpret_cast<Thread<QUEUE> *>(*it)  ;
                pThread->m_nStartItem = nStart  ;
                nStart += nThreadItemCount      ;
                pThread->m_nEndItem = nStart    ;
            }

            CPPUNIT_MSG( "   Push test, thread count=" << s_nThreadCount << " ...")   ;
            pool.run()  ;

            analyze( pool, testQueue )     ;
        }

        void setUpParams( const CppUnitMini::TestCfg& cfg ) {
            s_nThreadCount = cfg.getULong("ThreadCount", 8 ) ;
            s_nQueueSize = cfg.getULong("QueueSize", 20000000 );
        }

    protected:
        TEST_CASE( MoirQueue_HP, SimpleValue )
        TEST_CASE( MoirQueue_HRC, SimpleValue )
        TEST_CASE( MoirQueue_PTB, SimpleValue )
#ifdef CDS_DWORD_CAS_SUPPORTED
        TEST_CASE( MoirQueue_Tagged, SimpleValue )
#endif
        TEST_CASE( MSQueue_HP, SimpleValue  )
        TEST_CASE( MSQueue_HRC, SimpleValue )
        TEST_CASE( MSQueue_PTB, SimpleValue )
#ifdef CDS_DWORD_CAS_SUPPORTED
        TEST_CASE( MSQueue_Tagged, SimpleValue )
#endif
        TEST_CASE( LMSQueue_HP, SimpleValue  )
        TEST_CASE( LMSQueue_PTB, SimpleValue )

        TEST_CASE( MoirQueue_HP_Counted, SimpleValue )
        TEST_CASE( MoirQueue_HRC_Counted, SimpleValue )
        TEST_CASE( MoirQueue_PTB_Counted, SimpleValue )
#ifdef CDS_DWORD_CAS_SUPPORTED
        TEST_CASE( MoirQueue_Tagged_Counted, SimpleValue )
#endif
        TEST_CASE( MSQueue_HP_Counted, SimpleValue )
        TEST_CASE( MSQueue_HRC_Counted, SimpleValue )
        TEST_CASE( MSQueue_PTB_Counted, SimpleValue )
#ifdef CDS_DWORD_CAS_SUPPORTED
        TEST_CASE( MSQueue_Tagged_Counted, SimpleValue )
#endif
        TEST_CASE( LMSQueue_HP_Counted, SimpleValue )
        TEST_CASE( LMSQueue_PTB_Counted, SimpleValue)

        TEST_CASE( RWQueue_Spinlock, SimpleValue )
        TEST_CASE( RWQueue_Spinlock_Counted, SimpleValue )

        TEST_BOUNDED(TZCyclicQueue, SimpleValue)
        TEST_BOUNDED(TZCyclicQueue_Counted, SimpleValue)

        TEST_CASE( StdQueue_deque_Spinlock, SimpleValue )
        TEST_CASE( StdQueue_list_Spinlock, SimpleValue )
        TEST_CASE( StdQueue_deque_BoostMutex, SimpleValue )
        TEST_CASE( StdQueue_list_BoostMutex, SimpleValue )
#ifdef UNIT_LOCK_WIN_CS
        TEST_CASE( StdQueue_deque_WinCS, SimpleValue )
        TEST_CASE( StdQueue_list_WinCS, SimpleValue )
#endif

        TEST_CASE( HASQueue_Spinlock, SimpleValue )

        CPPUNIT_TEST_SUITE(Queue_Push_MT)
            CPPUNIT_TEST(MoirQueue_HP)              ;
            CPPUNIT_TEST(MoirQueue_HP_Counted)      ;
            CPPUNIT_TEST(MoirQueue_HRC)             ;
            CPPUNIT_TEST(MoirQueue_HRC_Counted)     ;
            CPPUNIT_TEST(MoirQueue_PTB)             ;
            CPPUNIT_TEST(MoirQueue_PTB_Counted)     ;

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
            CPPUNIT_TEST(LMSQueue_HP)                ;
            CPPUNIT_TEST(LMSQueue_HP_Counted)        ;
            CPPUNIT_TEST(LMSQueue_PTB)               ;
            CPPUNIT_TEST(LMSQueue_PTB_Counted)       ;

            CPPUNIT_TEST(RWQueue_Spinlock)          ;
            CPPUNIT_TEST(RWQueue_Spinlock_Counted)  ;

            CPPUNIT_TEST(TZCyclicQueue)             ;
            CPPUNIT_TEST(TZCyclicQueue_Counted)     ;

            CPPUNIT_TEST(StdQueue_deque_Spinlock)   ;
            CPPUNIT_TEST(StdQueue_list_Spinlock)    ;
            CPPUNIT_TEST(StdQueue_deque_BoostMutex) ;
            CPPUNIT_TEST(StdQueue_list_BoostMutex)  ;
#ifdef UNIT_LOCK_WIN_CS
            CPPUNIT_TEST(StdQueue_deque_WinCS) ;
            CPPUNIT_TEST(StdQueue_list_WinCS)  ;
#endif
            CPPUNIT_TEST( HASQueue_Spinlock )       ;
        CPPUNIT_TEST_SUITE_END();
    };

} // namespace queue

CPPUNIT_TEST_SUITE_REGISTRATION(queue::Queue_Push_MT);
