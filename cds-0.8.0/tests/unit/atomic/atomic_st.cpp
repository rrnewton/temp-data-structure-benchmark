/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include <cds/atomic.h>

#include "cppunit/thread.h"

/*
    Single-threaded atomic operation test
*/

namespace atomic {

#define CDS_TEST_INIT_32BIT     0x0007FFF3
#define CDS_TEST_INIT_64BIT     0x00FE00FFFFFFFFF3LL

    namespace {
        static cds::atomic32_t c_nRepeat = 100000   ;

        static const cds::memory_order c_arrMemOrder[] = {
            cds::membar_relaxed::order,
            cds::membar_consume::order,
            cds::membar_acquire::order,
            cds::membar_release::order,
            cds::membar_acq_rel::order,
            cds::membar_seq_cst::order,
        };
        static const size_t c_moCount = sizeof(c_arrMemOrder) / sizeof(c_arrMemOrder[0]) ;

        static const cds::memory_order c_arrMemLoadOrder[] = {
            cds::membar_relaxed::order,
            cds::membar_consume::order,
            cds::membar_acquire::order,
            cds::membar_seq_cst::order,
        };
        static const size_t c_moLoadCount = sizeof(c_arrMemLoadOrder) / sizeof(c_arrMemLoadOrder[0]) ;

        static const cds::memory_order c_arrMemStoreOrder[] = {
            cds::membar_relaxed::order,
            cds::membar_release::order,
            cds::membar_seq_cst::order,
        };
        static const size_t c_moStoreCount = sizeof(c_arrMemStoreOrder) / sizeof(c_arrMemStoreOrder[0]) ;

        template <typename T, int SIZE> struct CDS_TEST_INIT_LONG ;
        template <typename T> struct CDS_TEST_INIT_LONG<T, 4> { static const T c_nInit=CDS_TEST_INIT_32BIT; };
        template <typename T> struct CDS_TEST_INIT_LONG<T, 8> { static const T c_nInit=CDS_TEST_INIT_64BIT; };

        
        
    }

    class Atomic_ST: public CppUnitMini::TestCase
    {
        cds::OS::Timer    m_timer    ;

    protected:
        void setUpParams( const CppUnitMini::TestCfg& cfg ) {
            c_nRepeat = cfg.getULong("iterCount", 100000 )          ; // iteration count
        }

    protected:
        template <typename T>
        void do_cas( T nStart = 0 )
        {
            cds::atomic<T>  nAtomic ;
            CPPUNIT_MSG( "    operand size=" << sizeof(T) << ", atomic size=" << sizeof(nAtomic) << ", initial=" << nStart ) ;

            T nEnd = nStart + c_nRepeat     ;

            m_timer.reset() ;
            for ( size_t so = 0; so < c_moCount; ++so ) {
                cds::memory_order success_order = c_arrMemOrder[so] ;
                for ( size_t fo = 0; fo < c_moCount; ++fo ) {
                    cds::memory_order failure_order = c_arrMemOrder[fo] ;

                    nAtomic.template store<cds::membar_release>( nStart ) ;
                    CPPUNIT_ASSERT_EX( nAtomic.template load<cds::membar_acquire>() == nStart, "nStart=" << nStart )    ;

                    T nCur ;
                    for ( T i = nStart; i < nEnd; i += 1 ) {
                        CPPUNIT_ASSERT_EX( nAtomic.cas( i, i + 1, success_order, failure_order), "i=" << i << ", success_order=" << success_order )  ;
                        CPPUNIT_ASSERT_EX( (nCur = nAtomic.template load<cds::membar_acquire>()) == i + 1, "i=" << i << " nCur=" << nCur )    ;
                    }
                    CPPUNIT_ASSERT( nAtomic.template load<cds::membar_acquire>() == nEnd )    ;
                }
            }
            CPPUNIT_MSG( "    Duration=" << m_timer.duration() ) ;
        }

