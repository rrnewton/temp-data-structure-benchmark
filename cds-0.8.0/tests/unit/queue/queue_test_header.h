/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __UNIT_QUEUE_SIMPLE_H
#define __UNIT_QUEUE_SIMPLE_H

#include "cppunit/cppunit_proxy.h"
#include <cds/queue/details/queue_base.h>

namespace queue {
    struct QueueCountedTraits: public cds::queue::traits {
        typedef cds::atomics::item_counter<>  item_counter_type    ;
    };

    //
    // Test queue operation in single thread mode
    //
    class Queue_TestHeader : public CppUnitMini::TestCase
    {
    protected:
        template <class QUEUE>
        void testNoItemCounter()
        {
            QUEUE   q       ;
            int     it      ;
            int     nPrev   ;

            for ( size_t i = 0; i < 3; ++i ) {
                CPPUNIT_ASSERT( q.empty() )         ;
#ifndef _DEBUG
                CPPUNIT_ASSERT( q.size() == 0 )     ;
#endif
                CPPUNIT_ASSERT( q.enqueue( 1 ) )    ;
                CPPUNIT_ASSERT( !q.empty() )        ;
                CPPUNIT_ASSERT( q.push( 10 ) )      ;
                CPPUNIT_ASSERT( !q.empty() )        ;
#ifndef _DEBUG
                CPPUNIT_ASSERT( q.size() == 0 )     ;   // no queue's item counter!
#endif

                it = -1 ;
                CPPUNIT_ASSERT( q.pop( it ) )       ;
                CPPUNIT_ASSERT( it == 1 )          ;
                CPPUNIT_ASSERT( !q.empty() )        ;
                CPPUNIT_ASSERT( q.dequeue( it ) )   ;
                CPPUNIT_ASSERT( it == 10 )           ;
#ifndef _DEBUG
                CPPUNIT_ASSERT( q.size() == 0 )     ;
#endif
                CPPUNIT_ASSERT( q.empty() )         ;
                it += 2009                          ;
                nPrev = it                          ;
                CPPUNIT_ASSERT( !q.dequeue( it ) )  ;
                CPPUNIT_ASSERT( it == nPrev )       ;   // it must not be changed!
            }
        }

        template <class QUEUE_COUNTED>
        void testWithItemCounter()
        {
            QUEUE_COUNTED   q       ;
            int     it      ;
            int     nPrev   ;

            for ( size_t i = 0; i < 3; ++i ) {
                CPPUNIT_ASSERT( q.empty() )         ;
                CPPUNIT_ASSERT( q.size() == 0 )     ;
                CPPUNIT_ASSERT( q.enqueue( 1 ) )    ;
                CPPUNIT_ASSERT( q.size() == 1 )     ;
                CPPUNIT_ASSERT( !q.empty() )        ;
                CPPUNIT_ASSERT( q.push( 10 ) )      ;
                CPPUNIT_ASSERT( !q.empty() )        ;
                CPPUNIT_ASSERT( q.size() == 2 )     ;

                it = -1 ;
                CPPUNIT_ASSERT( q.pop( it ) )       ;
                CPPUNIT_ASSERT( it == 1 )           ;
                CPPUNIT_ASSERT( !q.empty() )        ;
                CPPUNIT_ASSERT( q.size() == 1 )     ;
                CPPUNIT_ASSERT( q.dequeue( it ) )   ;
                CPPUNIT_ASSERT( it == 10 )          ;
                CPPUNIT_ASSERT( q.size() == 0 )     ;
                CPPUNIT_ASSERT( q.empty() )         ;
                CPPUNIT_ASSERT( q.size() == 0 )     ;
                it += 2009                          ;
                nPrev = it                          ;
                CPPUNIT_ASSERT( !q.dequeue( it ) )  ;
                CPPUNIT_ASSERT( it == nPrev )       ;   // it must not be changed!

                CPPUNIT_ASSERT( q.empty() )         ;
                CPPUNIT_ASSERT( q.size() == 0 )     ;
            }
        }

    public:
        void MSQueue_HP()   ;
        void MSQueue_HP_Counted()   ;
        void MSQueue_HRC()  ;
        void MSQueue_HRC_Counted()  ;
        void MSQueue_PTB()  ;
        void MSQueue_PTB_Counted()  ;
        void MSQueue_tagged()  ;
        void MSQueue_tagged_Counted()  ;

        void MoirQueue_HP()   ;
        void MoirQueue_HP_Counted()   ;
        void MoirQueue_HRC()  ;
        void MoirQueue_HRC_Counted()  ;
        void MoirQueue_PTB()  ;
        void MoirQueue_PTB_Counted()  ;
        void MoirQueue_tagged()  ;
        void MoirQueue_tagged_Counted()  ;

        void LMSQueue_HP()      ;
        void LMSQueue_HP_Counted()      ;
        void LMSQueue_PTB()     ;
        void LMSQueue_PTB_Counted()     ;

        void TZCyclicQueue_()   ;
        void TZCyclicQueue_Counted()   ;

        void RWQueue_()   ;
        void RWQueue_Counted()   ;

        CPPUNIT_TEST_SUITE(Queue_TestHeader)
            //CPPUNIT_TEST(MSQueue_HP)                ;
            CPPUNIT_TEST(MSQueue_HP)                ;
            CPPUNIT_TEST(MSQueue_HP_Counted)        ;
            CPPUNIT_TEST(MSQueue_HRC)               ;
            CPPUNIT_TEST(MSQueue_HRC_Counted)       ;
            CPPUNIT_TEST(MSQueue_PTB)               ;
            CPPUNIT_TEST(MSQueue_PTB_Counted)       ;
            CPPUNIT_TEST(MSQueue_tagged)            ;
            CPPUNIT_TEST(MSQueue_tagged_Counted)    ;

            CPPUNIT_TEST(MoirQueue_HP)              ;
            CPPUNIT_TEST(MoirQueue_HP_Counted)      ;
            CPPUNIT_TEST(MoirQueue_HRC)             ;
            CPPUNIT_TEST(MoirQueue_HRC_Counted)     ;
            CPPUNIT_TEST(MoirQueue_PTB)             ;
            CPPUNIT_TEST(MoirQueue_PTB_Counted)     ;
            CPPUNIT_TEST(MoirQueue_tagged)          ;
            CPPUNIT_TEST(MoirQueue_tagged_Counted)  ;

            CPPUNIT_TEST(LMSQueue_HP)               ;
            CPPUNIT_TEST(LMSQueue_HP_Counted)       ;
            CPPUNIT_TEST(LMSQueue_PTB)              ;
            CPPUNIT_TEST(LMSQueue_PTB_Counted)      ;

            CPPUNIT_TEST(TZCyclicQueue_)            ;
            CPPUNIT_TEST(TZCyclicQueue_Counted)     ;

            CPPUNIT_TEST(RWQueue_)                  ;
            CPPUNIT_TEST(RWQueue_Counted)           ;
        CPPUNIT_TEST_SUITE_END();

    };
} // namespace queue

#endif // #ifndef __UNIT_QUEUE_SIMPLE_H
