/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __UNIT_MAP_TEST_HEADER_H
#define __UNIT_MAP_TEST_HEADER_H

#include "cppunit/cppunit_proxy.h"

namespace map {

    namespace {
        static bool bFuncFindCalled = false ;
        static void funcFind( int& n, const int& itemVal )
        {
            bFuncFindCalled = true ;
            n = itemVal ;
        }

        struct FunctorFind {
            void operator ()( int& n, const int& itemVal )
            {
                funcFind( n, itemVal )  ;
            }
        };

        static bool bFuncEmplaceCalled = false ;
        static void funcEmplace( int& nValue, const int& nNewVal )
        {
            bFuncEmplaceCalled = true ;
            nValue = nNewVal    ;
        }

        struct FunctorEmplace {
            void operator ()( int& nValue, const int& nNewVal )
            {
                funcEmplace( nValue, nNewVal ) ;
            }
        };

        enum {
            ensureNotCalled = 0,        // not called
            ensureNewCreated = 1,       // new item has been created
            ensureModifyExisting = 2    // existing item has been modified
        };
        static int nFuncEnsureResult = ensureNotCalled    ;   
        static void funcEnsure( int& nValue, const int& nNewVal, bool bNew )
        {
            nFuncEnsureResult = bNew ? ensureNewCreated : ensureModifyExisting ;
            nValue = nNewVal ;
        }

        struct FunctorEnsure {
            void operator ()( int& nValue, const int& nNewVal, bool bNew )
            {
                funcEnsure( nValue, nNewVal, bNew ) ;
            }
        };
    }

    //
    // Test map operation in single thread mode
    //
    class MapTestHeader : public CppUnitMini::TestCase
    {
    protected:
        static int const s_nStartItem = -10 ;
        static int const s_nEndItem = 90    ;

    protected:
        // Map implementation is valid only with real item counter implementation

