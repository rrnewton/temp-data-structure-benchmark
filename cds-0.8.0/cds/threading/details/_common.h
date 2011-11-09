/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_THREADING__COMMON_H
#define __CDS_THREADING__COMMON_H

#include <cds/gc/hzp/hzp.h>
#include <cds/gc/hrc/hrc.h>
#include <cds/gc/ptb/ptb.h>

namespace cds {
    /// Threading support
    /**
        The CDS library requires support from the threads.
        Each garbage collector manages a control structure on the per-thread basis.
        The library does not provide any thread model. To embed the library to your application you must provide
        appropriate implementation of \p cds::threading::Manager interface. This interface manages cds::threading::ThreadData
        structure that contains thread specific data of GCs.

        Any \p cds::threading::Manager implementation is a singleton and it must be accessible from any thread and from any point of
        your application. Note that you should not mix different implementation of the \p cds::threading::Manager in single application.

        Before compiling of your application you may define one of \p CDS_THREADING_xxx macro in cds/user_setup/threading.h file:
            \li \p CDS_THREADING_AUTODETECT - auto-detect appropriate threading model for your platform and compiler. This is
                predefined value of threading model in cds/user_setup/threading.h.
            \li \p CDS_THREADING_MSVC - use cds::threading::msvc::Manager implementation based on Microsoft Visual C++ \p __declspec(thread)
                construction. Intended for Windows and Microsoft Visual C++ only. This is default threading model for Windows and MS Visual C++. 
            \li \p CDS_THREADING_WIN_TLS - use cds::threading::wintls::Manager implementation based on Windows TLS API. 
                Intended for Windows and Microsoft Visual C++ only. This macro should be explicitly specified if you use \p libcds
                features in your dll that is dynamically loaded by LoadLibrary. See Note below.
            \li \p CDS_THREADING_GCC - use cds::threading::gcc::Manager implementation based on GCC \p __thread
                construction. Intended for GCC compiler only. Note, that GCC compiler supports \p __thread keyword properly
                not for all platforms and even not for all GCC version.
            \li \p CDS_THREADING_PTHREAD - use cds::threading::pthread::Manager implementation based on pthread thread-specific
                data functions \p pthread_getspecific / \p pthread_setspecific. Intended for GCC compiler. 
            \li \p CDS_THREADING_USER_DEFINED - use user-defined threading model. 

        The library's core (dynamic linked library) is free of usage of user-supplied \p cds::threading::Manager code.
        \p cds::threading::Manager is necessary for header-only part of CDS library (for \ref cds::threading::getGC function).

        Before using thread manager you should initialize \p CDS library:
        \code
        #include <cds/threading/model.h>    // threading manager
        #include <cds/gc/hzp/hzp/h>         // Hazard Pointer GC

        int main()
        {
            // // Initialize \p CDS library
            cds::Initilize() ;

            // // Initialize Garbage collector(s) that you use 
            cds::gc::hzp::GarbageCollector::Construct() ;

            // // Do some useful work 
            ...

            // // Terminate GCs 
            cds::gc::hzp::GarbageCollector::Destruct() ;

            // // Terminate \p CDS library
            cds::Terminate()    ;
        }
        \endcode

        <b>Note for Windows</b>

        When you use Garbage Collectors (GS) provided by \p libcds in your dll that dynamically loaded by \p LoadLibrary then there is no way 
        to use \p __declspec(thread) keyword to support threading model for \p libcds. MSDN says:

        \li If a DLL declares any nonlocal data or object as __declspec( thread ), it can cause a protection fault if dynamically loaded. 
            After the DLL is loaded with \p LoadLibrary, it causes system failure whenever the code references the nonlocal __declspec( thread ) data. 
            Because the global variable space for a thread is allocated at run time, the size of this space is based on a calculation of the requirements 
            of the application plus the requirements of all the DLLs that are statically linked. When you use \p LoadLibrary, there is no way to extend 
            this space to allow for the thread local variables declared with __declspec( thread ). Use the TLS APIs, such as TlsAlloc, in your 
            DLL to allocate TLS if the DLL might be loaded with LoadLibrary.

        Thus, in case when \p libcds or a dll that depends on \p libcds is loaded dynamically by calling \p LoadLibrary, 
        you may not use auto-defined \p CDS_THREADING_MSVC macro.  Instead, you should build your dll project with explicitly 
        defined \p CDS_THREADING_WIN_TLS.
    */
    namespace threading {

