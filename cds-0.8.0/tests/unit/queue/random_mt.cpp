/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include "cppunit/thread.h"
#include "queue/queue_type.h"

#include <vector>
#include <boost/type_traits/is_base_of.hpp>

// Multi-threaded queue test for random push/pop operation
namespace queue {

#define TEST_CASE( Q, V ) void Q() { test< Types<V>::Q >(); }

    namespace ns_Queue_Random_MT {
        static size_t s_nThreadCount = 16         ;
        static size_t s_nQueueSize = 10000000     ;

        struct SimpleValue {
            size_t      nNo ;
            size_t      nThread ;

            SimpleValue() {}
            SimpleValue( size_t n ): nNo(n) {}
            size_t getNo() const { return  nNo; }
        };
    }

    using namespace ns_Queue_Random_MT    ;

    class Queue_Random_MT: public CppUnitMini::TestCase
    {
        typedef CppUnitMini::TestCase base_class ;

        template <class QUEUE>
        class Thread: public CppUnitMini::TestThread
        {
            virtual TestThread *    clone()
            {
                return new Thread( *this )  ;
            }
        public:
            QUEUE&              m_Queue ;
            double              m_fTime ;

            size_t  m_nPushCount    ;    // число операций push
            size_t  m_nPopCount        ;    // число операций pop
            size_t  m_nEmptyPop        ;    // число пустых операций pop (ничего не смогли взять из очереди)

            size_t  m_nUndefWriter    ;    // число ошибок "Неизвестный writer"
            size_t  m_nRepeatValue    ;    // число ошибок "Такое значение уже было для данного writer'а"
            size_t  m_nPushError    ;    // push error count

            std::vector<size_t> m_arrLastRead ;   // массив последних прочитанных значений для каждого thread
            std::vector<size_t> m_arrPopCountPerThread ;

        public:
            Thread( CppUnitMini::ThreadPool& pool, QUEUE& q )
                : CppUnitMini::TestThread( pool )
                , m_Queue( q )
            {}
            Thread( Thread& src )
                : CppUnitMini::TestThread( src )
                , m_Queue( src.m_Queue )
            {}

            Queue_Random_MT&  getTest()
            {
                return reinterpret_cast<Queue_Random_MT&>( m_Pool.m_Test )   ;
            }

            virtual void init()
            {
                cds::threading::Manager::attachThread()     ;
                m_nPushCount =
                    m_nPopCount =
                    m_nEmptyPop =
                    m_nUndefWriter =
                    m_nRepeatValue =
                    m_nPushError = 0    ;

                m_arrLastRead.resize( s_nThreadCount, 0 )   ;
                m_arrPopCountPerThread.resize( s_nThreadCount, 0 ) ;
            }
            virtual void fini()
            {
                cds::threading::Manager::detachThread()   ;
            }

            virtual void test()
            {
                size_t const nThreadCount = s_nThreadCount    ;
                size_t const nTotalPush = getTest().m_nThreadPushCount  ;

                SimpleValue node    ;

                m_fTime = m_Timer.duration()        ;

                bool bNextPop = false ;
                while ( m_nPushCount < nTotalPush ) {
                    if ( !bNextPop && (rand() & 3) != 3 ) {
                        // push
                        node.nThread = m_nThreadNo        ;
                        node.nNo = ++m_nPushCount        ;
                        if ( !m_Queue.push( node )) {
                            ++m_nPushError    ;
                            --m_nPushCount    ;
                        }

                    }
                    else {
                        // pop
                        pop( nThreadCount )        ;
                        bNextPop = false        ;
                    }
                }

                // Дочитываем очередь до конца
                size_t nPopLoop = 0    ;
                while ( !m_Queue.empty() && nPopLoop < 1000000 ) {
                    if ( pop( nThreadCount ) )
                        nPopLoop = 0    ;
                    else
                        ++nPopLoop        ;
                }


                m_fTime = m_Timer.duration() - m_fTime  ;
            }

            bool pop( size_t nThreadCount )
            {
                SimpleValue node ;
                node.nThread = -1    ;
                node.nNo = -1        ;
                if ( m_Queue.pop( node )) {
                    ++m_nPopCount    ;
                    if ( node.nThread < nThreadCount ) {
                        m_arrPopCountPerThread[ node.nThread ] += 1    ;
                        if ( m_arrLastRead[ node.nThread ] < node.nNo ) {
                            m_arrLastRead[ node.nThread ] = node.nNo    ;
                        }
                        else
                            ++m_nRepeatValue    ;

                        //if ( node.nNo < m_Test.m_nPushCount )
                        //    m_Test.m_pRead[ node.nWriter ][ node.nNo ] = node.nNo    ;
                    }
                    else {
                        ++m_nUndefWriter        ;
                    }
                }
                else {
                    ++m_nEmptyPop    ;
                    return false    ;
                }
                return true ;
            }
        };