        template <typename T>
        void do_vcas( T nStart = 0 )
        {
            cds::atomic<T>  nAtomic ;
            CPPUNIT_MSG( "    operand size=" << sizeof(T) << ", atomic size=" << sizeof(nAtomic) << ", initial=" << nStart ) ;

            T nEnd = nStart + c_nRepeat     ;

            m_timer.reset() ;
            for ( size_t so = 0; so < c_moCount; ++so ) {
                cds::memory_order success_order = c_arrMemOrder[so] ;
                for ( size_t fo = 0; fo < c_moCount; ++fo ) {
                    cds::memory_order failure_order = c_arrMemOrder[fo] ;

                    nAtomic.template store<cds::membar_release>( nStart ) ;
                    for ( T i = nStart; i < nEnd; i += 1 ) {
                        CPPUNIT_ASSERT( nAtomic.vcas( i, i + 1, success_order, failure_order) == i )  ;
                    }
                    CPPUNIT_ASSERT( nAtomic.template load<cds::membar_acquire>() == nEnd )    ;
                }
            }
            CPPUNIT_MSG( "    Duration=" << m_timer.duration() ) ;
        }

        template <typename T>
        void do_xchg( T nStart = 0 )
        {
            CPPUNIT_MSG( "    operand size=" << sizeof(T)) ;
            cds::atomic<T> nAtomic ;

            T nEnd = nStart + c_nRepeat     ;

            m_timer.reset() ;
            for ( size_t so = 0; so < c_moCount; ++so ) {
                cds::memory_order success_order = c_arrMemOrder[so] ;

                nAtomic.template store<cds::membar_release>( nStart ) ;
                for ( T i = nStart; i < nEnd; i += 1 ) {
                    CPPUNIT_ASSERT( nAtomic.xchg( i + 1, success_order ) == i )  ;
                }
                CPPUNIT_ASSERT( nAtomic.template load<cds::membar_acquire>() == nEnd )    ;
            }
            CPPUNIT_MSG( "    Duration=" << m_timer.duration() ) ;
        }

        template <typename T>
        void do_xadd( T nStart = 0 )
        {
            CPPUNIT_MSG( "    operand size=" << sizeof(T)) ;
            cds::atomic<T>  nAtomic ;

            T nEnd = nStart + c_nRepeat     ;

            m_timer.reset() ;
            for ( size_t so = 0; so < c_moCount; ++so ) {
                cds::memory_order success_order = c_arrMemOrder[so] ;

                nAtomic.template store<cds::membar_release>( nStart ) ;
                for ( T i = nStart; i < nEnd; i += 1 ) {
                    CPPUNIT_ASSERT( nAtomic.xadd( 1, success_order ) == i )  ;
                }
                CPPUNIT_ASSERT( nAtomic.template load<cds::membar_acquire>() == nEnd )    ;
            }
            CPPUNIT_MSG( "    Duration=" << m_timer.duration() ) ;
        }

        template <typename T>
        void do_inc( T nStart = 0 )
        {
            CPPUNIT_MSG( "    operand size=" << sizeof(T)) ;
            cds::atomic<T>  nAtomic ;

            T nEnd = nStart + c_nRepeat     ;

            m_timer.reset() ;
            for ( size_t so = 0; so < c_moCount; ++so ) {
                cds::memory_order success_order = c_arrMemOrder[so] ;

                nAtomic.template store<cds::membar_release>( nStart ) ;
                for ( T i = nStart; i < nEnd; i += 1 ) {
                    CPPUNIT_ASSERT( nAtomic.inc( success_order ) == i )  ;
                }
                CPPUNIT_ASSERT( nAtomic.template load<cds::membar_acquire>() == nEnd )    ;
            }
            CPPUNIT_MSG( "    Duration=" << m_timer.duration() ) ;
        }

