/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


// Larson allocator test

#include "alloc/michael_allocator.h"

#include <cds/os/timer.h>
#include <cds/os/topology.h>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>

#include "cppunit/thread.h"

namespace memory {

    static size_t s_nMaxThreadCount = 32        ;
    static unsigned int s_nMinBlockSize = 8     ;
    static unsigned int s_nMaxBlockSize = 1024  ;
    static size_t s_nBlocksPerThread = 1000     ;
    static size_t s_nPassCount = 100000         ;

    static size_t s_nPassPerThread  ;

    static boost::mt19937   s_rndGen    ;

    template <typename T>
    static inline T random( T nMin, T nMax)
    {
        boost::uniform_int<T> dist(nMin, nMax);
        boost::variate_generator<boost::mt19937&, boost::uniform_int<T> > gen(s_rndGen, dist);
        return gen();
    }

    static inline unsigned int random()
    {
        return random( s_nMinBlockSize, s_nMaxBlockSize )   ;
    }

#    define TEST_ALLOC(X, CLASS)    void X() { test< CLASS >(false)    ; }
#    define TEST_ALLOC_STAT(X, CLASS)    void X() { test< CLASS >(true)    ; }

    /*
        In this test, initially one thread allocates and frees random
        sized blocks (s_nMinBlockSize to s_nMaxBlockSize bytes) in random order, then an
        equal number of blocks (s_nBlocksPerThread) is handed over to each of the
        remaining threads. In the parallel phase, each thread randomly selects a block and 
        frees it, then allocates a new random-sized block in its place. 
        The benchmark measures the duration of s_nPassCount free/malloc pairs 
        during the parallel phase. Larson captures the robustness of malloc’s latency 
        and scalability under irregular allocation patterns with respect to block-size 
        and order of deallocation over a long period of time.    
    */
    class Larson: public CppUnitMini::TestCase
    {
        typedef char ** thread_data     ;

        thread_data *   m_aThreadData   ;


        template <class ALLOC>
        class Thread: public CppUnitMini::TestThread
        {
            ALLOC&      m_Alloc ;
            typedef typename ALLOC::value_type value_type   ;
    
            virtual Thread *    clone()
            {
                return new Thread( *this )    ;
            }
        public:
            thread_data     m_arr   ;

        public:
            Thread( CppUnitMini::ThreadPool& pool, ALLOC& a )
                : CppUnitMini::TestThread( pool )
                , m_Alloc( a )
            {}
            Thread( Thread& src )
                : CppUnitMini::TestThread( src )
                , m_Alloc( src.m_Alloc )
            {}

            Larson&  getTest()
            {
                return reinterpret_cast<Larson&>( m_Pool.m_Test )   ;
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            virtual void test()
            {
                for ( size_t nPass = 0; nPass < s_nPassPerThread; ++nPass ) {
                    size_t nItem = random( size_t(1), s_nBlocksPerThread ) - 1 ;
                    m_Alloc.deallocate( reinterpret_cast<value_type *>(m_arr[nItem]), 1 )  ;
                    m_arr[nItem] = reinterpret_cast<char *>( m_Alloc.allocate( random(), NULL ))  ;
                    CPPUNIT_ASSERT( (reinterpret_cast<cds::uptr_atomic_t>(m_arr[nItem]) & (ALLOC::alignment - 1)) == 0 )  ;
                }
            }
        };

        template <class ALLOC>
        void test( size_t nThreadCount )
        {
            ALLOC alloc ;

            CPPUNIT_MSG( "Thread count=" << nThreadCount )      ;
            CPPUNIT_MSG("Initialize data..." ) ;

            s_nPassPerThread = s_nPassCount / nThreadCount      ;

            size_t nThread  ;
            m_aThreadData = new thread_data[ nThreadCount ] ;
            for ( nThread = 0; nThread < nThreadCount; ++nThread ) {
                thread_data thData 
                    = m_aThreadData[nThread] 
                    = new char *[ s_nBlocksPerThread ]    ;
                    for ( size_t i = 0; i < s_nBlocksPerThread; ++i ) {
                        thData[i] = reinterpret_cast<char *>( alloc.allocate( random(), NULL ))  ;
                        CPPUNIT_ASSERT( (reinterpret_cast<cds::uptr_atomic_t>(thData[i]) & (ALLOC::alignment - 1)) == 0 )  ;
                    }
            }

            CppUnitMini::ThreadPool pool( *this )   ;
            pool.add( new Thread<ALLOC>( pool, alloc ), nThreadCount ) ;
            nThread = 0  ;
            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it )
                static_cast<Thread<ALLOC> *>(*it)->m_arr = m_aThreadData[nThread++]    ;

