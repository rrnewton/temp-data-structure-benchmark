/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include "cppunit/cppunit_proxy.h"

#include <cds/bitop.h>
#include <cds/os/timer.h>

class bitop_ST : public CppUnitMini::TestCase
{
protected:

#if CDS_PROCESSOR_ARCH == CDS_PROCESSOR_SPARC
    void sparc_msb64_popc()
    {
        long long  n = 0   ;
/*
        CPPUNIT_ASSERT_EX( cds::bitop::platform::sparc_msb64( n ) == cds::bitop::MSB( n ),
            "n=" << std::hex << n << std::dec << ", sparc=" << cds::bitop::platform::sparc_msb64( n ) << ", common=" << cds::bitop::MSB( n ) ) ;
        n = 1   ;
        do {
            CPPUNIT_ASSERT_EX( cds::bitop::platform::sparc_msb64( n ) == cds::bitop::MSB( n ),
                "n=" << std::hex << n << std::dec << ", sparc=" << cds::bitop::platform::sparc_msb64( n ) << ", common=" << cds::bitop::MSB( n ) ) ;
            n = n + (n << 1)    ;
        } while ( !(n & (1ULL << 63)))           ;
*/

        cds::OS::Timer    timer    ;

        CPPUNIT_MSG( "generic MSB test..." ) ;
        n = 1   ;
        timer.reset()   ;
        for ( size_t i = 0; i < 1000000; ++i ) {
            volatile int res = cds::bitop::MSB( n )    ;
            n = n + (n << 1)        ;
        }
        CPPUNIT_MSG( "   Duration=" << timer.duration() ) ;

        CPPUNIT_MSG( "sparc_msb64 (popc) test..." ) ;
        n = 1   ;
        timer.reset()   ;

        for ( size_t i = 0; i < 1000000; ++i ) {
            volatile int res = cds::bitop::platform::sparc_msb64( n )  ;
            n = n + (n << 1)    ;
        }
        CPPUNIT_MSG( "   Duration=" << timer.duration() ) ;
    }
#endif // ifdef sparc

    void bitop32()
    {
        cds::atomic32u_t    n ;
        n = 0   ;
        CPPUNIT_ASSERT_EX( cds::bitop::MSB(n) == 0, "n=" << n ) ;
        CPPUNIT_ASSERT_EX( cds::bitop::LSB(n) == 0, "n=" << n ) ;
        CPPUNIT_ASSERT_EX( cds::bitop::SBC(n) == 0, "n=" << n ) ;
        CPPUNIT_ASSERT_EX( cds::bitop::ZBC(n) == sizeof(n) * 8, "n=" << n ) ;

        size_t nBit = 1 ;
        for ( n = 1; n != 0; n *= 2 ) {
            CPPUNIT_ASSERT_EX( cds::bitop::MSB(n) == nBit, "n=" << n )          ;
            CPPUNIT_ASSERT_EX( cds::bitop::LSB(n) == nBit, "n=" << n )          ;
            CPPUNIT_ASSERT_EX( cds::bitop::MSBnz(n) == nBit - 1, "n=" << n )    ;
            CPPUNIT_ASSERT_EX( cds::bitop::LSBnz(n) == nBit - 1, "n=" << n )    ;
            CPPUNIT_ASSERT_EX( cds::bitop::SBC(n) == 1, "n=" << n )             ;
            CPPUNIT_ASSERT_EX( cds::bitop::ZBC(n) == sizeof(n) * 8 - 1, "n=" << n ) ;

            ++nBit  ;
        }
    }

    void bitop64()
    {
        cds::atomic64u_t    n ;
        n = 0   ;
        CPPUNIT_ASSERT_EX( cds::bitop::MSB(n) == 0, "n=" << n ) ;
        CPPUNIT_ASSERT_EX( cds::bitop::LSB(n) == 0, "n=" << n ) ;
        CPPUNIT_ASSERT_EX( cds::bitop::SBC(n) == 0, "n=" << n ) ;
        CPPUNIT_ASSERT_EX( cds::bitop::ZBC(n) == sizeof(n) * 8, "n=" << n ) ;

        size_t nBit = 1 ;
        for ( n = 1; n != 0; n *= 2 ) {
            CPPUNIT_ASSERT_EX( cds::bitop::MSB(n) == nBit, "n=" << n )          ;
            CPPUNIT_ASSERT_EX( cds::bitop::LSB(n) == nBit, "n=" << n )          ;
            CPPUNIT_ASSERT_EX( cds::bitop::MSBnz(n) == nBit - 1, "n=" << n )    ;
            CPPUNIT_ASSERT_EX( cds::bitop::LSBnz(n) == nBit - 1, "n=" << n )    ;
            CPPUNIT_ASSERT_EX( cds::bitop::SBC(n) == 1, "n=" << n )             ;
            CPPUNIT_ASSERT_EX( cds::bitop::ZBC(n) == sizeof(n) * 8 - 1, "n=" << n ) ;

            ++nBit  ;
        }
    }

    CPPUNIT_TEST_SUITE(bitop_ST);
#if CDS_PROCESSOR_ARCH == CDS_PROCESSOR_SPARC
        CPPUNIT_TEST(sparc_msb64_popc) ;
#endif
        CPPUNIT_TEST(bitop32)
        CPPUNIT_TEST(bitop64)
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(bitop_ST)    ;
