/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include "map/map_types.h"
#include "cppunit/thread.h"

//#include <vector>
#include <cds/os/topology.h>

namespace map {

#    define TEST_MAP(X)    void X() { test<MapTypes<key_type, value_type>::X >()    ; }

    namespace {
        static size_t  c_nMapSize = 1000000    ;  // map size
        static size_t  c_nThreadCount = 4      ;  // count of insertion thread
        static size_t  c_nMaxLoadFactor = 8    ;  // maximum load factor
        static bool    c_bPrintGCState = true  ;
    }

    class Map_InsFind_int_MT: public CppUnitMini::TestCase
    {
        typedef size_t  key_type    ;
        typedef size_t  value_type  ;

        template <class MAP>
        class Inserter: public CppUnitMini::TestThread
        {
            MAP&     m_Map      ;

            virtual Inserter *    clone()
            {
                return new Inserter( *this )    ;
            }
        public:
            size_t  m_nInsertSuccess    ;
            size_t  m_nInsertFailed     ;
            size_t  m_nFindSuccess      ;
            size_t  m_nFindFail         ;

        public:
            Inserter( CppUnitMini::ThreadPool& pool, MAP& rMap )
                : CppUnitMini::TestThread( pool )
                , m_Map( rMap )
            {}
            Inserter( Inserter& src )
                : CppUnitMini::TestThread( src )
                , m_Map( src.m_Map )
            {}

            Map_InsFind_int_MT&  getTest()
            {
                return reinterpret_cast<Map_InsFind_int_MT&>( m_Pool.m_Test )   ;
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            virtual void test()
            {
                MAP& rMap = m_Map   ;

                m_nInsertSuccess =
                    m_nInsertFailed = 
                    m_nFindSuccess = 
                    m_nFindFail = 0 ;

                size_t nInc = c_nThreadCount    ;
                for ( size_t nItem = m_nThreadNo; rMap.size() < c_nMapSize; nItem += nInc ) {
                    if ( rMap.insert( nItem, nItem * 8 ) )
                        ++m_nInsertSuccess  ;
                    else
                        ++m_nInsertFailed   ;

                    for ( size_t nFind = m_nThreadNo; nFind <= nItem; nFind += nInc ) {
                        if ( rMap.find( nFind) )
                            ++m_nFindSuccess    ;
                        else
                            ++m_nFindFail       ;
                    }
                }
            }
        };

    protected:

        template <class MAP>
        void do_test( size_t nLoadFactor )
        {
            typedef Inserter<MAP>       InserterThread  ;
            MAP  testMap( c_nMapSize, nLoadFactor ) ;
            cds::OS::Timer    timer    ;

            CPPUNIT_MSG( "Load factor=" << nLoadFactor )   ;

            CppUnitMini::ThreadPool pool( *this )   ;
            pool.add( new InserterThread( pool, testMap ), c_nThreadCount ) ;
            pool.run()  ;
            CPPUNIT_MSG( "   Duration=" << pool.avgDuration() ) ;

            size_t nInsertSuccess = 0   ;
            size_t nInsertFailed = 0    ;
            size_t nFindSuccess = 0     ;
            size_t nFindFailed = 0      ;
            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it ) {
                InserterThread * pThread = static_cast<InserterThread *>( *it )   ;

                nInsertSuccess += pThread->m_nInsertSuccess ;
                nInsertFailed += pThread->m_nInsertFailed   ;
                nFindSuccess += pThread->m_nFindSuccess     ;
                nFindFailed += pThread->m_nFindFail         ;
            }

            CPPUNIT_MSG( "    Totals: Ins succ=" << nInsertSuccess << " fail=" << nInsertFailed << "\n"
                      << "           Find succ=" << nFindSuccess << " fail=" << nFindFailed
            ) ;

            CPPUNIT_ASSERT( nInsertFailed == 0 )    ;
            CPPUNIT_ASSERT( nFindFailed == 0 )      ;
        }

        template <class MAP>
        void test()
        {
            CPPUNIT_MSG( "Thread count: " << c_nThreadCount
                << " map size=" << c_nMapSize
                );

            for ( size_t nLoadFactor = 1; nLoadFactor <= c_nMaxLoadFactor; nLoadFactor *= 2 ) {
                do_test<MAP>( nLoadFactor )     ;
                if ( c_bPrintGCState )
                    print_gc_state()            ;
            }

        }

        void setUpParams( const CppUnitMini::TestCfg& cfg ) {
            c_nThreadCount = cfg.getULong("ThreadCount", 0 )            ;
            c_nMapSize = cfg.getULong("MapSize", 10000 )                ;
            c_nMaxLoadFactor = cfg.getULong("MaxLoadFactor", 8 )        ;
            c_bPrintGCState = cfg.getBool("PrintGCStateFlag", true )    ;
            if ( c_nThreadCount == 0 )
                c_nThreadCount = cds::OS::topology::processor_count()   ;
        }


