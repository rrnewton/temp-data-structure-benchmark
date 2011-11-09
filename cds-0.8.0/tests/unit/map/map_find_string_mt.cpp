/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


// defines concurrent access to map::nonconcurrent_iterator::Sequence::TValue::nAccess field

#include "map/map_types.h"
#include "cppunit/thread.h"

#include <vector>

// find int test in map<int> in mutithreaded mode
namespace map {

#    define TEST_MAP(X)    void X() { test<MapTypes<key_type, value_type>::X >()    ; }

    namespace {
        static size_t  c_nThreadCount = 8      ;  // thread count
        static size_t  c_nMapSize = 20000000   ;  // map size (count of searching item)
        static size_t  c_nPercentExists = 50   ;  // percent of existing keys in searching sequence
        static size_t  c_nPassCount = 2        ;
        static size_t  c_nMaxLoadFactor = 8    ;  // maximum load factor
        static bool    c_bPrintGCState = true  ;
    }

    class Map_find_string_MT: public CppUnitMini::TestCase
    {
        typedef std::string  key_type ;
        struct value_type {
            std::string const * pKey  ;
            bool        bExists ;   // true - key in map, false - key not in map
        }   ;

        typedef std::vector<value_type> ValueVector ;
        ValueVector             m_Arr   ;
        size_t                  m_nRealMapSize  ;
        bool                    m_bSeqInit      ;

        template <class MAP>
        class TestThread: public CppUnitMini::TestThread
        {
            MAP&     m_Map      ;

            virtual TestThread *    clone()
            {
                return new TestThread( *this )    ;
            }
        public:
            struct Stat {
                size_t      nSuccess    ;
                size_t      nFailed     ;

                Stat()
                    : nSuccess(0)
                    , nFailed(0)
                {}
            };

            Stat    m_KeyExists     ;
            Stat    m_KeyNotExists  ;

        public:
            TestThread( CppUnitMini::ThreadPool& pool, MAP& rMap )
                : CppUnitMini::TestThread( pool )
                , m_Map( rMap )
            {}
            TestThread( TestThread& src )
                : CppUnitMini::TestThread( src )
                , m_Map( src.m_Map )
            {}

            Map_find_string_MT&  getTest()
            {
                return reinterpret_cast<Map_find_string_MT&>( m_Pool.m_Test )   ;
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            virtual void test()
            {
                ValueVector& arr = getTest().m_Arr  ;
                //size_t nSize = arr.size()   ;

                MAP& rMap = m_Map   ;
                for ( size_t nPass = 0; nPass < c_nPassCount; ++nPass ) {
                    if ( m_nThreadNo & 1 ) {
                        ValueVector::const_iterator itEnd = arr.end()   ;
                        for ( ValueVector::const_iterator it = arr.begin(); it != itEnd; ++it ) {
                            bool bFound = rMap.find( *(it->pKey) ) ;
                            if ( it->bExists ) {
                                if ( bFound )
                                    ++m_KeyExists.nSuccess  ;
                                else
                                    ++m_KeyExists.nFailed   ;
                            }
                            else {
                                if ( bFound )
                                    ++m_KeyNotExists.nFailed    ;
                                else
                                    ++m_KeyNotExists.nSuccess   ;
                            }
                        }
                    }
                    else {
                        ValueVector::const_reverse_iterator itEnd = arr.rend()   ;
                        for ( ValueVector::const_reverse_iterator it = arr.rbegin(); it != itEnd; ++it ) {
                            bool bFound = rMap.find( *(it->pKey) ) ;
                            if ( it->bExists ) {
                                if ( bFound )
                                    ++m_KeyExists.nSuccess  ;
                                else
                                    ++m_KeyExists.nFailed   ;
                            }
                            else {
                                if ( bFound )
                                    ++m_KeyNotExists.nFailed    ;
                                else
                                    ++m_KeyNotExists.nSuccess   ;
                            }
                        }
                    }
                }
            }
        };

    public:
        Map_find_string_MT()
            : m_bSeqInit( false )
        {}

    protected:

        void generateSequence()
        {
            size_t nPercent = c_nPercentExists  ;

            if ( nPercent > 100 )
                nPercent = 100      ;
            else if ( nPercent < 1 )
                nPercent = 1        ;

            m_nRealMapSize = 0      ;

            std::vector<std::string> const & arrString = CppUnitMini::TestCase::getTestStrings()    ;
            size_t nSize = arrString.size() ;
            if ( nSize > c_nMapSize )
                nSize = c_nMapSize  ;
            m_Arr.resize( nSize )  ;
            for ( size_t i = 0; i < nSize; ++i ) {
                m_Arr[i].pKey = &( arrString[i] ) ;
                m_Arr[i].bExists = CppUnitMini::Rand( 100 ) <= nPercent ;
                if ( m_Arr[i].bExists )
                    ++m_nRealMapSize    ;
            }
        }


