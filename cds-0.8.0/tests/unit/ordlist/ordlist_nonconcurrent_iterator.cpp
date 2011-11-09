/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/



#include "ordlist/ordlist_nonconcurrent_iterator.h"
#include "ordlist/ordlist_types.h"

namespace ordlist {

#    define TEST_LIST(X)    void X() { test<OrdlistTypes<key_type, value_type>::X >()    ; }

    class OrdList_Nonconcurrent_iterator: public nonconcurrent_iterator::Test {
    protected:
        typedef nonconcurrent_iterator::Sequence::TKey        key_type    ;
        typedef nonconcurrent_iterator::Sequence::TValue    value_type    ;

        TEST_LIST(MichaelList_HP)    ;
        TEST_LIST(MichaelList_HRC)    ;
        TEST_LIST(MichaelList_PTB)    ;
#ifdef CDS_DWORD_CAS_SUPPORTED
        TEST_LIST(MichaelList_Tagged)   ;
#endif
        TEST_LIST(MichaelList_NoGC)    ;

        //TEST_LIST(HarrisList_HP)    ;
        //TEST_LIST(HarrisList_HRC)    ;

        TEST_LIST(LazyList_HP)        ;
        TEST_LIST(LazyList_HRC)        ;
        TEST_LIST(LazyList_PTB)        ;
        TEST_LIST(LazyList_NoGC)    ;

        CPPUNIT_TEST_SUITE( OrdList_Nonconcurrent_iterator )
            CPPUNIT_TEST( initTestSequence )

            CPPUNIT_TEST(MichaelList_HP)    ;
            CPPUNIT_TEST(MichaelList_HRC)    ;
            CPPUNIT_TEST(MichaelList_PTB)    ;
#ifdef CDS_DWORD_CAS_SUPPORTED
            CPPUNIT_TEST(MichaelList_Tagged);
#endif
            CPPUNIT_TEST(MichaelList_NoGC)    ;

            //CPPUNIT_TEST(HarrisList_HP)        ;
            //CPPUNIT_TEST(HarrisList_HRC)    ;

            CPPUNIT_TEST(LazyList_HP)        ;
            CPPUNIT_TEST(LazyList_HRC)        ;
            CPPUNIT_TEST(LazyList_PTB)        ;
            CPPUNIT_TEST(LazyList_NoGC)        ;

            CPPUNIT_TEST( cleanTestSequence )
        CPPUNIT_TEST_SUITE_END();
    } ;

    CPPUNIT_TEST_SUITE_REGISTRATION( OrdList_Nonconcurrent_iterator );

} // namespace ordlist

