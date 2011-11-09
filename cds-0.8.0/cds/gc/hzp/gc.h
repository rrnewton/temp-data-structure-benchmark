/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_HZP_HAZARDPTR_GC_H
#define __CDS_GC_HZP_HAZARDPTR_GC_H

#include <cds/gc/hzp/hzp.h>
#include <cds/threading/model.h>

//@cond
namespace cds { namespace gc { namespace hzp {

        /// User-space Hazard Pointer garbage collector
        /**
            This class is a wrapper for Hazard Pointer garbage collector internal implementation.
            It simplifies usage of \p libcds Hazard Pointer schema hiding implementation-specific details.

            \par Usage
            In your \p main function you declare a object of class cds::gc::hzp::GC. This declaration
            initializes internal GarbageCollector singleton.
            \code
            #include <cds/gc/hzp/gc.h>

            int main(int argc, char** argv) 
            {
                // Initialize libcds
                cds::Initialize() ;

                {
                    // Initialize Hazard Pointer singleton
                    cds::gc::hzp::GC hpGC() ;

                    // Some useful work
                    ...
                }

                // Terminate libcds
                cds::Terminate()    ;
            }
            \endcode

            Each thread that uses cds::gc::hzp_gc -based containers must be attached to Hazard Pointer
            singleton. To make attachment you should declare a object of class GC::thread_gc:
            \code
            #include <cds/gc/hzp/gc.h>

            int myThreadEntryPoint()
            {
                // Attach the thread to Hazard Pointer singleton                    
                cds::gc::hzp::GC::thread_gc myThreadGC()    ;

                // Do some work
                ...

                // The destructor of myThreadGC object detaches the thread from HP singleton
            }
            \endcode

            In some cases, you should work in a external thread. For example, your application
            is a plug-in for a server that calls your code in the threads that has been created by server. 
            In this case, you may use persistent mode of GC::thread_gc. In this mode, the thread attaches
            to the HP singleton only if it not yet attached and never call detaching:
            \code
            #include <cds/gc/hzp/gc.h>

            int myThreadEntryPoint()
            {
            // Attach the thread in persistent mode
            cds::gc::hzp::GC::thread_gc myThreadGC( true )    ;

            // Do some work
            ...

            // The destructor of myThreadGC object does NOT detach the thread from HP singleton
            }
            \endcode

        */
        class GC 
        {
        public:

            /// Wrapper for ThreadGC class
            /**
                This class performs automatically attaching/detaching Hazard Pointer GC 
                for the current thread.
            */
            class thread_gc: public ThreadGC
            {
                //@cond
                bool    m_bPersistent   ;
                //@endcond
            public:
                /// Constructor
                /**
                    The constructor attaches the current thread to the Hazard Pointer GC 
                    if it is not yet attached.
                    The \p bPersistent parameter specifies attachment persistence: 
                    - \p true - the class destructor will not detach the thread from Hazard Pointer GC.
                    - \p false (default) - the class destructor will detach the thread from Hazard Pointer GC.
                */
                thread_gc(
                    bool    bPersistent = false 
                )
                : m_bPersistent( bPersistent )
                {
                    if ( !threading::Manager::isThreadAttached() )
                        threading::Manager::attachThread() ;
                }

                /// Destructor
                /**
                    If the object has been created in persistent mode, the destructor does nothing.
                    Otherwise it detaches the current thread from Hazard Pointer GC.
                */
                ~thread_gc()
                {
                    if ( !m_bPersistent )
                        cds::threading::Manager::detachThread() ;
                }
            };

            /// Base for container node
            /**
                This struct is empty for Hazard Pointer GC
            */
            struct container_node
            {};

            /// Hazard Pointer guard
            /**
                This class is a wrapper for AutoHPGuard.
            */
            class Guard: public AutoHPGuard
            {
                //@cond
                typedef AutoHPGuard base_class  ;
                //@endcond

            public:
                //@cond
                Guard()
                    : base_class( threading::getGC<hzp_gc>() )
                {}
                //@endcond

                /// Guards a pointer of type \p T
                /**
                    Return the value of \p pToGuard.

                    The function tries to load \p pToGuard and to store it 
                    to the HP slot repeatedly until the guard's value equals \p pToGuard
                */
                template <typename T>
                T * guard( T * volatile & pToGuard )
                {
                    T * pRet ;
                    do {
                        base_class::operator =( atomics::load<membar_relaxed>(pToGuard) ) ;
                    } while ( (pRet = get<T>()) != atomics::load<membar_acquire>(pToGuard) ) ;

                    return pRet ;
                }

