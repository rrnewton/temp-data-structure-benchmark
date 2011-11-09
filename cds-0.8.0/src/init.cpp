/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include <cds/atomic.h>
#include <cds/os/topology.h>
#include <cds/threading/model.h>

#if CDS_OS_INTERFACE == CDS_OSI_WINDOWS
#   if CDS_COMPILER == CDS_COMPILER_MSVC
#       include <cds/threading/details/msvc_manager.h>
#   endif
#   include <cds/threading/details/wintls_manager.h>
#else   // CDS_OS_INTERFACE != CDS_OSI_WINDOWS
#   if CDS_COMPILER == CDS_COMPILER_GCC
#       include <cds/threading/details/gcc_manager.h>
#   endif
#   include <cds/threading/details/pthread_manager.h>
#endif

namespace cds {

    CDS_EXPORT_API size_t volatile threading::ThreadData::s_nLastUsedProcNo = 0    ;
    CDS_EXPORT_API size_t threading::ThreadData::s_nProcCount = 1  ;

#if CDS_OS_INTERFACE == CDS_OSI_WINDOWS
    CDS_EXPORT_API DWORD cds::threading::wintls::Manager::Holder::m_key = TLS_OUT_OF_INDEXES ;
#else
    pthread_key_t threading::pthread::Manager::Holder::m_key ;
#endif


    static size_t s_nInitCallCount = 0  ;

    namespace {
        static inline void init_thread_manager()
        {
#   if CDS_OS_INTERFACE == CDS_OSI_WINDOWS
#       if CDS_COMPILER == CDS_COMPILER_MSVC
            cds::threading::msvc::Manager::init() ;
#       endif
        cds::threading::wintls::Manager::init() ;
#   else
#       if CDS_COMPILER == CDS_COMPILER_GCC
            cds::threading::gcc::Manager::init() ;
#       endif
        cds::threading::pthread::Manager::init() ;
#   endif
        }

        static inline void fini_thread_manager()
        {
#   if CDS_OS_INTERFACE == CDS_OSI_WINDOWS
#       if CDS_COMPILER == CDS_COMPILER_MSVC
            cds::threading::msvc::Manager::fini() ;
#       endif
            cds::threading::wintls::Manager::fini() ;
#   else
#       if CDS_COMPILER == CDS_COMPILER_GCC
            cds::threading::gcc::Manager::fini() ;
#       endif
            cds::threading::pthread::Manager::fini() ;
#   endif
        }

    }

    void CDS_EXPORT_API Initialize( unsigned int /*nFeatureFlags*/ )   
    {
        if ( atomics::inc<membar_acquire>( &s_nInitCallCount ) == 0 ) {
            OS::topology::init()    ;
            threading::ThreadData::s_nProcCount = OS::topology::processor_count()    ;
            if ( threading::ThreadData::s_nProcCount == 0 )
                threading::ThreadData::s_nProcCount = 1 ;

            init_thread_manager()  ;
        }
    }

    void CDS_EXPORT_API Terminate()  
    {
        if ( atomics::dec<membar_acquire>( &s_nInitCallCount ) == 1 ) {
            fini_thread_manager()  ;

            OS::topology::fini()    ;
        }
    }

}   // namespace cds