        template <typename T>
        void do_dec( T nStart = 0 )
        {
            CPPUNIT_MSG( "    operand size=" << sizeof(T)) ;
            cds::atomic<T>  nAtomic ;

            T nEnd = nStart + c_nRepeat     ;

            m_timer.reset() ;
            for ( size_t so = 0; so < c_moCount; ++so ) {
                cds::memory_order success_order = c_arrMemOrder[so] ;

                nAtomic.template store<cds::membar_release>( nEnd ) ;
                for ( T i = nEnd; i > nStart; i -= 1 ) {
                    CPPUNIT_ASSERT( nAtomic.dec( success_order ) ==  i )  ;
                }
                CPPUNIT_ASSERT( nAtomic.template load<cds::membar_acquire>() == nStart )    ;
            }
            CPPUNIT_MSG( "    Duration=" << m_timer.duration() ) ;
        }

        template <typename T>
        void do_load_store( T nStart = 0 )
        {
            CPPUNIT_MSG( "    operand size=" << sizeof(T)) ;
            cds::atomic<T>  nAtomic ;

            T nEnd = nStart + c_nRepeat     ;

            m_timer.reset() ;
            for ( size_t soLoad = 0; soLoad < c_moLoadCount; ++soLoad ) {
                cds::memory_order moLoad = c_arrMemLoadOrder[soLoad] ;
                for ( size_t soStore = 0; soStore < c_moStoreCount; ++soStore ) {
                    cds::memory_order moStore = c_arrMemStoreOrder[soStore] ;

                    for ( T i = nStart; i < nEnd; i += 1 ) {
                        nAtomic.store( i, moStore )     ;
                        CDS_COMPILER_RW_BARRIER         ;
                        T cur = nAtomic.load( moLoad )  ;
                        CPPUNIT_ASSERT_EX( cur == i, "store=" << i << ", load=" << cur << ", order: load(" << moLoad << ") store(" << moStore << ")" )  ;
                    }
                    CPPUNIT_ASSERT( nAtomic.load( moLoad ) == nEnd - 1 )    ;
                }
            }
            CPPUNIT_MSG( "    Duration=" << m_timer.duration() ) ;
        }

    public:
        void test_cas()
        {
            CPPUNIT_MSG( "cas: " << c_nRepeat * cds::end_of_memory_order * 2 << " times" ) ;

            CPPUNIT_MSG( "  atomic32_t" )   ;
            do_cas<cds::atomic32_t>( CDS_TEST_INIT_32BIT )  ;
            CPPUNIT_MSG( "  int" )          ;
            do_cas<int>( CDS_TEST_INIT_32BIT )      ;
            CPPUNIT_MSG( "  unsigned int" ) ;
            do_cas<unsigned int>( CDS_TEST_INIT_32BIT )     ;
            CPPUNIT_MSG( "  long" )         ;
            do_cas<long>( CDS_TEST_INIT_LONG<long, sizeof(long)>::c_nInit )     ;
            CPPUNIT_MSG( "  unsigned long" );
            do_cas<unsigned long>( CDS_TEST_INIT_LONG<unsigned long, sizeof(unsigned long)>::c_nInit )    ;

            CPPUNIT_MSG( "  atomic64_t" )   ;
            do_cas<cds::atomic64_t>( CDS_TEST_INIT_64BIT )  ;
            CPPUNIT_MSG( "  long long" )    ;
            do_cas<long long>( CDS_TEST_INIT_64BIT )            ;
            CPPUNIT_MSG( "  unsigned long long" )   ;
            do_cas<unsigned long long>( CDS_TEST_INIT_64BIT )   ;
        }

