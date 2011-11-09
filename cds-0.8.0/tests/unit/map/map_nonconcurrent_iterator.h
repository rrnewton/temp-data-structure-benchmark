/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __UNIT_MAP_NONCONCURRENT_ITERATOR_H
#define __UNIT_MAP_NONCONCURRENT_ITERATOR_H

#include "cppunit/cppunit_proxy.h"

#include "nonconcurrent_iterator_sequence.h"
#include <cds/os/timer.h>

namespace map { namespace nonconcurrent_iterator {
    class Test: public CppUnitMini::TestCase
    {
        typedef CppUnitMini::TestCase    Base        ;

    protected:
        typedef Sequence::TDataArray::const_iterator    seq_const_iterator    ;
        Sequence *    m_pSeq    ;

    public:
        size_t      m_nItemCount    ;

    public:
        bool ensureTestSequence() {
            if ( m_pSeq == NULL ) {
                initTestSequence()  ;
                return true         ;
            }
            return false;
        }

        void initTestSequence() {
            assert( m_pSeq == NULL )    ;

            m_pSeq = new Sequence( m_nItemCount )    ;
            m_pSeq->generateSequence()            ;
            CPPUNIT_MSG( "Map size=" << m_pSeq->arrData.capacity())   ;
        }

        void cleanTestSequence() {
            delete m_pSeq    ;
            m_pSeq = NULL    ;
        }

        Test()
            : m_nItemCount( 1000000 )
            , m_pSeq( NULL )
            {}

    protected:

        template <class MAP>
        void preIncPass( MAP& testMap )
        {
            typedef typename MAP::nonconcurrent_iterator    ncIterator  ;

            ncIterator itEnd = testMap.nc_end()    ;
            for ( ncIterator it = testMap.nc_begin(); it != itEnd; ++it ) {
                CPPUNIT_ASSERT( it.key() == it.value().keyControl ) ;
                it.value().pOrigItem->nAccess++;
            }
        }

        template <class MAP>
        void postIncPass( MAP& testMap )
        {
            typedef typename MAP::nonconcurrent_iterator    ncIterator  ;

            ncIterator itEnd = testMap.nc_end()    ;
            //size_t nErrorCount = 0    ;
            for ( ncIterator it = testMap.nc_begin(); it != itEnd; it++ ) {
                CPPUNIT_ASSERT( it.key() == it.value().keyControl ) ;
                it.value().pOrigItem->nAccess++ ;
            }
        }

        template <class MAP>
        void searchPass( MAP& testMap )
        {
            seq_const_iterator seqEnd = m_pSeq->arrData.end()    ;
            for ( seq_const_iterator i = m_pSeq->arrData.begin(); i != seqEnd; ++i ) {
                CPPUNIT_ASSERT( testMap.find( i->key ) )    ;
            }
        }

        template <class MAP>
        void nciter_test( size_t nLoadFactor )
        {
            typedef typename MAP::nonconcurrent_iterator    ncIterator  ;

            MAP  testMap( m_pSeq->arrData.capacity(), nLoadFactor )    ;
            cds::OS::Timer    timer    ;

            CPPUNIT_MSG( "Load factor=" << nLoadFactor )   ;

            seq_const_iterator seqEnd = m_pSeq->arrData.end()    ;
            m_pSeq->clearAccess()   ;

            // Fill the map
            CPPUNIT_MSG( "   Fill map...") ;
            timer.reset()    ;
            for ( seq_const_iterator i = m_pSeq->arrData.begin(); i != seqEnd; ++i ) {
                // Все ключи в arrData - уникальные, поэтому ошибок при вставке быть не должно
                CPPUNIT_ASSERT( testMap.insert( i->key, i->value ) )    ;
            }
            CPPUNIT_MSG( "   Duration=" << timer.duration() ) ;

            // Обходим map (используется pre-increment)
            {
                CPPUNIT_MSG( "   [nonconcurrent_iterator] Iterate map (pre-inc)..." )  ;

                timer.reset()    ;
                preIncPass( testMap )   ;
                CPPUNIT_MSG( "   Duration=" << timer.duration() ) ;
            }

            // Обходим map (используется post-increment)
            {
                CPPUNIT_MSG( "   [nonconcurrent_iterator] Iterate map (post-inc)..." ) ;
                timer.reset()    ;
                postIncPass( testMap )  ;
                CPPUNIT_MSG( "   Duration=" << timer.duration() ) ;
            }

            // Item search
            {
                CPPUNIT_MSG( "   [item search]..." ) ;
                timer.reset()    ;
                searchPass( testMap )   ;
                CPPUNIT_MSG( "   Duration=" << timer.duration() ) ;
            }

            // Проверям, что посетили все элементы
            {
                CPPUNIT_MSG( "   [plain array] Test that all items has been accessed by map iterator" ) ;
                timer.reset()    ;
                for ( seq_const_iterator i = m_pSeq->arrData.begin(); i != seqEnd; ++i ) {
                    CPPUNIT_ASSERT( i->value.nAccess == 2 ) ;
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
            nciter_test<MAP>( 1 )    ;
            nciter_test<MAP>( 2 )    ;
            nciter_test<MAP>( 4 )    ;
            nciter_test<MAP>( 8 )    ;

            CPPUNIT_MSG("Asc sorted sequence")        ;
            m_pSeq->makeAscSortedSequence()  ;
            nciter_test<MAP>( 1 )    ;
            nciter_test<MAP>( 2 )    ;
            nciter_test<MAP>( 4 )    ;
            nciter_test<MAP>( 8 )    ;

            CPPUNIT_MSG("Desc sorted sequence")        ;
            m_pSeq->makeDescSortedSequence()  ;
            nciter_test<MAP>( 1 )    ;
            nciter_test<MAP>( 2 )    ;
            nciter_test<MAP>( 4 )    ;
            nciter_test<MAP>( 8 )    ;

            if ( bMustClear )
                cleanTestSequence() ;
        }
    };

} } // namespace map::nonconcurrent_iterator

#endif // #ifndef __UNIT_MAP_NONCONCURRENT_ITERATOR_H
