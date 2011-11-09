/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


// defines concurrent access to map::nonconcurrent_iterator::Sequence::TValue::nAccess field

#include "map/map_nonconcurrent_iterator.h"
#include "map/map_types.h"
#include "cppunit/thread.h"

// map nonconcurrent_iterator test in multi-threaded read-only environment

namespace map {
    namespace nonconcurrent_iterator {
        size_t Sequence::nConstructCount = 0    ;
        size_t Sequence::nDestructCount  = 0    ;
    }

#    define TEST_MAP(X)    void X() { test<MapTypes<key_type, value_type>::X >()    ; }

    class Map_Nonconcurrent_iterator_MT: public nonconcurrent_iterator::Test
    {
        struct Stat {
            double          fPreInc   ;
            double          fPostInc  ;
            double          fSearch   ;

            Stat()
                : fPreInc(0)
                , fPostInc(0)
                , fSearch(0)
            {}

            Stat& operator +=( const Stat& t )
            {
                fPostInc += t.fPostInc  ;
                fPreInc  += t.fPreInc   ;
                fSearch  += t.fSearch   ;
                return *this    ;
            }

            Stat& operator /=( size_t n )
            {
                fPostInc /= n;
                fPreInc  /= n;
                fSearch  /= n;
                return *this    ;
            }
        };

        template <class MAP>
        class TestThread: public CppUnitMini::TestThread
        {
            MAP&        m_Map      ;

            virtual TestThread *    clone()
            {
                return new TestThread( *this )    ;
            }
        public:
            Stat            m_Stat      ;

        public:
            TestThread( CppUnitMini::ThreadPool& pool, MAP& rMap )
                : CppUnitMini::TestThread( pool )
                , m_Map( rMap )
            {}
            TestThread( TestThread& src )
                : CppUnitMini::TestThread( src )
                , m_Map( src.m_Map )
            {}

            Map_Nonconcurrent_iterator_MT&  getTest()
            {
                return reinterpret_cast<Map_Nonconcurrent_iterator_MT&>( m_Pool.m_Test )   ;
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            virtual void test()
            {
                typedef typename MAP::nonconcurrent_iterator    ncIterator  ;

                // Обходим map (используется pre-increment)
                {
                    m_Stat.fPreInc = m_Timer.duration()  ;
                    getTest().preIncPass( m_Map )  ;
                    m_Stat.fPreInc = m_Timer.duration() - m_Stat.fPreInc  ;
                }

                // Обходим map (используется post-increment)
                {
                    m_Stat.fPostInc = m_Timer.duration()     ;
                    getTest().postIncPass( m_Map )     ;
                    m_Stat.fPostInc = m_Timer.duration() - m_Stat.fPostInc ;
                }

                // Item search
                {
                    m_Stat.fSearch = m_Timer.duration()  ;
                    getTest().searchPass( m_Map )     ;
                    m_Stat.fSearch = m_Timer.duration() - m_Stat.fSearch  ;
                }
            }
        };

    protected:
        size_t  c_nThreadCount  ;
        size_t  c_nMapSize      ;

    protected:
        typedef nonconcurrent_iterator::Sequence::TKey        key_type    ;
        typedef nonconcurrent_iterator::Sequence::TValue    value_type    ;

        template <class MAP>
        void nciter_test( size_t nLoadFactor )
        {
            typedef TestThread<MAP>     Thread  ;
            MAP  testMap( m_pSeq->arrData.capacity(), nLoadFactor ) ;
            cds::OS::Timer    timer    ;

            CPPUNIT_MSG( "Load factor=" << nLoadFactor )   ;

            seq_const_iterator seqEnd = m_pSeq->arrData.end()    ;
            m_pSeq->clearAccess()   ;

            // Fill the map
            CPPUNIT_MSG( "   Fill map [item count=" << c_nMapSize << "] ...") ;
            timer.reset()    ;
            for ( seq_const_iterator i = m_pSeq->arrData.begin(); i != seqEnd; ++i ) {
                // Все ключи в arrData - уникальные, поэтому ошибок при вставке быть не должно
                CPPUNIT_ASSERT( testMap.insert( i->key, i->value ) )    ;
            }
            CPPUNIT_MSG( "   Duration=" << timer.duration() ) ;

            // Multi-threading read-only test
            size_t nThreadCount = c_nThreadCount ;
            CPPUNIT_MSG( "   Multi-reader test (thread count =" << nThreadCount << ") ...") ;
            CppUnitMini::ThreadPool pool( *this )   ;
            pool.add( new Thread( pool, testMap ), nThreadCount ) ;
            pool.run()  ;

            Stat stat ;
            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it )
                stat += reinterpret_cast<Thread *>(*it)->m_Stat   ;
            stat /= nThreadCount    ;
            CPPUNIT_MSG( "     Iterate map (pre-inc) duration=" << stat.fPreInc ) ;
            CPPUNIT_MSG( "     Iterate map (post-inc) duration=" << stat.fPostInc ) ;
            CPPUNIT_MSG( "     [item search] duration=" << stat.fSearch ) ;