            cds::OS::Timer    timer    ;
            pool.run()  ;
            CPPUNIT_MSG( "  Duration=" << pool.avgDuration() ) ;

            for ( nThread = 0; nThread < nThreadCount; ++nThread ) {
                thread_data thData = m_aThreadData[nThread] ;
                for ( size_t i = 0; i < s_nBlocksPerThread; ++i ) {
                    alloc.deallocate( reinterpret_cast<typename ALLOC::value_type *>(thData[i]), 1 )   ;
                }
                delete [] thData    ;
            }
            delete [] m_aThreadData ;
        }

        template <class ALLOC>
        void test( bool bStat )
        {
            CPPUNIT_MSG( "Block size=" << s_nMinBlockSize << "-" << s_nMaxBlockSize 
                << ", block count per thread=" << s_nBlocksPerThread << ", pass count=" << s_nPassCount ) ;

            for ( size_t nThreadCount = 2; nThreadCount <= s_nMaxThreadCount; nThreadCount *= 2 ) {
                summary_stat stBegin    ;
                if ( bStat )
                    ALLOC::stat(stBegin)    ;

                test<ALLOC>( nThreadCount ) ;

                summary_stat    stEnd   ;
                if ( bStat ) {
                    ALLOC::stat( stEnd ) ;

                    std::cout << "\nStatistics:\n"
                        << stEnd
                        ;
                    stEnd -= stBegin    ;
                    std::cout << "\nDelta statistics:\n"
                        << stEnd
                        ;
                }
            }
        }

        void setUpParams( const CppUnitMini::TestCfg& cfg ) 
        {
            s_nPassCount = cfg.getULong( "PassCount", 100000 )      ;
            s_nMinBlockSize = cfg.getUInt( "MinBlockSize", 8 )      ;
            s_nMaxBlockSize = cfg.getUInt( "MaxBlockSize", 1024 )   ;
            s_nBlocksPerThread = cfg.getUInt( "BlocksPerThread", 10000 ) ;
            s_nMaxThreadCount = cfg.getUInt( "MaxThreadCount", 32 ) ;
            if ( s_nMaxThreadCount == 0 )
                s_nMaxThreadCount = cds::OS::topology::processor_count() * 2    ;
            if ( s_nMaxThreadCount < 2 )
                s_nMaxThreadCount = 2   ;
            if ( s_nPassCount < s_nBlocksPerThread )
                s_nBlocksPerThread = s_nPassCount   ;
        }

        typedef MichaelAlignHeap_Stat<int, 64>      t_MichaelAlignHeap_Stat     ;
        typedef MichaelAlignHeap_NoStat<int,64>     t_MichaelAlignHeap_NoStat   ;
        typedef system_aligned_allocator<int, 64>   t_system_aligned_allocator  ;

        TEST_ALLOC_STAT( michael_heap_stat,      MichaelHeap_Stat<int> )
        TEST_ALLOC( michael_heap_nostat,    MichaelHeap_NoStat<int> )
        TEST_ALLOC( std_alloc,              std_allocator<int> )

        TEST_ALLOC_STAT( michael_alignheap_stat,     t_MichaelAlignHeap_Stat )
        TEST_ALLOC( michael_alignheap_nostat,   t_MichaelAlignHeap_NoStat )
        TEST_ALLOC( system_aligned_alloc,       t_system_aligned_allocator )

        CPPUNIT_TEST_SUITE( Larson )
            CPPUNIT_TEST( michael_heap_stat )
            CPPUNIT_TEST( michael_heap_nostat )
            CPPUNIT_TEST( std_alloc )

            CPPUNIT_TEST( system_aligned_alloc )
            CPPUNIT_TEST( michael_alignheap_stat )
            CPPUNIT_TEST( michael_alignheap_nostat )

        CPPUNIT_TEST_SUITE_END();
    };

}   // namespace memory
CPPUNIT_TEST_SUITE_REGISTRATION( memory::Larson ) ;
