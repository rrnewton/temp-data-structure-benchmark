/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include "map/map_types.h"
#include "cppunit/thread.h"

#include <vector>

namespace map {

#    define TEST_MAP(X)    void X() { test<MapTypes<key_type, value_type>::X >()    ; }

    namespace {
        static size_t  c_nMapSize = 1000000    ;  // map size
        static size_t  c_nThreadCount = 4      ;  // thread count
        static size_t  c_nGoalItem = c_nMapSize / 2   ;
        static size_t  c_nAttemptCount = 100000       ;   // count of SUCCESS insert/delete for each thread
        static size_t  c_nMaxLoadFactor = 8    ;  // maximum load factor
        static bool    c_bPrintGCState = true  ;
    }

    class Map_InsDel_Item_int_MT: public CppUnitMini::TestCase
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

        public:
            Inserter( CppUnitMini::ThreadPool& pool, MAP& rMap )
                : CppUnitMini::TestThread( pool )
                , m_Map( rMap )
            {}
            Inserter( Inserter& src )
                : CppUnitMini::TestThread( src )
                , m_Map( src.m_Map )
            {}

            Map_InsDel_Item_int_MT&  getTest()
            {
                return reinterpret_cast<Map_InsDel_Item_int_MT&>( m_Pool.m_Test )   ;
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            virtual void test()
            {
                MAP& rMap = m_Map   ;

                m_nInsertSuccess =
                    m_nInsertFailed = 0 ;

                size_t nGoalItem = c_nGoalItem ;
                for ( size_t nAttempt = 0; nAttempt < c_nAttemptCount; ) {
                    if ( rMap.insert( nGoalItem, nGoalItem )) {
                        ++m_nInsertSuccess  ;
                        ++nAttempt         ;
                    }
                    else
                        ++m_nInsertFailed   ;
                }
            }
        };

        template <class MAP>
        class Deleter: public CppUnitMini::TestThread
        {
            MAP&     m_Map      ;

            virtual Deleter *    clone()
            {
                return new Deleter( *this )    ;
            }
        public:
            size_t  m_nDeleteSuccess    ;
            size_t  m_nDeleteFailed     ;

        public:
            Deleter( CppUnitMini::ThreadPool& pool, MAP& rMap )
                : CppUnitMini::TestThread( pool )
                , m_Map( rMap )
            {}
            Deleter( Deleter& src )
                : CppUnitMini::TestThread( src )
                , m_Map( src.m_Map )
            {}

            Map_InsDel_Item_int_MT&  getTest()
            {
                return reinterpret_cast<Map_InsDel_Item_int_MT&>( m_Pool.m_Test )   ;
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            virtual void test()
            {
                MAP& rMap = m_Map   ;

                m_nDeleteSuccess =
                    m_nDeleteFailed = 0 ;

                size_t nGoalItem = c_nGoalItem ;
                for ( size_t nAttempt = 0; nAttempt < c_nAttemptCount; ) {
                    if ( rMap.erase( nGoalItem )) {
                        ++m_nDeleteSuccess  ;
                        ++nAttempt  ;
                    }
                    else
                        ++m_nDeleteFailed   ;
                }
            }
        };

    protected:

        template <class MAP>
        void do_test( size_t nLoadFactor )
        {
            typedef Inserter<MAP>       InserterThread  ;
            typedef Deleter<MAP>        DeleterThread   ;
            MAP  testMap( c_nMapSize, nLoadFactor ) ;
            cds::OS::Timer    timer    ;

            CPPUNIT_MSG( "Load factor=" << nLoadFactor )   ;

            // Fill the map
            CPPUNIT_MSG( "  Fill map (" << c_nMapSize << " items)...") ;
            timer.reset()    ;
            for ( size_t i = 0; i < c_nMapSize; ++i ) {
                CPPUNIT_ASSERT( testMap.insert( i, i )) ;
            }
            CPPUNIT_MSG( "   Duration=" << timer.duration() ) ;

            CPPUNIT_MSG( "  Insert/delete the key " << c_nGoalItem << " (" << c_nAttemptCount << " successful times)...") ;
            CppUnitMini::ThreadPool pool( *this )   ;
            pool.add( new InserterThread( pool, testMap ), (c_nThreadCount + 1) / 2 ) ;
            pool.add( new DeleterThread( pool, testMap ), (c_nThreadCount + 1) / 2 ) ;
            pool.run()  ;
            CPPUNIT_MSG( "   Duration=" << pool.avgDuration() ) ;

            size_t nInsertSuccess = 0   ;
            size_t nInsertFailed = 0    ;
            size_t nDeleteSuccess = 0   ;
            size_t nDeleteFailed = 0    ;
            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it ) {
                InserterThread * pThread = dynamic_cast<InserterThread *>( *it )   ;
                if ( pThread ) {
                    CPPUNIT_ASSERT( pThread->m_nInsertSuccess == c_nAttemptCount ) ;
                    nInsertSuccess += pThread->m_nInsertSuccess ;
                    nInsertFailed += pThread->m_nInsertFailed   ;
                }
                else {
                    DeleterThread * p = static_cast<DeleterThread *>( *it ) ;
                    CPPUNIT_ASSERT( p->m_nDeleteSuccess == c_nAttemptCount ) ;
                    nDeleteSuccess += p->m_nDeleteSuccess   ;
                    nDeleteFailed += p->m_nDeleteFailed ;
                }
            }
            CPPUNIT_ASSERT( nInsertSuccess == nDeleteSuccess ) ;
            size_t nGoalItem = c_nGoalItem ;
            CPPUNIT_ASSERT( testMap.find( nGoalItem ))  ;