        TEST_MAP(SplitListDyn_Michael_HP)        ;
        TEST_MAP(SplitListStatic_Michael_HP)    ;
        TEST_MAP(SplitListDyn_Michael_HRC)        ;
        TEST_MAP(SplitListStatic_Michael_HRC)    ;
        TEST_MAP(SplitListDyn_Michael_PTB)        ;
        TEST_MAP(SplitListStatic_Michael_PTB)    ;
#ifdef CDS_DWORD_CAS_SUPPORTED
        TEST_MAP(SplitListDyn_Michael_Tagged)    ;
        TEST_MAP(SplitListStatic_Michael_Tagged);
#endif
        TEST_MAP(SplitListDyn_Michael_NoGC)         ;
        TEST_MAP(SplitListStatic_Michael_NoGC)      ;
        //TEST_MAP(SplitListDyn_Harris_HP)        ;
        //TEST_MAP(SplitListStatic_Harris_HP)        ;
        //TEST_MAP(SplitListDyn_Harris_HRC)        ;
        //TEST_MAP(SplitListStatic_Harris_HRC)    ;

        TEST_MAP(SplitListDyn_Lazy_HP)            ;
        TEST_MAP(SplitListStatic_Lazy_HP)        ;
        TEST_MAP(SplitListDyn_Lazy_HRC)            ;
        TEST_MAP(SplitListStatic_Lazy_HRC)        ;
        TEST_MAP(SplitListDyn_Lazy_PTB)            ;
        TEST_MAP(SplitListStatic_Lazy_PTB)        ;
        TEST_MAP(SplitListDyn_Lazy_NoGC)            ;
        TEST_MAP(SplitListStatic_Lazy_NoGC)        ;

        TEST_MAP(MichaelHashMap_Michael_HP)
        TEST_MAP(MichaelHashMap_Michael_HRC)
        TEST_MAP(MichaelHashMap_Michael_PTB)
        TEST_MAP(MichaelHashMap_Michael_NoGC)
#ifdef CDS_DWORD_CAS_SUPPORTED
        TEST_MAP(MichaelHashMap_Michael_Tagged)
        TEST_MAP(MichaelHashMap_Michael_TaggedShared)
#endif
            //TEST_MAP(MichaelHashMap_Harris_HP)
            //TEST_MAP(MichaelHashMap_Harris_HRC)

        TEST_MAP(MichaelHashMap_Lazy_HP)
        TEST_MAP(MichaelHashMap_Lazy_HRC)
        TEST_MAP(MichaelHashMap_Lazy_PTB)
        TEST_MAP(MichaelHashMap_Lazy_NoGC)

        TEST_MAP(StdMap_Spin)    ;
        TEST_MAP(StdHashMap_Spin)    ;
#ifdef WIN32
        TEST_MAP(StdMap_WinCS)    ;
        TEST_MAP(StdHashMap_WinCS)    ;
#endif

        CPPUNIT_TEST_SUITE( Map_InsFind_int_MT )
            CPPUNIT_TEST( SplitListDyn_Michael_HP       )
            CPPUNIT_TEST( SplitListStatic_Michael_HP    )
            CPPUNIT_TEST( SplitListDyn_Michael_HRC      )
            CPPUNIT_TEST( SplitListStatic_Michael_HRC   )
            CPPUNIT_TEST( SplitListDyn_Michael_PTB      )
            CPPUNIT_TEST( SplitListStatic_Michael_PTB   )
#ifdef CDS_DWORD_CAS_SUPPORTED
            CPPUNIT_TEST( SplitListDyn_Michael_Tagged   )
            CPPUNIT_TEST( SplitListStatic_Michael_Tagged)
#endif
            CPPUNIT_TEST( SplitListDyn_Michael_NoGC     )
            CPPUNIT_TEST( SplitListStatic_Michael_NoGC  )
            CPPUNIT_TEST( SplitListDyn_Lazy_HP          )
            CPPUNIT_TEST( SplitListStatic_Lazy_HP       )
            CPPUNIT_TEST( SplitListDyn_Lazy_HRC         )
            CPPUNIT_TEST( SplitListStatic_Lazy_HRC      )
            CPPUNIT_TEST( SplitListDyn_Lazy_PTB         )
            CPPUNIT_TEST( SplitListStatic_Lazy_PTB      )
            CPPUNIT_TEST( SplitListDyn_Lazy_NoGC        )
            CPPUNIT_TEST( SplitListStatic_Lazy_NoGC     )

            CPPUNIT_TEST(MichaelHashMap_Michael_HP)
            CPPUNIT_TEST(MichaelHashMap_Michael_HRC)
            CPPUNIT_TEST(MichaelHashMap_Michael_PTB)
#ifdef CDS_DWORD_CAS_SUPPORTED
            CPPUNIT_TEST(MichaelHashMap_Michael_Tagged)
            CPPUNIT_TEST(MichaelHashMap_Michael_TaggedShared  )
#endif
            CPPUNIT_TEST(MichaelHashMap_Michael_NoGC)

            CPPUNIT_TEST(MichaelHashMap_Lazy_HP)
            CPPUNIT_TEST(MichaelHashMap_Lazy_HRC)
            CPPUNIT_TEST(MichaelHashMap_Lazy_PTB)
            CPPUNIT_TEST(MichaelHashMap_Lazy_NoGC)

            CPPUNIT_TEST( StdMap_Spin )
            CPPUNIT_TEST( StdHashMap_Spin )
#ifdef WIN32
            //            CPPUNIT_TEST( StdMap_WinCS )
            //            CPPUNIT_TEST( StdHashMap_WinCS )
#endif
            CPPUNIT_TEST_SUITE_END();
    } ;

    CPPUNIT_TEST_SUITE_REGISTRATION( Map_InsFind_int_MT );
} // namespace map
