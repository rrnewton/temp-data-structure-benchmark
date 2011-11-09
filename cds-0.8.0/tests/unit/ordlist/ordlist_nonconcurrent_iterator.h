/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __UNIT_ORDLIST_NONCONCURRENT_ITERATOR_H
#define __UNIT_ORDLIST_NONCONCURRENT_ITERATOR_H

#include "cppunit/cppunit_proxy.h"
#include "nonconcurrent_iterator_sequence.h"

#include <cds/os/timer.h>


namespace ordlist { namespace nonconcurrent_iterator {

    using map::nonconcurrent_iterator::Sequence    ;

    class Test: public CppUnitMini::TestCase
    {
        typedef CppUnitMini::TestCase    Base        ;

    protected:
        typedef Sequence::const_iterator    seq_const_iterator    ;
        Sequence *    m_pSeq    ;
    public:
        void initTestSequence() {
            assert( m_pSeq == NULL )    ;

            m_pSeq = new Sequence( 10000 )    ;
            m_pSeq->generateSequence()            ;
            CPPUNIT_MSG( "Sequence size=" << m_pSeq->arrData.capacity())   ;
        }

        void cleanTestSequence() {
            delete m_pSeq    ;
            m_pSeq = NULL    ;
        }

        Test(): m_pSeq( NULL ) {}

    protected:
        template <class ORDLIST>
        void preIncPass( ORDLIST& list )
        {
            typedef typename ORDLIST::nonconcurrent_iterator    ncIterator  ;

            ncIterator itEnd = list.nc_end()    ;
            //size_t nErrorCount = 0    ;
            for ( ncIterator it = list.nc_begin(); it != itEnd; ++it ) {
                CPPUNIT_ASSERT( it.key() == it.value().keyControl ) ;
                it.value().pOrigItem->nAccess.inc() ;
            }
        }

        template <class ORDLIST>
        void postIncPass( ORDLIST& list )
        {
            typedef typename ORDLIST::nonconcurrent_iterator    ncIterator  ;

            ncIterator itEnd = list.nc_end()    ;
            //size_t nErrorCount = 0    ;
            for ( ncIterator it = list.nc_begin(); it != itEnd; it++ ) {
                CPPUNIT_ASSERT( it.key() == it.value().keyControl ) ;
                it.value().pOrigItem->nAccess.inc() ;
            }
        }

        template <class ORDLIST>
        void searchPass( ORDLIST& list )
        {
            seq_const_iterator seqEnd = m_pSeq->arrData.end()    ;
            for ( seq_const_iterator i = m_pSeq->arrData.begin(); i != seqEnd; ++i ) {
                CPPUNIT_ASSERT( list.find( i->key ) )    ;
            }
        }

        template <class ORDLIST>
        void nciter_test()
        {
            typedef typename ORDLIST::nonconcurrent_iterator    ncIterator  ;

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

            // Обходим list (используется pre-increment)
            {
                CPPUNIT_MSG( "   [nonconcurrent_iterator] Iterate list (pre-inc)..." )  ;
                timer.reset()    ;
                preIncPass( testList )  ;
                CPPUNIT_MSG( "   Duration=" << timer.duration() ) ;
            }

            // Обходим list (используется post-increment)
            {
                CPPUNIT_MSG( "   [nonconcurrent_iterator] Iterate list (post-inc)..." ) ;
                timer.reset()    ;
                postIncPass( testList ) ;
                CPPUNIT_MSG( "   Duration=" << timer.duration() ) ;
            }

            // Item search
            {
                CPPUNIT_MSG( "   [item search]..." ) ;
                timer.reset()    ;
                searchPass( testList )  ;
                CPPUNIT_MSG( "   Duration=" << timer.duration() ) ;
            }

            // Проверям, что посетили все элементы
            {
                CPPUNIT_MSG( "   [plain array] Test that all items has been accessed by ordlist iter" ) ;
                timer.reset()    ;
                for ( seq_const_iterator i = m_pSeq->arrData.begin(); i != seqEnd; ++i ) {
                    CPPUNIT_ASSERT( i->value.nAccess == 2 ) ;
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
    } ;

} } // namespace ordlist::nonconcurrent_iterator

#endif // #ifndef __UNIT_ORDLIST_NONCONCURRENT_ITERATOR_H