            CPPUNIT_MSG( "    Totals: Ins fail=" << nInsertFailed << " Del fail=" << nDeleteFailed ) ;

            // Check if the map contains all items
            CPPUNIT_MSG( "    Check if the map contains all items" ) ;
            timer.reset()   ;
            for ( size_t i = 0; i < c_nMapSize; ++i ) {
                CPPUNIT_ASSERT( testMap.find( i )) ;
            }
            CPPUNIT_MSG( "    Duration=" << timer.duration() )  ;

        }

        template <class MAP>
        void test()
        {
            for ( size_t nLoadFactor = 1; nLoadFactor <= c_nMaxLoadFactor; nLoadFactor *= 2 ) {
                do_test<MAP>( nLoadFactor )     ;
                if ( c_bPrintGCState )
                    print_gc_state()            ;
            }

        }

        void setUpParams( const CppUnitMini::TestCfg& cfg ) {
            c_nThreadCount = cfg.getULong("ThreadCount", 8 )        ; // thread count
            c_nMapSize = cfg.getULong("MapSize", 1000000 )         ;
            c_nGoalItem = cfg.getULong("GoalItem", (unsigned long) (c_nMapSize / 2) ) ;
            c_nAttemptCount = cfg.getULong("AttemptCount", 100000 ) ;
            c_nMaxLoadFactor = cfg.getULong("MaxLoadFactor", 8 )    ;
            c_bPrintGCState = cfg.getBool("PrintGCStateFlag", true );
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

        TEST_MAP(SplitListDyn_Lazy_HP)            ;
        TEST_MAP(SplitListStatic_Lazy_HP)        ;
        TEST_MAP(SplitListDyn_Lazy_HRC)            ;
        TEST_MAP(SplitListStatic_Lazy_HRC)        ;
        TEST_MAP(SplitListDyn_Lazy_PTB)            ;
        TEST_MAP(SplitListStatic_Lazy_PTB)        ;

        TEST_MAP(MichaelHashMap_Michael_HP)
        TEST_MAP(MichaelHashMap_Michael_HRC)
        TEST_MAP(MichaelHashMap_Michael_PTB)
#ifdef CDS_DWORD_CAS_SUPPORTED
        TEST_MAP(MichaelHashMap_Michael_Tagged)
        TEST_MAP(MichaelHashMap_Michael_TaggedShared)
#endif

        TEST_MAP(MichaelHashMap_Lazy_HP)
        TEST_MAP(MichaelHashMap_Lazy_HRC)
        TEST_MAP(MichaelHashMap_Lazy_PTB)

        TEST_MAP(StdMap_Spin)    ;
        TEST_MAP(StdHashMap_Spin)    ;
#ifdef WIN32
        TEST_MAP(StdMap_WinCS)    ;
        TEST_MAP(StdHashMap_WinCS)    ;
#endif

        CPPUNIT_TEST_SUITE( Map_InsDel_Item_int_MT )
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

            CPPUNIT_TEST( SplitListDyn_Lazy_HP          )
            CPPUNIT_TEST( SplitListStatic_Lazy_HP       )
            CPPUNIT_TEST( SplitListDyn_Lazy_HRC         )
            CPPUNIT_TEST( SplitListStatic_Lazy_HRC      )
            CPPUNIT_TEST( SplitListDyn_Lazy_PTB         )
            CPPUNIT_TEST( SplitListStatic_Lazy_PTB      )

            CPPUNIT_TEST(MichaelHashMap_Michael_HP)
            CPPUNIT_TEST(MichaelHashMap_Michael_HRC)
            CPPUNIT_TEST(MichaelHashMap_Michael_PTB)
#ifdef CDS_DWORD_CAS_SUPPORTED
            CPPUNIT_TEST(MichaelHashMap_Michael_Tagged)
            CPPUNIT_TEST(MichaelHashMap_Michael_TaggedShared  )
#endif
            CPPUNIT_TEST(MichaelHashMap_Lazy_HP)
            CPPUNIT_TEST(MichaelHashMap_Lazy_HRC)
            CPPUNIT_TEST(MichaelHashMap_Lazy_PTB)

//            CPPUNIT_TEST( StdMap_Spin )
//            CPPUNIT_TEST( StdHashMap_Spin )
#ifdef WIN32
//            CPPUNIT_TEST( StdMap_WinCS )
//            CPPUNIT_TEST( StdHashMap_WinCS )
#endif
        CPPUNIT_TEST_SUITE_END();
    } ;

    CPPUNIT_TEST_SUITE_REGISTRATION( Map_InsDel_Item_int_MT );
} // namespace map