            // Проверям, что посетили все элементы
            {
                CPPUNIT_MSG( "   [plain array] Test that all items has been accessed by map iter" ) ;
                timer.reset()    ;
                size_t n = 0    ;
                size_t nErrors = 0  ;
                for ( seq_const_iterator i = m_pSeq->arrData.begin(); i != seqEnd; ++i ) {
                    if ( i->value.nAccess != 2 * nThreadCount ) {
                        ++nErrors   ;
                        CPPUNIT_MSG( "value.nAccess[" << n << "]=" << i->value.nAccess )     ;
                        if ( nErrors > 10 )
                            CPPUNIT_ASSERT( i->value.nAccess == 2 * nThreadCount )  ;
                    }
                    ++n;
                }
                CPPUNIT_MSG( "   Duration=" << timer.duration() ) ;
            }
        }

        template <class MAP>
        void test()
        {
            bool bMustClear = ensureTestSequence()    ;

            CPPUNIT_MSG("Random sequence")        ;
            m_pSeq->makeRandomSortedSequence()  ;
            nciter_test<MAP>(1)    ;
            nciter_test<MAP>(2)    ;
            nciter_test<MAP>(4)    ;
            nciter_test<MAP>(8)    ;

            CPPUNIT_MSG("Asc sorted sequence")        ;
            m_pSeq->makeAscSortedSequence()  ;
            nciter_test<MAP>(1)    ;
            nciter_test<MAP>(2)    ;
            nciter_test<MAP>(4)    ;
            nciter_test<MAP>(8)    ;

            CPPUNIT_MSG("Desc sorted sequence")        ;
            m_pSeq->makeDescSortedSequence()  ;
            nciter_test<MAP>(1)    ;
            nciter_test<MAP>(2)    ;
            nciter_test<MAP>(4)    ;
            nciter_test<MAP>(8)    ;

            if ( bMustClear )
                cleanTestSequence() ;
        }

        void setUpParams( const CppUnitMini::TestCfg& cfg ) {
            c_nThreadCount = cfg.getULong("ThreadCount", 8 )        ; // thread count
            c_nMapSize = cfg.getULong("MapSize", 10000000 )         ;  // map size (count of searching item)
            m_nItemCount = c_nMapSize           ;
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

        TEST_MAP(SplitListDyn_Lazy_HP)          ;
        TEST_MAP(SplitListStatic_Lazy_HP)       ;
        TEST_MAP(SplitListDyn_Lazy_HRC)         ;
        TEST_MAP(SplitListStatic_Lazy_HRC)      ;
        TEST_MAP(SplitListDyn_Lazy_PTB)         ;
        TEST_MAP(SplitListStatic_Lazy_PTB)      ;
        TEST_MAP(SplitListDyn_Lazy_NoGC)        ;
        TEST_MAP(SplitListStatic_Lazy_NoGC)     ;

        TEST_MAP(MichaelHashMap_Michael_HP)     ;
        TEST_MAP(MichaelHashMap_Michael_HRC)    ;
        TEST_MAP(MichaelHashMap_Michael_PTB)    ;
        TEST_MAP(MichaelHashMap_Michael_NoGC)   ;
#ifdef CDS_DWORD_CAS_SUPPORTED
        TEST_MAP(MichaelHashMap_Michael_Tagged) ;
#endif
        //TEST_MAP(MichaelHashMap_Harris_HP)      ;
        //TEST_MAP(MichaelHashMap_Harris_HRC)     ;

        TEST_MAP(MichaelHashMap_Lazy_HP)        ;
        TEST_MAP(MichaelHashMap_Lazy_HRC)       ;
        TEST_MAP(MichaelHashMap_Lazy_PTB)       ;
        TEST_MAP(MichaelHashMap_Lazy_NoGC)      ;

        CPPUNIT_TEST_SUITE( Map_Nonconcurrent_iterator_MT )
            CPPUNIT_TEST( initTestSequence )

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

            CPPUNIT_TEST( SplitListDyn_Lazy_HP          )
            CPPUNIT_TEST( SplitListStatic_Lazy_HP       )
            CPPUNIT_TEST( SplitListDyn_Lazy_HRC         )
            CPPUNIT_TEST( SplitListStatic_Lazy_HRC      )
            CPPUNIT_TEST( SplitListDyn_Lazy_PTB         )
            CPPUNIT_TEST( SplitListStatic_Lazy_PTB      )
            CPPUNIT_TEST( SplitListDyn_Lazy_NoGC        )
            CPPUNIT_TEST( SplitListStatic_Lazy_NoGC     )

            CPPUNIT_TEST(MichaelHashMap_Michael_HP      )
            CPPUNIT_TEST(MichaelHashMap_Michael_HRC     )
            CPPUNIT_TEST(MichaelHashMap_Michael_PTB     )
            CPPUNIT_TEST(MichaelHashMap_Michael_NoGC    )
#ifdef CDS_DWORD_CAS_SUPPORTED
            CPPUNIT_TEST(MichaelHashMap_Michael_Tagged  )
#endif
            //CPPUNIT_TEST(MichaelHashMap_Harris_HP       )
            //CPPUNIT_TEST(MichaelHashMap_Harris_HRC      )

            CPPUNIT_TEST(MichaelHashMap_Lazy_HP         )
            CPPUNIT_TEST(MichaelHashMap_Lazy_HRC        )
            CPPUNIT_TEST(MichaelHashMap_Lazy_PTB        )
            CPPUNIT_TEST(MichaelHashMap_Lazy_NoGC       )

            CPPUNIT_TEST( cleanTestSequence )
        CPPUNIT_TEST_SUITE_END();
    } ;

    CPPUNIT_TEST_SUITE_REGISTRATION( Map_Nonconcurrent_iterator_MT );

} // namespace ordlist