        void test_vcas()
        {
            CPPUNIT_MSG( "vcas: " << c_nRepeat * cds::end_of_memory_order * 2 << " times" ) ;

            CPPUNIT_MSG( "  atomic32_t" )   ;
            do_vcas<cds::atomic32_t>( CDS_TEST_INIT_32BIT )  ;
            CPPUNIT_MSG( "  int" )          ;
            do_vcas<int>( CDS_TEST_INIT_32BIT )      ;
            CPPUNIT_MSG( "  unsigned int" ) ;
            do_vcas<unsigned int>( CDS_TEST_INIT_32BIT )     ;
            CPPUNIT_MSG( "  long" )         ;
            do_vcas<long>( CDS_TEST_INIT_32BIT )     ;
            CPPUNIT_MSG( "  unsigned long" );
            do_vcas<unsigned long>( CDS_TEST_INIT_32BIT )    ;

            CPPUNIT_MSG( "  atomic64_t" )   ;
            do_vcas<cds::atomic64_t>( CDS_TEST_INIT_64BIT )  ;
            CPPUNIT_MSG( "  long long" )    ;
            do_vcas<long long>( CDS_TEST_INIT_64BIT )            ;
            CPPUNIT_MSG( "  unsigned long long" )   ;
            do_vcas<unsigned long long>( CDS_TEST_INIT_64BIT )   ;
        }

        void test_xchg()
        {
            CPPUNIT_MSG( "xchg: " << c_nRepeat * cds::end_of_memory_order * 2 << " times" ) ;

            CPPUNIT_MSG( "  atomic32_t" )   ;
            do_xchg<cds::atomic32_t>( CDS_TEST_INIT_32BIT )  ;
            CPPUNIT_MSG( "  int" )          ;
            do_xchg<int>( CDS_TEST_INIT_32BIT )      ;
            CPPUNIT_MSG( "  unsigned int" ) ;
            do_xchg<unsigned int>( CDS_TEST_INIT_32BIT )     ;
            CPPUNIT_MSG( "  long" )         ;
            do_xchg<long>( CDS_TEST_INIT_32BIT )     ;
            CPPUNIT_MSG( "  unsigned long" );
            do_xchg<unsigned long>( CDS_TEST_INIT_32BIT )    ;

            CPPUNIT_MSG( "  atomic64_t" )   ;
            do_xchg<cds::atomic64_t>( CDS_TEST_INIT_64BIT )  ;
            CPPUNIT_MSG( "  long long" )    ;
            do_xchg<long long>( CDS_TEST_INIT_64BIT )            ;
            CPPUNIT_MSG( "  unsigned long long" )   ;
            do_xchg<unsigned long long>( CDS_TEST_INIT_64BIT )   ;
        }

        void test_xadd()
        {
            CPPUNIT_MSG( "xadd: " << c_nRepeat * cds::end_of_memory_order * 2 << " times" ) ;

            CPPUNIT_MSG( "  atomic32_t" )   ;
            do_xadd<cds::atomic32_t>( CDS_TEST_INIT_32BIT )  ;
            CPPUNIT_MSG( "  int" )          ;
            do_xadd<int>( CDS_TEST_INIT_32BIT )      ;
            CPPUNIT_MSG( "  unsigned int" ) ;
            do_xadd<unsigned int>( CDS_TEST_INIT_32BIT )     ;
            CPPUNIT_MSG( "  long" )         ;
            do_xadd<long>( CDS_TEST_INIT_32BIT )     ;
            CPPUNIT_MSG( "  unsigned long" );
            do_xadd<unsigned long>( CDS_TEST_INIT_32BIT )    ;

            CPPUNIT_MSG( "  atomic64_t" )   ;
            do_xadd<cds::atomic64_t>( CDS_TEST_INIT_64BIT )  ;
            CPPUNIT_MSG( "  long long" )    ;
            do_xadd<long long>( CDS_TEST_INIT_64BIT )            ;
            CPPUNIT_MSG( "  unsigned long long" )   ;
            do_xadd<unsigned long long>( CDS_TEST_INIT_64BIT )   ;
        }

