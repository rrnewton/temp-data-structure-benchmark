/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_THREADING_DETAILS_WINTLS_MANAGER_H
#define __CDS_THREADING_DETAILS_WINTLS_MANAGER_H

#include <stdio.h>
#include <cds/threading/details/_common.h>

namespace cds { namespace threading {

    /// cds::threading::Manager implementation based on Windows TLS API
    namespace wintls {

        /// Thread-specific data manager based on Windows TLS API
        /**
            Manager throws an exception of Manager::api_exception class if an error occurs
        */
        class Manager {
        private :
            /// Windows TLS API error code type
            typedef DWORD api_error_code  ;

            /// TLS API exception
            class api_exception: public cds::Exception {
            public:
                const api_error_code    m_errCode   ;   ///< error code
            public:
                /// Exception constructor
                api_exception( api_error_code nCode, const char * pszFunction )
                    : m_errCode( nCode )
                {
                    char buf[256]   ;
                    sprintf_s( buf, sizeof(buf)/sizeof(buf[0]), "Win32 TLS API error %u [function %s]", nCode, pszFunction ) ;
                    m_strMsg = buf  ;
                }
            };

            //@cond
            enum EThreadAction {
                do_getData,
                do_attachThread,
                do_detachThread,
                do_checkData
            };
            //@endcond

            //@cond
            /// TLS key holder
            struct Holder {
                static CDS_EXPORT_API DWORD m_key    ;

                static void init()
                {
                    if ( m_key == TLS_OUT_OF_INDEXES ) {
                        if ( (m_key = ::TlsAlloc()) == TLS_OUT_OF_INDEXES )
                            throw api_exception( ::GetLastError(), "TlsAlloc" )   ;
                    }
                }

                static void fini()
                {
                    if ( m_key != TLS_OUT_OF_INDEXES ) {
                        if ( ::TlsFree( m_key ) == 0 )
                            throw api_exception( ::GetLastError(), "TlsFree" )   ;
                        m_key = TLS_OUT_OF_INDEXES  ;
                    }
                }

                static ThreadData *    get()
                {
                    api_error_code  nErr    ;
                    void * pData = ::TlsGetValue( m_key )   ;
                    if ( pData == NULL && (nErr = ::GetLastError()) != ERROR_SUCCESS )
                        throw api_exception( nErr, "TlsGetValue" )      ;
                    return reinterpret_cast<ThreadData *>( pData )      ;
                }

                static void alloc()
                {
                    ThreadData * pData = new ThreadData ;
                    if ( !::TlsSetValue( m_key, pData )) 
                        throw api_exception( ::GetLastError(), "TlsSetValue" )   ;
                }
                static void free()
                {
                    ThreadData * p  ;
                    if ( (p = get()) != NULL ) {
                        delete p    ;
                        ::TlsSetValue( m_key, NULL ) ;
                    }
                }
            };
            //@endcond

            //@cond
            static ThreadData * _threadData( EThreadAction nAction )
            {
                switch ( nAction ) {
                    case do_getData:
#           ifdef _DEBUG
                        {
                            ThreadData * p = Holder::get() ;
                            assert( p != NULL ) ;
                            return p            ;
                        }
#           else
                        return Holder::get() ;
#           endif
                    case do_checkData:
                        return Holder::get() ;
                    case do_attachThread:
                        if ( Holder::get() == NULL )
                            Holder::alloc()  ;
                        return Holder::get() ;
                    case do_detachThread:
                        Holder::free()   ;
                        return NULL     ;
                    default:
                        assert( false ) ;   // anything forgotten?..
                }
                return NULL     ;
            }
            //@endcond

        public:
            /// Initialize manager 
            /**
                If you explicitly specify the \p CDS_THREADING_WIN_TLS macro at compile time then this
                function is mandatory and should be called before using any feature provided by \p libcds.
            */
            static void init()
            {
                Holder::init()  ;
            }

            /// Terminate manager 
            /**
                If you explicitly specify the \p CDS_THREADING_WIN_TLS macro at compile time then this
                function is mandatory and should be called on termination step of your application after 
                using any feature provided by \p libcds.
            */
            static void fini()
            {
                Holder::fini()  ;
            }

            /// Checks whether current thread is attached to \p libcds feature or not.
            static bool isThreadAttached()
            {
                return _threadData( do_checkData ) != NULL  ;
            }

            /// This method must be called in beginning of thread execution
            /**
                If TLS pointer to manager's data is NULL, api_exception is thrown 
                with code = -1.
                If an error occurs in call of Win TLS API function, api_exception is thrown 
                with Windows error code.
            */
            static void attachThread()
            {
                ThreadData * pData = _threadData( do_attachThread )    ;
                assert( pData != NULL ) ;

                if ( pData ) {
                    pData->init()   ;
                }
                else
                    throw api_exception( api_error_code(-1), "cds::threading::wintls::Manager::attachThread" )   ;
            }

            /// This method must be called in end of thread execution
            /**
                If TLS pointer to manager's data is NULL, api_exception is thrown 
                with code = -1.
                If an error occurs in call of Win TLS API function, api_exception is thrown 
                with Windows error code.
            */
            static void detachThread()
            {
                ThreadData * pData = _threadData( do_getData )    ;
                assert( pData != NULL ) ;

                if ( pData ) {
                    pData->fini()   ;

                    _threadData( do_detachThread )    ;
                }
                else
                    throw api_exception( api_error_code(-1), "cds::threading::winapi::Manager::detachThread" )   ;
            }

            /// Returns gc::hzp::ThreadGC object of current thread
            /**
                The object returned may be uninitialized if you did not call Manager::attachThread in the beginning of thread execution
                or do not use gc::hzp::GarbageCollector.
                To initialize global gc::hzp::GarbageCollector object you must call cds::gc::hzp::GarbageCollector::Construct()
                in the beginning of your application
            */
            static gc::hzp_gc::thread_gc&   getHZPGC()
            {
                return *(_threadData( do_getData )->m_hpManager)    ;
            }

            /// Returns gc::hrc::ThreadGC object of current thread
            /**
                The object returned may be uninitialized if you did not call Manager::attachThread in the beginning of thread execution
                or do not use gc::hrc::GarbageCollector.
                To initialize global gc::hrc::GarbageCollector object you must call cds::gc::hrc::GarbageCollector::Construct()
                in the beginning of your application
            */
            static gc::hrc_gc::thread_gc&   getHRCGC()
            {
                return *(_threadData( do_getData )->m_hrcManager)   ;
            }

            /// Returns gc::ptb::ThreadGC object of current thread
            /**
                The object returned may be uninitialized if you did not call Manager::attachThread in the beginning of thread execution
                or do not use gc::ptb::GarbageCollector.
                To initialize global gc::ptb::GarbageCollector object you must call cds::gc::ptb::GarbageCollector::Construct()
                in the beginning of your application
            */
            static gc::ptb_gc::thread_gc&   getPTBGC()
            {
                return *(_threadData( do_getData )->m_ptbManager)   ;
            }

            //@cond
            static size_t fake_current_processor()
            {
                return _threadData( do_getData )->fake_current_processor()  ;
            }
            //@endcond
        } ;

    } // namespace wintls
} } // namespace cds::threading

#endif // #ifndef __CDS_THREADING_DETAILS_WINTLS_MANAGER_H