                /// Guards a pointer of type \p atomic<T*>
                /**
                    Return the value of \p toGuard

                    The function tries to load \p toGuard and to store it 
                    to the HP slot repeatedly until the guard's value equals \p toGuard
                */
                template <typename T>
                T * guard( atomic<T *>& toGuard )
                {
                    T * pRet ;
                    do {
                        base_class::operator=( toGuard.template load<membar_relaxed>() )    ;
                    } while ( ( pRet = get<T>()) != toGuard.template load<membar_acquire>() ) ;
            
                    return pRet ;
                }

                /// Store \p to the guard
                /**
                    The function equals to a simple assignment, no loop is performed.
                    Can be used for a pointer that cannot be changed concurrently.
                */
                template <typename T>
                T * assign( T * p )
                {
                    return base_class::operator =(p) ;
                }

                /// Clear value of the guard
                void clear()
                {
                    assign( reinterpret_cast<void *>(NULL) )    ;
                }

                /// Get current value guarded
                template <typename T>
                T * get() const
                {
                    return reinterpret_cast<T *>( base_class::m_hzp )   ;
                }
            };

            /// Array of Hazard Pointer guards
            /**
                This class is a wrapper for AutoHPArray template. 
                Template parameter \p COUNT defines the size of HP array.
            */
            template <size_t COUNT>
            class GuardArray: public AutoHPArray<COUNT>
            {
                //@cond
                typedef AutoHPArray<COUNT> base_class   ;
                //@endcond
            public:
                /// Rebind array for other size \p COUNT2
                template <size_t COUNT2>
                struct rebind {
                    typedef GuardArray<COUNT2>  other   ;   ///< rebinding result
                };

            public:
                //@cond
                GuardArray()
                    : base_class( threading::getGC<hzp_gc>() )
                {}
                //@endcond

                /// Guards a pointer of type \p T
                /**
                    Return the value of \p pToGuard.

                    The function tries to load \p pToGuard and to store it 
                    to the slot \p nIndex repeatedly until the guard's value equals \p pToGuard
                */

                template <typename T>
                T * guard( size_t nIndex, T * volatile & pToGuard )
                {
                    T * pRet    ;
                    do {
                        base_class::set( nIndex, atomics::load<membar_relaxed>(pToGuard) ) ;
                    } while ((pRet = get<T>(nIndex)) != atomics::load<membar_acquire>(pToGuard))    ;

                    return pRet ;
                }

                /// Guards a pointer of type \p atomic<T*>
                /**
                    Return the value of \p toGuard

                    The function tries to load \p toGuard and to store it 
                    to the slot \p nIndex repeatedly until the guard's value equals \p toGuard
                */
                template <typename T>
                T * quard(size_t nIndex, atomic<T *>& toGuard )
                {
                    T * pRet    ;
                    do {
                        base_class::set( nIndex, toGuard.template load<membar_relaxed>() ) ;
                    } while ((pRet = get<T>(nIndex)) != toGuard.template load<membar_acquire>())    ;

                    return pRet ;
                }

                /// Store \p to the slot \p nIndex
                /**
                    The function equals to a simple assignment, no loop is performed.
                */
                template <typename T>
                T * assign( size_t nIndex, T * p )
                {
                    base_class::set(nIndex, p) ;
                    return base_class::operator[](nIndex) ;
                }

                /// Clear value of the slot \p nIndex 
                void clear( size_t nIndex)
                {
                    base_class::clear( nIndex );
                }

                /// Get current value of slot \p nIndex
                template <typename T>
                T * get( size_t nIndex) const
                {
                    return reinterpret_cast<T *>( base_class::operator[](nIndex) )   ;
                }

                /// Capacity of the guard array
                size_t capacity() const
                {
                    return COUNT ;
                }
            };

        public:
            /// Initializes GarbageCollector singleton
            /**
                The constructor calls GarbageCollector::Construct with passed parameters.
                See GarbageCollector::Construct for explanation of parameters meaning.
            */
            GC(
                size_t nHazardPtrCount = 0,     ///< Hazard pointer count per thread
                size_t nMaxThreadCount = 0,     ///< Max count of thread in your application
                size_t nMaxRetiredPtrCount = 0, ///< Capacity of the array of retired objects for the thread
                scan_type nScanType = inplace   ///< Scan type (see \ref scan_type enum)
            )
            {
                GarbageCollector::Construct(
                    nHazardPtrCount,
                    nMaxThreadCount,
                    nMaxRetiredPtrCount,
                    nScanType
                )   ;
            }

            /// Terminates GarbageCollector singleton
            /**
                The destructor calls \code GarbageCollector::Destruct( true ) \endcode
            */
            ~GC()
            {
                GarbageCollector::Destruct( true )  ;
            }
        };

}}} // namespace cds::gc::hzp
//@endcond

#endif // #ifndef __CDS_GC_HZP_HAZARDPTR_GC_H