        template <class MAP>
        void testWithItemCounter()
        {
            typedef MAP Map ;
            Map     m       ;
            int     nValue  ;
            const int nDefValue = s_nStartItem - 10 ;

            CPPUNIT_CHECK( m.empty() ) ;
            CPPUNIT_CHECK( m.size() == 0 )     ;

            FunctorFind ftorFind    ;
            FunctorEmplace ftorEmplace  ;
            FunctorEnsure ftorEnsure    ;

            std::pair<bool, bool> bEnsure ;

            for ( int i = s_nStartItem; i <= s_nEndItem; ++i ) {
                CPPUNIT_CHECK( !m.find(i) )        ;

                nValue = nDefValue  ;
                bFuncFindCalled = false     ;
                CPPUNIT_CHECK( !m.find(i, nValue, funcFind )) ;
                CPPUNIT_CHECK( nValue == nDefValue )       ;
                CPPUNIT_CHECK( !bFuncFindCalled )          ;
                CPPUNIT_CHECK( !m.find(i, nValue, ftorFind )) ;
                CPPUNIT_CHECK( nValue == nDefValue )       ;
                CPPUNIT_CHECK( !bFuncFindCalled )          ;

                // emplace unknown value test
                bFuncEmplaceCalled = false ;
                CPPUNIT_CHECK( !m.emplace( i, nValue, funcEmplace )) ;
                CPPUNIT_CHECK( !bFuncEmplaceCalled )       ;
                CPPUNIT_CHECK( nValue == nDefValue )       ;
                CPPUNIT_CHECK( !m.emplace( i, nValue, ftorEmplace )) ;
                CPPUNIT_CHECK( !bFuncEmplaceCalled )       ;
                CPPUNIT_CHECK( nValue == nDefValue )       ;

                for ( int j = s_nStartItem; j < i; ++j )
                    CPPUNIT_ASSERT_EX( m.find(j), "Before insert: all inserted items must be in map, item " << j << " not found (i=" << i << ")" ) ;

                CPPUNIT_CHECK( m.insert(i, i * s_nEndItem * 10 ) )    ;
                CPPUNIT_CHECK( m.find(i) )                 ;

                for ( int j = s_nStartItem; j <= i; ++j )
                    CPPUNIT_ASSERT_EX( m.find(j), "After insert: all inserted items must be in map, item " << j << " not found (i=" << i << ")" ) ;

                nValue = nDefValue  ;
                bFuncFindCalled = false     ;
                CPPUNIT_CHECK( m.find(i, nValue, funcFind) )           ;
                CPPUNIT_CHECK( nValue == i * s_nEndItem * 10 )       ;
                CPPUNIT_CHECK( bFuncFindCalled )       ;
                bFuncFindCalled = false     ;
                CPPUNIT_CHECK( m.find(i, nValue, ftorFind) )           ;
                CPPUNIT_CHECK( nValue == i * s_nEndItem * 10 )       ;
                CPPUNIT_CHECK( bFuncFindCalled )       ;

                CPPUNIT_CHECK( !m.insert(i, i * s_nEndItem * 20 ) )   ;
                nValue = nDefValue                          ;
                bFuncFindCalled = false     ;
                CPPUNIT_CHECK( m.find(i, nValue, funcFind) )         ;
                CPPUNIT_CHECK( nValue == i * s_nEndItem * 10 )        ;
                CPPUNIT_CHECK( bFuncFindCalled )       ;
                bFuncFindCalled = false     ;
                CPPUNIT_CHECK( m.find(i, nValue, ftorFind) )           ;
                CPPUNIT_CHECK( nValue == i * s_nEndItem * 10 )       ;
                CPPUNIT_CHECK( bFuncFindCalled )                    ;

                // emplace test
                bFuncEmplaceCalled = false ;
                CPPUNIT_CHECK( m.emplace( i, i * s_nEndItem * 20, funcEmplace )) ;
                CPPUNIT_CHECK( bFuncEmplaceCalled )    ;
                bFuncFindCalled = false     ;
                CPPUNIT_CHECK( m.find(i, nValue, funcFind) )           ;
                CPPUNIT_CHECK( nValue == i * s_nEndItem * 20 )       ;
                CPPUNIT_CHECK( bFuncFindCalled )       ;
                bFuncEmplaceCalled = false ;
                CPPUNIT_CHECK( m.emplace( i, i * s_nEndItem * 10, ftorEmplace )) ;
                CPPUNIT_CHECK( bFuncEmplaceCalled )    ;
                bFuncFindCalled = false     ;
                CPPUNIT_CHECK( m.find(i, nValue, funcFind) )           ;
                CPPUNIT_CHECK( nValue == i * s_nEndItem * 10 )       ;
                CPPUNIT_CHECK( bFuncFindCalled )       ;

                // ensure test, existing key
                nFuncEnsureResult = ensureNotCalled ;
                bEnsure = m.ensure( i, i * s_nEndItem * 20, funcEnsure )  ;
                CPPUNIT_CHECK( bEnsure.first )         ;
                CPPUNIT_CHECK( !bEnsure.second )        ;
                CPPUNIT_CHECK( nFuncEnsureResult == ensureModifyExisting )     ;
                bFuncFindCalled = false     ;
                CPPUNIT_CHECK( m.find(i, nValue, funcFind) )           ;
                CPPUNIT_CHECK( nValue == i * s_nEndItem * 20 )       ;
                CPPUNIT_CHECK( bFuncFindCalled )       ;
                nFuncEnsureResult = ensureNotCalled ;
                bEnsure = m.ensure( i, i * s_nEndItem * 10, ftorEnsure ) ;
                CPPUNIT_CHECK( bEnsure.first )         ;
                CPPUNIT_CHECK( !bEnsure.second )        ;
                CPPUNIT_CHECK( nFuncEnsureResult == ensureModifyExisting )    ;
                bFuncFindCalled = false     ;
                CPPUNIT_CHECK( m.find(i, nValue, funcFind) )           ;
                CPPUNIT_CHECK( nValue == i * s_nEndItem * 10 )       ;
                CPPUNIT_CHECK( bFuncFindCalled )       ;

                // ensure test, new key
                CPPUNIT_CHECK( m.find( i ))    ;
                nFuncEnsureResult = ensureNotCalled ;
                bEnsure = m.ensure( i + s_nEndItem * 10, i * s_nEndItem * 100, funcEnsure ) ;
                CPPUNIT_CHECK( bEnsure.first )         ;
                CPPUNIT_CHECK( bEnsure.second )        ;
                CPPUNIT_CHECK( nFuncEnsureResult == ensureNewCreated )    ;
                CPPUNIT_CHECK( m.find( i + s_nEndItem * 10 ))    ;
                CPPUNIT_CHECK( m.find( i ))    ;
                CPPUNIT_CHECK( m.erase( i + s_nEndItem * 10 ))   ;
                CPPUNIT_CHECK( m.find( i ))    ;
                CPPUNIT_CHECK( !m.find( i + s_nEndItem * 10 ))    ;
                nFuncEnsureResult = ensureNotCalled ;
                bEnsure = m.ensure( i + s_nEndItem * 10, i * s_nEndItem * 100, ftorEnsure ) ;
                CPPUNIT_CHECK( bEnsure.first )         ;
                CPPUNIT_CHECK( bEnsure.second )        ;
                CPPUNIT_CHECK( nFuncEnsureResult == ensureNewCreated )    ;
                CPPUNIT_CHECK( m.find( i ))    ;
                CPPUNIT_CHECK( m.find( i + s_nEndItem * 10 ))    ;
                CPPUNIT_CHECK( m.erase( i + s_nEndItem * 10 ))   ;
                CPPUNIT_CHECK( m.find( i ))    ;
                CPPUNIT_CHECK( !m.find( i + s_nEndItem * 10 ))    ;

                CPPUNIT_CHECK( m.size() == (size_t) (i - s_nStartItem + 1) )    ;
                CPPUNIT_CHECK( !m.empty() ) ;
            }

            CPPUNIT_CHECK( !m.empty() )    ;

            for ( int i = s_nStartItem; i <= s_nEndItem; ++i ) {
                CPPUNIT_CHECK_EX( m.find(i), i )                 ;
                CPPUNIT_CHECK( !m.find( i + s_nEndItem*10) )           ;
                nValue = nDefValue                          ;
                bFuncFindCalled = false ;
                CPPUNIT_CHECK( m.find(i, nValue, funcFind ) )         ;
                CPPUNIT_CHECK( bFuncFindCalled )  ;
                CPPUNIT_CHECK( nValue == i * s_nEndItem * 10 )        ;
                nValue = nDefValue                          ;
                bFuncFindCalled = false ;
                CPPUNIT_CHECK( !m.find(i + s_nEndItem * 10, nValue, ftorFind ) )   ;
                CPPUNIT_CHECK( !bFuncFindCalled )          ;
                CPPUNIT_CHECK( nValue == nDefValue )       ;

                CPPUNIT_CHECK( m.size() == size_t((s_nEndItem - s_nStartItem + 1) - (i - s_nStartItem)) )       ;
                CPPUNIT_CHECK( !m.empty() )            ;

                CPPUNIT_CHECK( !m.erase(i + s_nEndItem*10))       ;

                for ( int j = i; j <= s_nEndItem; ++j )
                    CPPUNIT_ASSERT_EX( m.find(j), "Before erase: all non-erased items must be in map, item " << j << " not found" ) ;
                CPPUNIT_CHECK( m.erase(i))             ;
                for ( int j = i + 1; j <= s_nEndItem; ++j )
                    CPPUNIT_ASSERT_EX( m.find(j), "After erase: all non-erased items must be in map, item " << j << " not found" ) ;
                CPPUNIT_CHECK( !m.erase(i))             ;

                CPPUNIT_CHECK( m.size() == size_t((s_nEndItem - s_nStartItem + 1) - (i - s_nStartItem) - 1) )       ;

                CPPUNIT_CHECK( !m.find(i) )                ;
                nValue = nDefValue                         ;
                bFuncFindCalled = false ;
                CPPUNIT_CHECK( !m.find(i, nValue, funcFind) )        ;
                CPPUNIT_CHECK( nValue == nDefValue )       ;
                CPPUNIT_CHECK( !bFuncFindCalled )          ;
            }

            CPPUNIT_CHECK( m.size() == 0 );
            CPPUNIT_CHECK( m.empty() )    ;
//            m.clear()   ;
//          CPPUNIT_CHECK( m.empty() )     ;
        }