        template <class MAP>
        void find_string_test( size_t nLoadFactor )
        {
            typedef TestThread<MAP>     Thread  ;
            MAP  testMap( m_Arr.size(), nLoadFactor ) ;
            cds::OS::Timer    timer    ;

            CPPUNIT_MSG( "Load factor=" << nLoadFactor )   ;

            // Fill the map
            CPPUNIT_MSG( "  Fill map...") ;
            timer.reset()    ;
            for ( size_t i = 0; i < m_Arr.size(); ++i ) {
                // Все ключи в arrData - уникальные, поэтому ошибок при вставке быть не должно
                if ( m_Arr[i].bExists )
                    CPPUNIT_ASSERT( testMap.insert( *(m_Arr[i].pKey), m_Arr[i] ) )    ;
            }
            CPPUNIT_MSG( "   Duration=" << timer.duration() ) ;

            CPPUNIT_MSG( "  Searching...") ;
            CppUnitMini::ThreadPool pool( *this )   ;
            pool.add( new Thread( pool, testMap ), c_nThreadCount ) ;
            pool.run()  ;
            CPPUNIT_MSG( "   Duration=" << pool.avgDuration() ) ;

            // Проверяем, что у всех threads число успешных поисков = числу элементов в map
            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it ) {
                Thread * pThread = static_cast<Thread *>( *it )   ;
                CPPUNIT_ASSERT( pThread->m_KeyExists.nSuccess == m_nRealMapSize * c_nPassCount ) ;
                CPPUNIT_ASSERT( pThread->m_KeyExists.nFailed == 0 ) ;
                CPPUNIT_ASSERT( pThread->m_KeyNotExists.nSuccess == (m_Arr.size() - m_nRealMapSize) * c_nPassCount ) ;
                CPPUNIT_ASSERT( pThread->m_KeyNotExists.nFailed == 0 ) ;
            }
        }

        void initTestSequence()
        {
            if ( !m_bSeqInit ) {
                m_bSeqInit = true   ;

                CPPUNIT_MSG( "Generating test data...") ;
                cds::OS::Timer    timer    ;
                generateSequence()  ;
                CPPUNIT_MSG( "   Duration=" << timer.duration() )   ;
                CPPUNIT_MSG( "Map size=" << m_nRealMapSize << " find key loop=" << m_Arr.size() << " (" << c_nPercentExists << "% success)" ) ;
                CPPUNIT_MSG( "Thread count=" << c_nThreadCount << " Pass count=" << c_nPassCount )    ;
            }
        }

        template <class MAP>
        void test()
        {
            initTestSequence()  ;

            for ( size_t nLoadFactor = 1; nLoadFactor <= c_nMaxLoadFactor; nLoadFactor *= 2 ) {
                find_string_test<MAP>( nLoadFactor )    ;
                if ( c_bPrintGCState )
                    print_gc_state()                    ;
            }
        }

        void setUpParams( const CppUnitMini::TestCfg& cfg ) {
            c_nThreadCount = cfg.getULong("ThreadCount", 8 )        ; // thread count
            c_nMapSize = cfg.getULong("MapSize", 20000000 )         ;  // map size (count of searching item)
            c_nPercentExists = cfg.getULong("PercentExists", 50 )   ;  // percent of existing keys in searching sequence
            c_nPassCount = cfg.getULong("PassCount", 2 )            ;
            c_nMaxLoadFactor = cfg.getULong("MaxLoadFactor", 8 )    ;
            c_bPrintGCState = cfg.getBool("PrintGCStateFlag", true );
        }

        TEST_MAP(SplitListDyn_Michael_HP)        ;
        TEST_MAP(SplitListStatic_Michael_HP)    ;
        TEST_MAP(SplitListDyn_Michael_HRC)        ;
        TEST_MAP(SplitListStatic_Michael_HRC)    ;
        TEST_MAP(SplitListDyn_Michael_PTB)        ;
        TEST_MAP(SplitListStatic_Michael_PTB)    ;
        TEST_MAP(SplitListDyn_Michael_NoGC)        ;
        TEST_MAP(SplitListStatic_Michael_NoGC)    ;
#ifdef CDS_DWORD_CAS_SUPPORTED
        TEST_MAP(SplitListDyn_Michael_Tagged)    ;
        TEST_MAP(SplitListStatic_Michael_Tagged);
#endif
        //TEST_MAP(SplitListDyn_Harris_HP)        ;
        //TEST_MAP(SplitListStatic_Harris_HP)        ;
        //TEST_MAP(SplitListDyn_Harris_HRC)        ;
        //TEST_MAP(SplitListStatic_Harris_HRC)    ;

