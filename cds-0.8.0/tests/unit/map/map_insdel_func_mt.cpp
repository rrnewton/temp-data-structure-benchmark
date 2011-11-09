/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include "map/map_types.h"
#include "cppunit/thread.h"

#include <cds/lock/spinlock.h>
#include <vector>
#include <boost/ref.hpp>

namespace map {

#    define TEST_MAP(X)    void X() { test<MapTypes<key_type, value_type>::X >()    ; }

    namespace {
        static size_t  c_nMapSize = 1000000    ;  // map size
        static size_t  c_nInsertThreadCount = 4;  // count of insertion thread
        static size_t  c_nDeleteThreadCount = 4;  // count of deletion thread
        static size_t  c_nEnsureThreadCount = 4;  // count of ensure thread
        static size_t  c_nThreadPassCount = 4  ;  // pass count for each thread
        static size_t  c_nMaxLoadFactor = 8    ;  // maximum load factor
        static bool    c_bPrintGCState = true  ;
    }

    class Map_InsDel_func_MT: public CppUnitMini::TestCase
    {
        typedef size_t  key_type    ;
        struct value_type {
            size_t      nKey        ;
            size_t      nData       ;
            size_t      nEnsureCall ;

            typedef cds::lock::SpinT< cds::lock::atomic_spin_t, cds::backoff::pause >   lock_type   ;
            mutable cds::lock::SpinT< cds::lock::atomic_spin_t, cds::backoff::pause >   m_access    ;

            value_type()
                : nKey(0)
                , nData(0)
                , nEnsureCall(0)
            {}
        };

        template <class MAP>
        class Inserter: public CppUnitMini::TestThread
        {
            MAP&     m_Map      ;

            virtual Inserter *    clone()
            {
                return new Inserter( *this )    ;
            }

            struct insert_functor {
                size_t nTestFunctorRef ;

                insert_functor()
                    : nTestFunctorRef(0)
                {}

                void operator()( value_type& val, size_t n )
                {
                    cds::lock::Auto<value_type::lock_type>    ac( val.m_access )  ;

                    val.nKey = n        ;
                    val.nData = n * 8   ;

                    ++nTestFunctorRef   ;
                }
            } ;

        public:
            size_t  m_nInsertSuccess    ;
            size_t  m_nInsertFailed     ;

            size_t  m_nTestFunctorRef   ;

        public:
            Inserter( CppUnitMini::ThreadPool& pool, MAP& rMap )
                : CppUnitMini::TestThread( pool )
                , m_Map( rMap )
            {}
            Inserter( Inserter& src )
                : CppUnitMini::TestThread( src )
                , m_Map( src.m_Map )
            {}

            Map_InsDel_func_MT&  getTest()
            {
                return reinterpret_cast<Map_InsDel_func_MT&>( m_Pool.m_Test )   ;
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            virtual void test()
            {
                MAP& rMap = m_Map   ;

                m_nInsertSuccess =
                    m_nInsertFailed = 
                    m_nTestFunctorRef = 0 ;

                // func is passed by value
                insert_functor  func    ;

                if ( m_nThreadNo & 1 ) {
                    for ( size_t nPass = 0; nPass < c_nThreadPassCount; ++nPass ) {
                        for ( size_t nItem = 0; nItem < c_nMapSize; ++nItem ) {
                            if ( rMap.insert( nItem, nItem, cds::ref(func) ) )
                                ++m_nInsertSuccess  ;
                            else
                                ++m_nInsertFailed   ;
                        }
                    }
                }
                else {
                    for ( size_t nPass = 0; nPass < c_nThreadPassCount; ++nPass ) {
                        for ( size_t nItem = c_nMapSize; nItem > 0; --nItem ) {
                            if ( rMap.insert( nItem - 1, nItem -1, cds::ref(func) ) )
                                ++m_nInsertSuccess  ;
                            else
                                ++m_nInsertFailed   ;
                        }
                    }
                }

                m_nTestFunctorRef = func.nTestFunctorRef    ;
            }
        };

        template <class MAP>
        class Ensurer: public CppUnitMini::TestThread
        {
            MAP&     m_Map      ;

            virtual Ensurer *    clone()
            {
                return new Ensurer( *this )    ;
            }

            struct ensure_functor {
                size_t  nCreated    ;
                size_t  nModified   ;

                ensure_functor()
                    : nCreated(0)
                    , nModified(0)
                {}

