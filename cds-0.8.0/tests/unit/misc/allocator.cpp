/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include <cds/details/allocator.h>

#include "cppunit/cppunit_proxy.h"

static size_t const nArraySize = 10  ;
static size_t nConstructCount = 0   ;
static size_t nDestructCount = 0    ;

class Allocator_test : public CppUnitMini::TestCase
{
    struct SimpleStruct
    {
        int     n   ;
        SimpleStruct()
        {
            ++nConstructCount   ;
        }

        ~SimpleStruct()
        {
            ++nDestructCount    ;
        }
    };

    protected:
        void testArray()
        {
            SimpleStruct * pArr        ;
            cds::details::Allocator<SimpleStruct>  a       ;
            pArr = a.NewArray( nArraySize )  ;
            a.Delete( pArr, nArraySize )     ;

            CPPUNIT_ASSERT( nConstructCount == nArraySize ) ;
            CPPUNIT_ASSERT( nConstructCount == nDestructCount ) ;
        }

    CPPUNIT_TEST_SUITE(Allocator_test);
        CPPUNIT_TEST(testArray) ;
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(Allocator_test)    ;