    protected:
        size_t  m_nThreadPushCount  ;

    protected:
        template <class QUEUE>
        void analyze( CppUnitMini::ThreadPool& pool, QUEUE& testQueue  )
        {
            CPPUNIT_ASSERT( testQueue.empty() ) ;

            std::vector< size_t > arrPushCount  ;
            arrPushCount.resize( s_nThreadCount, 0 ) ;

            size_t nPushTotal = 0   ;
            size_t nPopTotal  = 0   ;
            double fTime = 0    ;

            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it ) {
                Thread<QUEUE> * pThread = static_cast<Thread<QUEUE> *>( *it ) ;
                CPPUNIT_ASSERT( pThread->m_nUndefWriter == 0 )  ;
                CPPUNIT_ASSERT( pThread->m_nRepeatValue == 0 )  ;
                if ( !boost::is_base_of<cds::concept::bounded_container, QUEUE>::value )
                    CPPUNIT_ASSERT( pThread->m_nPushError == 0 )    ;

                arrPushCount[ pThread->m_nThreadNo ] += pThread->m_nPushCount   ;

                nPushTotal += pThread->m_nPushCount ;
                nPopTotal += pThread->m_nPopCount   ;
                fTime += pThread->m_fTime           ;
            }

            CPPUNIT_MSG( "     Duration=" << (fTime /= s_nThreadCount) )   ;

            size_t nTotalItems = m_nThreadPushCount * s_nThreadCount    ;

            CPPUNIT_ASSERT( nPushTotal == nTotalItems ) ;
            CPPUNIT_ASSERT( nPopTotal == nTotalItems ) ;

            for ( size_t i = 0; i < s_nThreadCount; ++i )
                CPPUNIT_ASSERT( arrPushCount[i] == m_nThreadPushCount ) ;
        }

        template <class QUEUE>
        void test()
        {
            CPPUNIT_MSG( "Random push/pop test\n    thread count=" << s_nThreadCount << ", push count=" << s_nQueueSize << " ..." )   ;

            m_nThreadPushCount = s_nQueueSize / s_nThreadCount  ;

            QUEUE testQueue ;
            CppUnitMini::ThreadPool pool( *this )   ;
            pool.add( new Thread<QUEUE>( pool, testQueue ), s_nThreadCount )       ;

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

        TEST_CASE( TZCyclicQueue, SimpleValue )
        TEST_CASE( TZCyclicQueue_Counted, SimpleValue )

        TEST_CASE( StdQueue_deque_Spinlock, SimpleValue )
        TEST_CASE( StdQueue_list_Spinlock, SimpleValue )
        TEST_CASE( StdQueue_deque_BoostMutex, SimpleValue )
        TEST_CASE( StdQueue_list_BoostMutex, SimpleValue )
#ifdef UNIT_LOCK_WIN_CS
        TEST_CASE( StdQueue_deque_WinCS, SimpleValue )
        TEST_CASE( StdQueue_list_WinCS, SimpleValue )
#endif
        TEST_CASE( HASQueue_Spinlock, SimpleValue )

        CPPUNIT_TEST_SUITE(Queue_Random_MT)
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
            CPPUNIT_TEST(LMSQueue_HP)               ;
            CPPUNIT_TEST(LMSQueue_HP_Counted)       ;
            CPPUNIT_TEST(LMSQueue_PTB)              ;
            CPPUNIT_TEST(LMSQueue_PTB_Counted)      ;

            //CPPUNIT_TEST( HASQueue_Spinlock )       ;

            CPPUNIT_TEST(TZCyclicQueue)          ;
            CPPUNIT_TEST(TZCyclicQueue_Counted)  ;

            CPPUNIT_TEST(RWQueue_Spinlock)          ;
            CPPUNIT_TEST(RWQueue_Spinlock_Counted)  ;

            CPPUNIT_TEST(StdQueue_deque_Spinlock)   ;
            CPPUNIT_TEST(StdQueue_list_Spinlock)    ;
            CPPUNIT_TEST(StdQueue_deque_BoostMutex) ;
            CPPUNIT_TEST(StdQueue_list_BoostMutex)  ;
#ifdef UNIT_LOCK_WIN_CS
            CPPUNIT_TEST(StdQueue_deque_WinCS) ;
            CPPUNIT_TEST(StdQueue_list_WinCS)  ;
#endif
        CPPUNIT_TEST_SUITE_END();
    };

} // namespace queue

CPPUNIT_TEST_SUITE_REGISTRATION(queue::Queue_Random_MT);