                void operator()( value_type& val, size_t n, bool bNew )
                {
                    cds::lock::Auto<value_type::lock_type>    ac( val.m_access )  ;
                    if ( bNew ) {
                        ++nCreated          ;
                        val.nKey = n        ;
                        val.nData = n * 8   ;
                    }
                    else {
                        cds::atomics::inc<cds::membar_acquire>( &val.nEnsureCall )    ;
                        ++nModified         ;
                    }
                }
            private:
                ensure_functor(const ensure_functor& )  ;
            } ;

        public:
            size_t  m_nEnsureFailed     ;
            size_t  m_nEnsureCreated    ;
            size_t  m_nEnsureExisted    ;
            size_t  m_nFunctorCreated   ;
            size_t  m_nFunctorModified  ;

        public:
            Ensurer( CppUnitMini::ThreadPool& pool, MAP& rMap )
                : CppUnitMini::TestThread( pool )
                , m_Map( rMap )
            {}
            Ensurer( Ensurer& src )
                : CppUnitMini::TestThread( src )
                , m_Map( src.m_Map )
            {}

            Map_InsDel_func_MT&  getTest()
            {
                return reinterpret_cast<Map_InsDel_func_MT&>( m_Pool.m_Test )   ;
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            virtual void test()
            {
                MAP& rMap = m_Map   ;

                m_nEnsureCreated =
                    m_nEnsureExisted =
                    m_nEnsureFailed = 0 ;

                ensure_functor func ;

                if ( m_nThreadNo & 1 ) {
                    for ( size_t nPass = 0; nPass < c_nThreadPassCount; ++nPass ) {
                        for ( size_t nItem = 0; nItem < c_nMapSize; ++nItem ) {
                            std::pair<bool, bool> ret = rMap.ensure( nItem, nItem, cds::ref( func ) ) ;
                            if ( ret.first  ) {
                                if ( ret.second )
                                    ++m_nEnsureCreated  ;
                                else
                                    ++m_nEnsureExisted  ;
                            }
                            else
                                ++m_nEnsureFailed       ;
                        }
                    }
                }
                else {
                    for ( size_t nPass = 0; nPass < c_nThreadPassCount; ++nPass ) {
                        for ( size_t nItem = c_nMapSize; nItem > 0; --nItem ) {
                            std::pair<bool, bool> ret = rMap.ensure( nItem - 1, nItem - 1, cds::ref( func ) ) ;
                            if ( ret.first  ) {
                                if ( ret.second )
                                    ++m_nEnsureCreated  ;
                                else
                                    ++m_nEnsureExisted  ;
                            }
                            else
                                ++m_nEnsureFailed       ;
                        }
                    }
                }

                m_nFunctorCreated = func.nCreated   ;
                m_nFunctorModified = func.nModified ;
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

            struct value_container 
            {                
                size_t      nKeyExpected    ;

                size_t      nSuccessItem    ;
                size_t      nFailedItem     ;

                value_container()
                    : nSuccessItem(0)
                    , nFailedItem(0)
                {}
            };

            struct erase_functor {
                void operator ()( value_container& cnt, value_type& item )
                {
                    cds::lock::Auto<value_type::lock_type>    ac( item.m_access )  ;

                    if ( cnt.nKeyExpected == item.nKey && cnt.nKeyExpected * 8 == item.nData )
                        ++cnt.nSuccessItem  ;
                    else
                        ++cnt.nFailedItem   ;
                }
            };

        public:
            size_t  m_nDeleteSuccess    ;
            size_t  m_nDeleteFailed     ;

            size_t  m_nValueSuccess     ;
            size_t  m_nValueFailed      ;

        public:
            Deleter( CppUnitMini::ThreadPool& pool, MAP& rMap )
                : CppUnitMini::TestThread( pool )
                , m_Map( rMap )
            {}
            Deleter( Deleter& src )
                : CppUnitMini::TestThread( src )
                , m_Map( src.m_Map )
            {}

            Map_InsDel_func_MT&  getTest()
            {
                return reinterpret_cast<Map_InsDel_func_MT&>( m_Pool.m_Test )   ;
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            virtual void test()
            {
                MAP& rMap = m_Map   ;

                m_nDeleteSuccess =
                    m_nDeleteFailed = 0 ;

                value_container     delVal  ;

                if ( m_nThreadNo & 1 ) {
                    for ( size_t nPass = 0; nPass < c_nThreadPassCount; ++nPass ) {
                        for ( size_t nItem = 0; nItem < c_nMapSize; ++nItem ) {
                            delVal.nKeyExpected = nItem ;
                            if ( rMap.erase( delVal.nKeyExpected, delVal, erase_functor() ))
                                ++m_nDeleteSuccess  ;
                            else
                                ++m_nDeleteFailed   ;
                        }
                    }
                }
                else {
                    for ( size_t nPass = 0; nPass < c_nThreadPassCount; ++nPass ) {
                        for ( size_t nItem = c_nMapSize; nItem > 0; --nItem ) {
                            delVal.nKeyExpected = nItem - 1 ;
                            if ( rMap.erase( delVal.nKeyExpected, delVal, erase_functor() ))
                                ++m_nDeleteSuccess  ;
                            else
                                ++m_nDeleteFailed   ;
                        }
                    }
                }

                m_nValueSuccess = delVal.nSuccessItem   ;
                m_nValueFailed = delVal.nFailedItem     ;
            }
        };

    protected:

        template <class MAP>
        void do_test( size_t nLoadFactor )
        {
            typedef Inserter<MAP>       InserterThread  ;
            typedef Deleter<MAP>        DeleterThread   ;
            typedef Ensurer<MAP>        EnsurerThread   ;
            MAP  testMap( c_nMapSize, nLoadFactor ) ;
            cds::OS::Timer    timer    ;

            CPPUNIT_MSG( "Load factor=" << nLoadFactor )   ;

            CppUnitMini::ThreadPool pool( *this )   ;
            pool.add( new InserterThread( pool, testMap ), c_nInsertThreadCount ) ;
            pool.add( new DeleterThread( pool, testMap ), c_nDeleteThreadCount ) ;
            pool.add( new EnsurerThread( pool, testMap ), c_nEnsureThreadCount ) ;
            pool.run()  ;
            CPPUNIT_MSG( "   Duration=" << pool.avgDuration() ) ;

            size_t nInsertSuccess = 0   ;
            size_t nInsertFailed = 0    ;
            size_t nDeleteSuccess = 0   ;
            size_t nDeleteFailed = 0    ;
            size_t nDelValueSuccess = 0 ;
            size_t nDelValueFailed = 0  ;
            size_t nEnsureFailed = 0    ;
            size_t nEnsureCreated = 0   ;
            size_t nEnsureModified = 0  ;
            size_t nEnsFuncCreated = 0  ;
            size_t nEnsFuncModified = 0 ;
            size_t nTestFunctorRef = 0  ;

            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it ) {
                InserterThread * pThread = dynamic_cast<InserterThread *>( *it )   ;
                if ( pThread ) {
                    nInsertSuccess += pThread->m_nInsertSuccess ;
                    nInsertFailed += pThread->m_nInsertFailed   ;
                    nTestFunctorRef += pThread->m_nTestFunctorRef   ;
                }
                else {
                    DeleterThread * p = dynamic_cast<DeleterThread *>( *it ) ;
                    if ( p ) {
                        nDeleteSuccess += p->m_nDeleteSuccess   ;
                        nDeleteFailed += p->m_nDeleteFailed     ;
                        nDelValueSuccess += p->m_nValueSuccess  ;
                        nDelValueFailed += p->m_nValueFailed    ;
                    }
                    else {
                        EnsurerThread * pEns = static_cast<EnsurerThread *>( *it )    ;
                        nEnsureCreated += pEns->m_nEnsureCreated    ;
                        nEnsureModified += pEns->m_nEnsureExisted   ;
                        nEnsureFailed += pEns->m_nEnsureFailed      ;
                        nEnsFuncCreated += pEns->m_nFunctorCreated  ;
                        nEnsFuncModified += pEns->m_nFunctorModified;
                    }
                }
            }

            CPPUNIT_MSG( "    Totals: Ins succ=" << nInsertSuccess 
                << " Del succ=" << nDeleteSuccess << "\n"
                << "          : Ins fail=" << nInsertFailed
                << " Del fail=" << nDeleteFailed << "\n"
                << "          : Ensure succ=" << (nEnsureCreated + nEnsureModified) << " fail=" << nEnsureFailed 
                << " create=" << nEnsureCreated << " modify=" << nEnsureModified << "\n"
                << "          Map size=" << testMap.size()
                ) ;

            CPPUNIT_CHECK_EX( nDelValueFailed == 0, "Functor del failed=" << nDelValueFailed )  ;
            CPPUNIT_CHECK_EX( nDelValueSuccess == nDeleteSuccess,  "Delete success=" << nDeleteSuccess << " functor=" << nDelValueSuccess )    ;

            CPPUNIT_CHECK( nEnsureFailed == 0 )    ;

            // ensure functor may be called several times when new item has been added
            CPPUNIT_CHECK_EX( nEnsureCreated <= nEnsFuncCreated, "Ensure created=" << nEnsureCreated << " functor=" << nEnsFuncCreated )       ;
            CPPUNIT_CHECK_EX( nEnsureModified == nEnsFuncModified, "Ensure modified=" << nEnsureModified << " functor=" << nEnsFuncModified )  ;

            // nTestFunctorRef is not accumulated because insert_functor is passed by value in Insert threads
            CPPUNIT_CHECK_EX( nTestFunctorRef >= nInsertSuccess, "nInsertSuccess=" << nInsertSuccess << " functor nTestFunctorRef=" << nTestFunctorRef )  ;

            CPPUNIT_MSG( "  Clear map (single-threaded)..." ) ;
            timer.reset()   ;
            for ( size_t nItem = 0; nItem < c_nMapSize; ++nItem ) {
                testMap.erase( nItem )  ;
            }
            CPPUNIT_MSG( "   Duration=" << timer.duration() ) ;
            CPPUNIT_CHECK( testMap.empty() ) ;
        }

        template <class MAP>
        void test()
        {
            CPPUNIT_MSG( "Thread count: insert=" << c_nInsertThreadCount
                << " delete=" << c_nDeleteThreadCount
                << " ensure=" << c_nEnsureThreadCount
                << " pass count=" << c_nThreadPassCount
                << " map size=" << c_nMapSize
                );

            for ( size_t nLoadFactor = 1; nLoadFactor <= c_nMaxLoadFactor; nLoadFactor *= 2 ) {
                do_test<MAP>( nLoadFactor )     ;
                if ( c_bPrintGCState )
                    print_gc_state()            ;
            }

        }

        void setUpParams( const CppUnitMini::TestCfg& cfg ) {
            c_nInsertThreadCount = cfg.getULong("InsertThreadCount", 4 )  ;
            c_nDeleteThreadCount = cfg.getULong("DeleteThreadCount", 4 )  ;
            c_nEnsureThreadCount = cfg.getULong("EnsureThreadCount", 4 )  ;
            c_nThreadPassCount = cfg.getULong("ThreadPassCount", 4 )      ;
            c_nMapSize = cfg.getULong("MapSize", 1000000 )                ;
            c_nMaxLoadFactor = cfg.getULong("MaxLoadFactor", 8 )          ;
            c_bPrintGCState = cfg.getBool("PrintGCStateFlag", true )      ;
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

        TEST_MAP(MichaelHashMap_Michael_HP)
        TEST_MAP(MichaelHashMap_Michael_HRC)
        TEST_MAP(MichaelHashMap_Michael_PTB)
#ifdef CDS_DWORD_CAS_SUPPORTED
        TEST_MAP(MichaelHashMap_Michael_Tagged)
        TEST_MAP(MichaelHashMap_Michael_TaggedShared)
#endif
        //TEST_MAP(MichaelHashMap_Harris_HP)
        //TEST_MAP(MichaelHashMap_Harris_HRC)

        TEST_MAP(MichaelHashMap_Lazy_HP)
        TEST_MAP(MichaelHashMap_Lazy_HRC)
        TEST_MAP(MichaelHashMap_Lazy_PTB)

        TEST_MAP(StdMap_Spin)    ;
        TEST_MAP(StdHashMap_Spin)    ;
#ifdef WIN32
        //TEST_MAP(StdMap_WinCS)    ;
        //TEST_MAP(StdHashMap_WinCS)    ;
#endif

        CPPUNIT_TEST_SUITE( Map_InsDel_func_MT )
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

            CPPUNIT_TEST( StdMap_Spin )
            CPPUNIT_TEST( StdHashMap_Spin )
#ifdef WIN32
//            CPPUNIT_TEST( StdMap_WinCS )
//            CPPUNIT_TEST( StdHashMap_WinCS )
#endif
        CPPUNIT_TEST_SUITE_END();
    } ;

    CPPUNIT_TEST_SUITE_REGISTRATION( Map_InsDel_func_MT );
} // namespace map