        void test_inc()
        {
            CPPUNIT_MSG( "inc: " << c_nRepeat * cds::end_of_memory_order * 2 << " times" ) ;

            CPPUNIT_MSG( "  atomic32_t" )   ;
            do_inc<cds::atomic32_t>( CDS_TEST_INIT_32BIT )  ;
            CPPUNIT_MSG( "  int" )          ;
            do_inc<int>( CDS_TEST_INIT_32BIT )      ;
            CPPUNIT_MSG( "  unsigned int" ) ;
            do_inc<unsigned int>( CDS_TEST_INIT_32BIT )     ;
            CPPUNIT_MSG( "  long" )         ;
            do_inc<long>( CDS_TEST_INIT_32BIT )     ;
            CPPUNIT_MSG( "  unsigned long" );
            do_inc<unsigned long>( CDS_TEST_INIT_32BIT )    ;

            CPPUNIT_MSG( "  atomic64_t" )   ;
            do_inc<cds::atomic64_t>( CDS_TEST_INIT_64BIT )  ;
            CPPUNIT_MSG( "  long long" )    ;
            do_inc<long long>( CDS_TEST_INIT_64BIT )            ;
            CPPUNIT_MSG( "  unsigned long long" )   ;
            do_inc<unsigned long long>( CDS_TEST_INIT_64BIT )   ;
        }

        void test_dec()
        {
            CPPUNIT_MSG( "dec: " << c_nRepeat * cds::end_of_memory_order * 2 << " times" ) ;

            CPPUNIT_MSG( "  atomic32_t" )   ;
            do_dec<cds::atomic32_t>( CDS_TEST_INIT_32BIT )  ;
            CPPUNIT_MSG( "  int" )          ;
            do_dec<int>( CDS_TEST_INIT_32BIT )      ;
            CPPUNIT_MSG( "  unsigned int" ) ;
            do_dec<unsigned int>( CDS_TEST_INIT_32BIT )     ;
            CPPUNIT_MSG( "  long" )         ;
            do_dec<long>( CDS_TEST_INIT_32BIT )     ;
            CPPUNIT_MSG( "  unsigned long" );
            do_dec<unsigned long>( CDS_TEST_INIT_32BIT )    ;

            CPPUNIT_MSG( "  atomic64_t" )   ;
            do_dec<cds::atomic64_t>( CDS_TEST_INIT_64BIT )  ;
            CPPUNIT_MSG( "  long long" )    ;
            do_dec<long long>( CDS_TEST_INIT_64BIT )            ;
            CPPUNIT_MSG( "  unsigned long long" )   ;
            do_dec<unsigned long long>( CDS_TEST_INIT_64BIT )   ;
        }

        void test_load_store()
        {
            CPPUNIT_MSG( "load/store: " << c_nRepeat * cds::end_of_memory_order * 2 << " times" ) ;

            CPPUNIT_MSG( "  atomic32_t" )   ;
            do_load_store<cds::atomic32_t>( CDS_TEST_INIT_32BIT )   ;
            CPPUNIT_MSG( "  int" )          ;
            do_load_store<int>( CDS_TEST_INIT_32BIT )      ;
            CPPUNIT_MSG( "  unsigned int" ) ;
            do_load_store<unsigned int>( CDS_TEST_INIT_32BIT )      ;
            CPPUNIT_MSG( "  long" )         ;
            do_load_store<long>( CDS_TEST_INIT_32BIT )     ;
            CPPUNIT_MSG( "  unsigned long" );
            do_load_store<unsigned long>( CDS_TEST_INIT_32BIT )     ;

            CPPUNIT_MSG( "  atomic64_t" )   ;
            do_load_store<cds::atomic64_t>( CDS_TEST_INIT_64BIT )   ;
            CPPUNIT_MSG( "  long long" )    ;
            do_load_store<long long>( CDS_TEST_INIT_64BIT )         ;
            CPPUNIT_MSG( "  unsigned long long" )   ;
            do_load_store<unsigned long long>( CDS_TEST_INIT_64BIT );
        }