        TEST_MAP(SplitListDyn_Lazy_HP)            ;
        TEST_MAP(SplitListStatic_Lazy_HP)        ;
        TEST_MAP(SplitListDyn_Lazy_HRC)            ;
        TEST_MAP(SplitListStatic_Lazy_HRC)        ;
        TEST_MAP(SplitListDyn_Lazy_NoGC)        ;
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
        TEST_MAP(MichaelHashMap_Lazy_NoGC)


        TEST_MAP(StdMap_NoLock)    ;
        TEST_MAP(StdHashMap_NoLock)    ;
        TEST_MAP(StdMap_Spin)    ;
        TEST_MAP(StdHashMap_Spin)    ;
#ifdef WIN32
        TEST_MAP(StdMap_WinCS)    ;
        TEST_MAP(StdHashMap_WinCS)    ;
#endif

        CPPUNIT_TEST_SUITE( Map_find_string_MT )
            CPPUNIT_TEST( SplitListDyn_Michael_HP        )
            CPPUNIT_TEST( SplitListStatic_Michael_HP    )
            CPPUNIT_TEST( SplitListDyn_Michael_HRC        )
            CPPUNIT_TEST( SplitListStatic_Michael_HRC    )
            CPPUNIT_TEST( SplitListDyn_Michael_PTB        )
            CPPUNIT_TEST( SplitListStatic_Michael_PTB    )
            CPPUNIT_TEST( SplitListDyn_Michael_NoGC        )
            CPPUNIT_TEST( SplitListStatic_Michael_NoGC  )
#ifdef CDS_DWORD_CAS_SUPPORTED
            CPPUNIT_TEST( SplitListDyn_Michael_Tagged    )
            CPPUNIT_TEST( SplitListStatic_Michael_Tagged)
#endif
            //CPPUNIT_TEST( SplitListDyn_Harris_HP        )
            //CPPUNIT_TEST( SplitListStatic_Harris_HP        )
            //CPPUNIT_TEST( SplitListDyn_Harris_HRC        )
            //CPPUNIT_TEST( SplitListStatic_Harris_HRC    )

            CPPUNIT_TEST( SplitListDyn_Lazy_HP            )
            CPPUNIT_TEST( SplitListStatic_Lazy_HP        )
            CPPUNIT_TEST( SplitListDyn_Lazy_HRC            )
            CPPUNIT_TEST( SplitListStatic_Lazy_HRC        )
            CPPUNIT_TEST( SplitListDyn_Lazy_NoGC        )
            CPPUNIT_TEST( SplitListStatic_Lazy_NoGC        )

            CPPUNIT_TEST(MichaelHashMap_Michael_HP      )
            CPPUNIT_TEST(MichaelHashMap_Michael_HRC     )
            CPPUNIT_TEST(MichaelHashMap_Michael_PTB     )
            CPPUNIT_TEST(MichaelHashMap_Michael_NoGC    )
#ifdef CDS_DWORD_CAS_SUPPORTED
            CPPUNIT_TEST(MichaelHashMap_Michael_Tagged  )
            CPPUNIT_TEST(MichaelHashMap_Michael_TaggedShared  )
#endif
            //CPPUNIT_TEST(MichaelHashMap_Harris_HP       )
            //CPPUNIT_TEST(MichaelHashMap_Harris_HRC      )

            CPPUNIT_TEST(MichaelHashMap_Lazy_HP         )
            CPPUNIT_TEST(MichaelHashMap_Lazy_HRC        )
            CPPUNIT_TEST(MichaelHashMap_Lazy_NoGC       )

            CPPUNIT_TEST( StdMap_NoLock )
            CPPUNIT_TEST( StdHashMap_NoLock )
            CPPUNIT_TEST( StdMap_Spin )
            CPPUNIT_TEST( StdHashMap_Spin )
#ifdef WIN32
//            CPPUNIT_TEST( StdMap_WinCS )
//            CPPUNIT_TEST( StdHashMap_WinCS )
#endif
        CPPUNIT_TEST_SUITE_END();
    } ;

    CPPUNIT_TEST_SUITE_REGISTRATION( Map_find_string_MT );
} // namespace map
