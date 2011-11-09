/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


// defines concurrent access to map::nonconcurrent_iterator::Sequence::TValue::nAccess field

#include "ordlist/ordlist_nonconcurrent_iterator.h"
#include "ordlist/ordlist_types.h"
#include "cppunit/thread.h"

// ordered list nonconcurrent_iterator test in multi-threaded read-only environment

namespace ordlist {

#    define TEST_LIST(X)    void X() { test<OrdlistTypes<key_type, value_type>::X >()    ; }

    class OrdList_Nonconcurrent_iterator_MT: public nonconcurrent_iterator::Test
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

        template <class ORDLIST>
        class TestThread: public CppUnitMini::TestThread
        {
            ORDLIST&        m_List      ;

            virtual TestThread *    clone()
            {
                return new TestThread( *this )    ;
            }
        public:
            Stat            m_Stat      ;

        public:
            TestThread( CppUnitMini::ThreadPool& pool, ORDLIST& list )
                : CppUnitMini::TestThread( pool )
                , m_List( list )
            {}
            TestThread( TestThread& src )
                : CppUnitMini::TestThread( src )
                , m_List( src.m_List )
            {}

            OrdList_Nonconcurrent_iterator_MT&  getTest()
            {
                return reinterpret_cast<OrdList_Nonconcurrent_iterator_MT&>( m_Pool.m_Test )   ;
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            virtual void test()
            {
                typedef typename ORDLIST::nonconcurrent_iterator    ncIterator  ;

                // Обходим list (используется pre-increment)
                {
                    m_Stat.fPreInc = m_Timer.duration()  ;
                    getTest().preIncPass( m_List )  ;
                    m_Stat.fPreInc = m_Timer.duration() - m_Stat.fPreInc  ;
                }

                // Обходим list (используется post-increment)
                {
                    m_Stat.fPostInc = m_Timer.duration()     ;
                    getTest().postIncPass( m_List )     ;
                    m_Stat.fPostInc = m_Timer.duration() - m_Stat.fPostInc ;
                }

                // Item search
                {
                    m_Stat.fSearch = m_Timer.duration()  ;
                    getTest().searchPass( m_List )     ;
                    m_Stat.fSearch = m_Timer.duration() - m_Stat.fSearch  ;
                }
            }
        };

    protected:
        typedef nonconcurrent_iterator::Sequence::TKey        key_type    ;
        typedef nonconcurrent_iterator::Sequence::TValue    value_type    ;

        template <class ORDLIST>
        void nciter_test()
        {
            typedef TestThread<ORDLIST>     Thread  ;
            ORDLIST  testList    ;
            cds::OS::Timer    timer    ;

            seq_const_iterator seqEnd = m_pSeq->arrData.end()    ;
            m_pSeq->clearAccess()   ;

            // Fill the list
            CPPUNIT_MSG( "   Fill list...") ;
            timer.reset()    ;
            for ( seq_const_iterator i = m_pSeq->arrData.begin(); i != seqEnd; ++i ) {
                // Все ключи в arrData - уникальные, поэтому ошибок при вставке быть не должно
                CPPUNIT_ASSERT( testList.insert( i->key, i->value ) )    ;
            }
            CPPUNIT_MSG( "   Duration=" << timer.duration() ) ;

            // Multi-threading read-only test
            size_t nThreadCount = 4 ;
            CPPUNIT_MSG( "   Multi-reader test (thread count =" << nThreadCount << ") ...") ;
            CppUnitMini::ThreadPool pool( *this )   ;
            pool.add( new Thread( pool, testList ), nThreadCount ) ;
            pool.run()  ;

            Stat stat ;
            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it )
                stat += reinterpret_cast<Thread *>(*it)->m_Stat   ;
            stat /= nThreadCount    ;
            CPPUNIT_MSG( "     Iterate list (pre-inc) duration=" << stat.fPreInc ) ;
            CPPUNIT_MSG( "     Iterate list (post-inc) duration=" << stat.fPostInc ) ;
            CPPUNIT_MSG( "     [item search] duration=" << stat.fSearch ) ;

            // Проверям, что посетили все элементы
            {
                CPPUNIT_MSG( "   [plain array] Test that all items has been accessed by ordlist iter" ) ;
                timer.reset()    ;
                for ( seq_const_iterator i = m_pSeq->arrData.begin(); i != seqEnd; ++i ) {
                    CPPUNIT_ASSERT( i->value.nAccess == 2 * nThreadCount ) ;
                }
                CPPUNIT_MSG( "   Duration=" << timer.duration() ) ;
            }
        }

        template <class ORDLIST>
        void test()
        {
            CPPUNIT_MSG("Random sequence")        ;
            m_pSeq->makeRandomSortedSequence()  ;

            nciter_test<ORDLIST>()    ;

            CPPUNIT_MSG("Asc sorted sequence")        ;
            m_pSeq->makeAscSortedSequence()  ;
            nciter_test<ORDLIST>()    ;

            CPPUNIT_MSG("Desc sorted sequence")        ;
            m_pSeq->makeDescSortedSequence()  ;
            nciter_test<ORDLIST>()    ;
        }

        TEST_LIST(MichaelList_HP)    ;
        TEST_LIST(MichaelList_HRC)    ;
        TEST_LIST(MichaelList_PTB)    ;
#ifdef CDS_DWORD_CAS_SUPPORTED
        TEST_LIST(MichaelList_Tagged);
#endif
        TEST_LIST(MichaelList_NoGC)    ;

        //TEST_LIST(HarrisList_HP)    ;
        //TEST_LIST(HarrisList_HRC)    ;

        TEST_LIST(LazyList_HP)        ;
        TEST_LIST(LazyList_HRC)        ;
        TEST_LIST(LazyList_PTB)        ;
        TEST_LIST(LazyList_NoGC)    ;

        CPPUNIT_TEST_SUITE( OrdList_Nonconcurrent_iterator_MT )
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

    CPPUNIT_TEST_SUITE_REGISTRATION( OrdList_Nonconcurrent_iterator_MT );

} // namespace ordlist