    protected:
        void MichaelHash()  ;
        void MichaelHash_Michael_hp()           ;
        void MichaelHash_Michael_hrc()          ;
        void MichaelHash_Michael_ptb()          ;
        void MichaelHash_Michael_tagged()       ;
        void MichaelHash_Lazy_hp()              ;
        void MichaelHash_Lazy_hrc()             ;
        void MichaelHash_Lazy_ptb()             ;

        void SplitList()                        ;
        void SplitList_Static()                 ;
        void SplitList_Dynamic_Michael_hp()     ;
        void SplitList_Static_Michael_hp()      ;
        void SplitList_Dynamic_Michael_hrc()    ;
        void SplitList_Static_Michael_hrc()     ;
        void SplitList_Dynamic_Michael_ptb()    ;
        void SplitList_Static_Michael_ptb()     ;
        void SplitList_Dynamic_Michael_tagged() ;
        void SplitList_Static_Michael_tagged()  ;
        void SplitList_Dynamic_Lazy_hp()        ;
        void SplitList_Static_Lazy_hp()         ;
        void SplitList_Dynamic_Lazy_hrc()       ;
        void SplitList_Static_Lazy_hrc()        ;
        void SplitList_Dynamic_Lazy_ptb()       ;
        void SplitList_Static_Lazy_ptb()        ;

