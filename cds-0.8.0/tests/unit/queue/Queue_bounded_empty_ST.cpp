/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include "cppunit/thread.h"
#include "queue/queue_type.h"

// Bounded queue tests
namespace queue {
#define TEST_CASE( Q, V ) void Q() { test< Types<V>::Q >(); }

    namespace {
        static size_t s_nPassCount = 100  ;
        static size_t s_nQueueSize = 256 ;   // no more than 20 million records

        struct SimpleValue {
            size_t      nNo ;

            SimpleValue() {}
            SimpleValue( size_t n ): nNo(n) {}
            size_t getNo() const { return  nNo; }
        };
    }

    class Queue_bounded_empty_ST: public CppUnitMini::TestCase
    {
        template <class Q>
        void test()
        {
            const size_t nLog2 = cds::beans::exp2Ceil( s_nQueueSize )   ;
            const size_t nSize = 1 << nLog2 ;
            Q   queue( (unsigned int) nLog2) ;

            CPPUNIT_MSG( "  queue.empty(), queue size=" << nSize << ", pass count=" << s_nPassCount ) ;

            for ( size_t nPass = 0; nPass < s_nPassCount; ++nPass ) {
                CPPUNIT_ASSERT_EX( queue.empty(), "start pass=" << nPass )    ;
                for ( size_t i = 0; i < queue.capacity(); ++i ) {
                    CPPUNIT_ASSERT_EX( queue.push(i), "item=" << i )    ;                    
                }
                CPPUNIT_ASSERT_EX( !queue.empty(), "pass=" << nPass )   ;

                // try to push to full queue
                CPPUNIT_ASSERT_EX( !queue.push(nSize + 1), "push to full queue" )   ;
                CPPUNIT_ASSERT_EX( !queue.empty(), "check emptiness of full queue" )   ;

                size_t nItem    ;
                for ( size_t i = 0; i < queue.capacity(); ++i ) {
                    CPPUNIT_ASSERT_EX( queue.pop(nItem), "item=" << i ) ;
                    CPPUNIT_ASSERT_EX( nItem == i, "item=" << i << ", popped=" << nItem )    ;
                }
                CPPUNIT_ASSERT_EX( queue.empty(), "before end pass=" << nPass )    ;

                // Push/pop to offset head/tail position in queue
                CPPUNIT_ASSERT_EX( queue.push(nSize), "head/tail offset" )  ;
                CPPUNIT_ASSERT_EX( !queue.empty(), "head/tail offset" )    ;
                CPPUNIT_ASSERT_EX( queue.pop(nItem), "head/tail offset" )   ;
                CPPUNIT_ASSERT_EX( nItem == nSize, "item=" << nSize << ", popped=" << nItem )    ;

                CPPUNIT_ASSERT_EX( queue.empty(), "end pass=" << nPass )    ;
            }
        }

        void setUpParams( const CppUnitMini::TestCfg& cfg ) {
            s_nPassCount = cfg.getULong("PassCount", (unsigned long) s_nPassCount ) ;
            s_nQueueSize = cfg.getULong("QueueSize", (unsigned long) s_nQueueSize ) ;
        }


        TEST_CASE( TZCyclicQueue, size_t )
        TEST_CASE( TZCyclicQueue_Counted, size_t )

        CPPUNIT_TEST_SUITE(Queue_bounded_empty_ST)
            CPPUNIT_TEST( TZCyclicQueue )      
            CPPUNIT_TEST( TZCyclicQueue_Counted )      
        CPPUNIT_TEST_SUITE_END();
    };
}   // namespace queue

CPPUNIT_TEST_SUITE_REGISTRATION(queue::Queue_bounded_empty_ST);