        /// Thread-specific data
        struct ThreadData {

            //@cond
            char CDS_DATA_ALIGNMENT(8) m_hpManagerPlaceholder[sizeof(gc::hzp::ThreadGC)]   ;   ///< Michael's Hazard Pointer GC placeholder
            char CDS_DATA_ALIGNMENT(8) m_hrcManagerPlaceholder[sizeof(gc::hrc::ThreadGC)]  ;   ///< Gidenstam's GC placeholder
            char CDS_DATA_ALIGNMENT(8) m_ptbManagerPlaceholder[sizeof(gc::ptb::ThreadGC)]  ;   ///< Pass The Buck GC placeholder
            //@endcond

            gc::hzp::ThreadGC * m_hpManager     ;   ///< Michael's Hazard Pointer GC thread-specific data
            gc::hrc::ThreadGC * m_hrcManager    ;   ///< Gidenstam's GC thread-specific data
            gc::ptb::ThreadGC * m_ptbManager    ;   ///< Pass The Buck GC thread-specific data

            size_t  m_nFakeProcessorNumber  ;   ///< fake "current processor" number

            //@cond
            static CDS_EXPORT_API volatile size_t  s_nLastUsedProcNo   ;
            static CDS_EXPORT_API size_t           s_nProcCount        ;
            //@endcond

            //@cond
            ThreadData()
                : m_nFakeProcessorNumber( cds::atomics::inc<membar_relaxed>( &s_nLastUsedProcNo ) % s_nProcCount )
            {
                if (gc::hzp::GarbageCollector::isUsed() )
                    m_hpManager = new (m_hpManagerPlaceholder) gc::hzp::ThreadGC ;
                else
                    m_hpManager = NULL  ;

                if ( gc::hrc::GarbageCollector::isUsed() )
                    m_hrcManager = new (m_hrcManagerPlaceholder) gc::hrc::ThreadGC  ;
                else
                    m_hrcManager = NULL ;

                if ( gc::ptb::GarbageCollector::isUsed() )
                    m_ptbManager = new (m_ptbManagerPlaceholder) gc::ptb::ThreadGC  ;
                else
                    m_ptbManager = NULL ;
            }

            ~ThreadData()
            {
                if ( m_hpManager ) {
                    m_hpManager->gc::hzp::ThreadGC::~ThreadGC()     ;
                    m_hpManager = NULL  ;
                }

                if ( m_hrcManager ) {
                    m_hrcManager->gc::hrc::ThreadGC::~ThreadGC()    ;
                    m_hrcManager = NULL ;
                }

                if ( m_ptbManager ) {
                    m_ptbManager->gc::ptb::ThreadGC::~ThreadGC()  ;
                    m_ptbManager = NULL     ;
                }
            }

            void init()
            {
                if ( gc::hzp::GarbageCollector::isUsed() )
                    m_hpManager->init()   ;
                if ( gc::hrc::GarbageCollector::isUsed() )
                    m_hrcManager->init()  ;
                if ( gc::ptb::GarbageCollector::isUsed() )
                    m_ptbManager->init()  ;
            }

            void fini()
            {
                if ( gc::hrc::GarbageCollector::isUsed() )
                    m_hrcManager->fini()  ;
                if ( gc::hzp::GarbageCollector::isUsed() )
                    m_hpManager->fini()   ;
                if ( gc::ptb::GarbageCollector::isUsed() )
                    m_ptbManager->fini()   ;
            }

            size_t fake_current_processor()
            {
                return m_nFakeProcessorNumber   ;
            }
            //@endcond
        };

        /*
        //@cond
        namespace details {
            typedef void (* manager_init_func)()    ;
        }
        //@endcond
        */

    } // namespace threading
} // namespace cds::threading

#endif // #ifndef __CDS_THREADING__COMMON_H