    CPPUNIT_TEST_SUITE(MapTestHeader);
        CPPUNIT_TEST(MichaelHash)                   ;
        CPPUNIT_TEST(MichaelHash_Michael_hp)        ;
        CPPUNIT_TEST(MichaelHash_Michael_hrc)       ;
        CPPUNIT_TEST(MichaelHash_Michael_ptb)       ;
        CPPUNIT_TEST(MichaelHash_Michael_tagged)    ;
        CPPUNIT_TEST(MichaelHash_Lazy_hp)           ;
        CPPUNIT_TEST(MichaelHash_Lazy_hrc)          ;
        CPPUNIT_TEST(MichaelHash_Lazy_ptb)          ;

        CPPUNIT_TEST(SplitList) ;
        CPPUNIT_TEST(SplitList_Static) ;
        CPPUNIT_TEST(SplitList_Dynamic_Michael_hp)      ;
        CPPUNIT_TEST(SplitList_Static_Michael_hp)       ;
        CPPUNIT_TEST(SplitList_Dynamic_Michael_hrc)     ;
        CPPUNIT_TEST(SplitList_Static_Michael_hrc)      ;
        CPPUNIT_TEST(SplitList_Dynamic_Michael_ptb)     ;
        CPPUNIT_TEST(SplitList_Static_Michael_ptb)      ;
        CPPUNIT_TEST(SplitList_Dynamic_Michael_tagged)  ;
        CPPUNIT_TEST(SplitList_Static_Michael_tagged)   ;
        CPPUNIT_TEST(SplitList_Dynamic_Lazy_hp)  ;
        CPPUNIT_TEST(SplitList_Static_Lazy_hp)   ;
        CPPUNIT_TEST(SplitList_Dynamic_Lazy_hrc) ;
        CPPUNIT_TEST(SplitList_Static_Lazy_hrc)  ;
        CPPUNIT_TEST(SplitList_Dynamic_Lazy_ptb) ;
        CPPUNIT_TEST(SplitList_Static_Lazy_ptb)  ;

    CPPUNIT_TEST_SUITE_END();

    };

    class MapTestHeaderNoGC : public CppUnitMini::TestCase
    {
    protected:
        static int const s_nItemCount = 100 ;
        //typedef CppUnitMini::TestCase       Base    ;

