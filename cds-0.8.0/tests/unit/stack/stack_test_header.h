/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __UNIT_STACK_TEST_HEADER_H
#define __UNIT_STACK_TEST_HEADER_H

#include "cppunit/cppunit_proxy.h"

namespace stack {

    //
    // Test map operation in single thread mode
    //
    class StackTestHeader : public CppUnitMini::TestCase
    {
    protected:
        static int const s_nItemCount = 100 ;
//        typedef CppUnitMini::TestCase       Base    ;

    protected:
        template <class STACK>
        void test()
        {
            STACK stack ;

            CPPUNIT_ASSERT( stack.empty() ) ;

            CPPUNIT_ASSERT( stack.push(1))  ;
            CPPUNIT_ASSERT( !stack.empty() ) ;
            CPPUNIT_ASSERT( stack.push(2))  ;
            CPPUNIT_ASSERT( !stack.empty() ) ;
            CPPUNIT_ASSERT( stack.push(3))  ;
            CPPUNIT_ASSERT( !stack.empty() ) ;

            int n   ;
            CPPUNIT_ASSERT( stack.pop(n) )  ;
            CPPUNIT_ASSERT( n == 3 )    ;
            CPPUNIT_ASSERT( !stack.empty() ) ;
            CPPUNIT_ASSERT( stack.pop(n) )  ;
            CPPUNIT_ASSERT( n == 2 )    ;
            CPPUNIT_ASSERT( !stack.empty() ) ;
            CPPUNIT_ASSERT( stack.pop(n) )  ;
            CPPUNIT_ASSERT( n == 1 )    ;
            CPPUNIT_ASSERT( stack.empty() ) ;
            n = 1000    ;
            CPPUNIT_ASSERT( !stack.pop(n) )  ;
            CPPUNIT_ASSERT( stack.empty() ) ;
        }

    protected:
        void Stack_tagged() ;
        void Stack_hp()    ;

    CPPUNIT_TEST_SUITE(StackTestHeader);
#ifdef CDS_DWORD_CAS_SUPPORTED
        CPPUNIT_TEST(Stack_tagged)                   ;
#endif
        CPPUNIT_TEST(Stack_hp)                   ;
    CPPUNIT_TEST_SUITE_END();

    } ;
} // namespace stack

#endif // #ifndef __UNIT_STACK_TEST_HEADER_H