        void test_bool()
        {
            CPPUNIT_MSG( "atomic<bool>" ) ;

            for ( size_t soLoad = 0; soLoad < c_moLoadCount; ++soLoad ) {\
                cds::memory_order moLoad = c_arrMemLoadOrder[soLoad] ;
                for ( size_t soStore = 0; soStore < c_moStoreCount; ++soStore ) {
                    cds::memory_order moStore = c_arrMemStoreOrder[soStore] ;

                    cds::atomic<bool>  bAtomic1 ;
                    cds::atomic<bool>  bAtomic2( true )         ;

                    CPPUNIT_ASSERT( !bAtomic1.load(moLoad) )    ;
                    bAtomic1.store( false, moStore)     ;
                    CPPUNIT_ASSERT( !bAtomic1.load(moLoad) )    ;

                    CPPUNIT_ASSERT( bAtomic2.load(moLoad))      ;
                    bAtomic2.store( true, moStore)      ;
                    CPPUNIT_ASSERT( bAtomic2.load(moLoad) )     ;

                    bAtomic1.store( true, moStore)      ;
                    CPPUNIT_ASSERT( bAtomic1.load(moLoad) )     ;
                    bAtomic1.store( false, moStore)     ;
                    CPPUNIT_ASSERT( !bAtomic1.load(moLoad) )    ;

                    bAtomic2.store( false, moStore)     ;
                    CPPUNIT_ASSERT( !bAtomic2.load(moLoad) )    ;
                    bAtomic2.store( true, moStore)      ;
                    CPPUNIT_ASSERT( bAtomic2.load(moLoad) )     ;
                }
            }

            {
                cds::atomic<bool>  bAtomic1 ;
                CPPUNIT_ASSERT( !bAtomic1.exchange<cds::membar_relaxed>( true ))  ;
                CPPUNIT_ASSERT( bAtomic1.exchange<cds::membar_acquire>( false ))  ;
                CPPUNIT_ASSERT( !bAtomic1.exchange<cds::membar_release>( true ))   ;
                CPPUNIT_ASSERT( bAtomic1.exchange<cds::membar_acq_rel>( false  ))   ;
                CPPUNIT_ASSERT( !bAtomic1.exchange<cds::membar_seq_cst>( true  ))   ;
            }

            {
                cds::atomic<bool>  bAtomic ;
                CPPUNIT_ASSERT( !bAtomic.exchange( true, cds::membar_relaxed::order ))  ;
                CPPUNIT_ASSERT( bAtomic.exchange( false, cds::membar_acquire::order ))  ;
                CPPUNIT_ASSERT( !bAtomic.exchange( true, cds::membar_release::order ))   ;
                CPPUNIT_ASSERT( bAtomic.exchange( false, cds::membar_acq_rel::order ))   ;
                CPPUNIT_ASSERT( !bAtomic.exchange( true, cds::membar_seq_cst::order ))   ;
            }
        }

#if defined(CDS_DWORD_CAS_SUPPORTED) && CDS_BUILD_BITS==64
        void test_128bits()
        {
            CPPUNIT_MSG( "atomic<atomic128_t>" ) ;

            cds::atomic128_t    v1  ;
            v1.lo = CDS_TEST_INIT_64BIT ;
            v1.hi = CDS_TEST_INIT_64BIT - 0x0010101010101010LL ;
            //((cds::atomic64_t *) &v1)[0] = CDS_TEST_INIT_64BIT ;
            //((cds::atomic64_t *) &v1)[1] = CDS_TEST_INIT_64BIT - 0x0010101010101010LL ;

            cds::atomic128_t    n0  ;   // == 0
            n0.lo = n0.hi = 0       ;
            cds::atomic128_t    v2  ;

            m_timer.reset() ;
            for ( size_t so = 0; so < c_moCount; ++so ) {
                cds::memory_order success_order = c_arrMemOrder[so] ;

                for ( size_t i = 0; i < c_nRepeat / 3; i += 1 ) {
                    cds::atomic<cds::atomic128_t>  nAtomic ;

                    v1.lo -= i;
                    v1.hi -= i;
                    //((cds::atomic64_t *) &v1)[0] -= i   ;
                    //((cds::atomic64_t *) &v1)[1] -= i   ;

                    nAtomic.store<cds::membar_relaxed>( v1 ) ;
                    v2 = nAtomic.load<cds::membar_relaxed>() ;
                    CPPUNIT_ASSERT( v1.lo == v2.lo )  ;
                    CPPUNIT_ASSERT( v1.hi == v2.hi )  ;

                    nAtomic.store<cds::membar_release>( n0 ) ;
                    v2 = nAtomic.load<cds::membar_acquire>() ;
                    CPPUNIT_ASSERT( v2.lo == 0 )  ;
                    CPPUNIT_ASSERT( v2.hi == 0 )  ;

                    CPPUNIT_ASSERT_EX( nAtomic.cas( n0, v1, success_order, cds::membar_relaxed::order ),
                        ", n0.lo=" << n0.lo << " n0.hi=" << n0.hi <<
                        ", v1.lo=" << v1.lo << " v1.hi=" << v1.hi  )  ;
                    CPPUNIT_ASSERT_EX( nAtomic.cas( v1, n0, success_order, cds::membar_relaxed::order ), 
                        ", v1.lo=" << v1.lo << " v1.hi=" << v1.hi <<
                        ", n0.lo=" << n0.lo << " n0.hi=" << n0.hi )  ;
                    CPPUNIT_ASSERT( !nAtomic.cas( v1, n0, success_order, cds::membar_relaxed::order ))  ;

                    v2 = nAtomic.vcas( n0, v1, success_order, cds::membar_relaxed::order )   ;
                    CPPUNIT_ASSERT( v2.lo == 0 )  ;
                    CPPUNIT_ASSERT( v2.hi == 0 )  ;

                    cds::atomic128_t v3 = nAtomic.vcas( v1, n0, success_order, cds::membar_relaxed::order )   ;
                    CPPUNIT_ASSERT_EX( v1.lo == v3.lo, "v1.lo=" << v1.lo << " v3.lo" <<  v3.lo )  ;
                    CPPUNIT_ASSERT_EX( v1.hi == v3.hi, "v1.hi=" << v1.hi << " v3.hi" <<  v3.hi )  ;

                    // now nAtomic == 0

                    v2 = nAtomic.xchg( v1, success_order )   ;
                    CPPUNIT_ASSERT( v2.lo == 0 )  ;
                    CPPUNIT_ASSERT( v2.hi == 0 )  ;

                    v3 = nAtomic.xchg( n0, success_order )   ;
                    CPPUNIT_ASSERT( v1.lo == v3.lo )  ;
                    CPPUNIT_ASSERT( v1.hi == v3.hi )  ;
                }
            }
            CPPUNIT_MSG( "    Duration=" << m_timer.duration() ) ;
        }
#endif
        CPPUNIT_TEST_SUITE(Atomic_ST);
            CPPUNIT_TEST( test_cas )    ;
            CPPUNIT_TEST( test_vcas )   ;
            CPPUNIT_TEST( test_xchg )   ;
            CPPUNIT_TEST( test_xadd )   ;
            CPPUNIT_TEST( test_inc )    ;
            CPPUNIT_TEST( test_dec )    ;
            CPPUNIT_TEST( test_load_store ) ;
            CPPUNIT_TEST( test_bool )   ;
#if defined(CDS_DWORD_CAS_SUPPORTED) && CDS_BUILD_BITS==64
            CPPUNIT_TEST( test_128bits )   ;
#endif
        CPPUNIT_TEST_SUITE_END();

    };

}

CPPUNIT_TEST_SUITE_REGISTRATION(atomic::Atomic_ST);