    protected:
        // Map implementation is valid only with real item counter implementation
        template <class MAP>
        void testWithItemCounter()
        {
            typedef MAP Map ;
            Map     m       ;
            const int nDefValue = -1    ;
            int * pVal      ;

            FunctorEmplace ftorEmplace  ;
            FunctorEnsure ftorEnsure    ;

            std::pair<bool, bool> pairRet ;

            CPPUNIT_CHECK( m.empty() ) ;
            CPPUNIT_CHECK( m.size() == 0 )     ;

            for ( int i = 1; i <= s_nItemCount; ++i ) {
                CPPUNIT_CHECK( !m.find(i) )        ;
                CPPUNIT_CHECK( m.get(i) == NULL ) ;
                CPPUNIT_CHECK( m.insert(i, i * s_nItemCount * 10 ) )    ;
                CPPUNIT_CHECK( m.find(i) )                 ;
                pVal = m.get(i) ;
                CPPUNIT_CHECK( pVal != NULL )              ;
                if ( pVal != NULL ) {
                    CPPUNIT_CHECK( *pVal == i * s_nItemCount * 10 )    ;
                }

                CPPUNIT_CHECK( !m.insert(i, i * s_nItemCount * 20 ) )   ;
                pVal = m.get(i) ;
                CPPUNIT_CHECK( pVal != NULL )              ;
                if ( pVal != NULL ) {
                    CPPUNIT_CHECK( *pVal == i * s_nItemCount * 10 )    ;
                }

                // ensure test
                nFuncEnsureResult = ensureNotCalled ;
                pairRet = m.ensure(i, i * s_nItemCount * 20, funcEnsure ) ;
                CPPUNIT_CHECK( pairRet.first )     ;
                CPPUNIT_CHECK( !pairRet.second )   ;
                CPPUNIT_CHECK( nFuncEnsureResult == ensureModifyExisting ) ;
                pVal = m.get(i) ;
                CPPUNIT_CHECK( pVal != NULL )              ;
                if ( pVal != NULL ) {
                    CPPUNIT_CHECK( *pVal == i * s_nItemCount * 20 )    ;
                }
                nFuncEnsureResult = ensureNotCalled ;
                pairRet = m.ensure(i, i * s_nItemCount * 10, ftorEnsure ) ;
                CPPUNIT_CHECK( pairRet.first )     ;
                CPPUNIT_CHECK( !pairRet.second )   ;
                CPPUNIT_CHECK( nFuncEnsureResult == ensureModifyExisting ) ;
                pVal = m.get(i) ;
                CPPUNIT_CHECK( pVal != NULL )              ;
                if ( pVal != NULL ) {
                    CPPUNIT_CHECK( *pVal == i * s_nItemCount * 10 )    ;
                }

                // emplace test
                bFuncEmplaceCalled = false ;
                CPPUNIT_CHECK( m.emplace( i, i * s_nItemCount * 20, funcEmplace )) ;
                CPPUNIT_CHECK( bFuncEmplaceCalled )    ;
                bFuncFindCalled = false     ;
                pVal = m.get(i) ;
                CPPUNIT_CHECK( pVal != NULL )              ;
                if ( pVal != NULL ) {
                    CPPUNIT_CHECK( *pVal == i * s_nItemCount * 20 )    ;
                }
                bFuncEmplaceCalled = false ;
                CPPUNIT_CHECK( m.emplace( i, i * s_nItemCount * 10, ftorEmplace )) ;
                CPPUNIT_CHECK( bFuncEmplaceCalled )    ;
                bFuncFindCalled = false     ;
                pVal = m.get(i) ;
                CPPUNIT_CHECK( pVal != NULL )              ;
                if ( pVal != NULL ) {
                    CPPUNIT_CHECK( *pVal == i * s_nItemCount * 10 )    ;
                }

                CPPUNIT_CHECK( m.size() == (size_t) i )             ;
                CPPUNIT_CHECK( !m.empty() ) ;
            }

            CPPUNIT_CHECK( !m.empty() )    ;
            //m.clear()   ;
            //CPPUNIT_CHECK( m.empty() )    ;
        }

    protected:
        void MichaelHash_Michael()           ;
        void MichaelHash_Lazy()              ;

        void SplitList_Dynamic_Michael()     ;
        void SplitList_Static_Michael()      ;
        void SplitList_Dynamic_Lazy()        ;
        void SplitList_Static_Lazy()         ;


        CPPUNIT_TEST_SUITE(MapTestHeaderNoGC);
            CPPUNIT_TEST(MichaelHash_Michael)        ;
            CPPUNIT_TEST(MichaelHash_Lazy)           ;

            CPPUNIT_TEST(SplitList_Dynamic_Michael)  ;
            CPPUNIT_TEST(SplitList_Static_Michael)   ;
            CPPUNIT_TEST(SplitList_Dynamic_Lazy)  ;
            CPPUNIT_TEST(SplitList_Static_Lazy)   ;
        CPPUNIT_TEST_SUITE_END();

    };

} // namespace map

#endif // #ifndef __UNIT_MAP_TEST_HEADER_H
