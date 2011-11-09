/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_PTB_PASS_THE_BUCK_GC_H
#define __CDS_GC_PTB_PASS_THE_BUCK_GC_H

#include <cds/gc/ptb/ptb.h>
#include <cds/threading/model.h>

//@cond
namespace cds { namespace gc { namespace ptb {

        /// User-space Pass-the-Buck garbage collector
        /**
            This class is a wrapper for Pass-the-Buck garbage collector internal implementation.
            It simplifies usage of \p libcds PTB schema hiding implementation-specific details.

            \par Usage
            In your \p main function you declare a object of class cds::gc::ptb::GC. This declaration
            initializes internal GarbageCollector singleton.
            \code
            #include <cds/gc/ptb/gc.h>

            int main(int argc, char** argv) 
            {
                // Initialize libcds
                cds::Initialize() ;

                {                
                    // Initialize Hazard Pointer singleton
                    cds::gc::ptb::GC ptbGC() ;

                    // Some useful work
                    ...
                }

                // Terminate libcds
                cds::Terminate()    ;
            }
            \endcode

            Each thread that uses cds::gc::ptb_gc -based containers must be attached to Pas-the-Buck
            singleton. To make attachment you should declare a object of class GC::thread_gc:
            \code
            #include <cds/gc/ptb/gc.h>

            int myThreadEntryPoint()
            {
                // Attach the thread to PTB singleton                    
                cds::gc::ptb::GC::thread_gc myThreadGC()    ;

                // Do some work
                ...

                // The destructor of myThreadGC object detaches the thread from PTB singleton
            }
            \endcode

            In some cases, you should work in a external thread. For example, your application
            is a plug-in for a server that calls your code in the threads that has been created by server. 
            In this case, you may use persistent mode of GC::thread_gc. In this mode, the thread attaches
            to the PTB singleton only if it not yet attached and never call detaching:
            \code
            #include <cds/gc/ptb/gc.h>

            int myThreadEntryPoint()
            {
            // Attach the thread in persistent mode
            cds::gc::ptb::GC::thread_gc myThreadGC( true )    ;

            // Do some work
            ...

            // The destructor of myThreadGC object does NOT detach the thread from PTB singleton
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
                    The constructor attaches the current thread to the Pass-the-Buck GC 
                    if it is not yet attached.
                    The \p bPersistent parameter specifies attachment persistence: 
                    - \p true - the class destructor will not detach the thread from Pass-the-Buck GC.
                    - \p false (default) - the class destructor will detach the thread from Pass-the-Buck GC.
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
                    Otherwise it detaches the current thread from Pass-the-Buck GC.
                */
                ~thread_gc()
                {
                    if ( !m_bPersistent )
                        cds::threading::Manager::detachThread() ;
                }
            };

            /// Base for container node
            /**
                This struct is empty for Pass-the-Buck GC
            */
            struct container_node
            {};


            /// Pass-the-Buck guard
            /**
                This class is a wrapper for ptb::Guard.
            */
            class Guard: public ptb::Guard
            {
                //@cond
                typedef ptb::Guard base_class  ;
                //@endcond

            public:
                //@cond
                Guard()
                    : base_class( threading::getGC<ptb_gc>() )
                {}
                //@endcond

                /// Guards a pointer of type \p T
                /**
                    Return the value of \p pToGuard.

                    The function tries to load \p pToGuard and to store it 
                    to the guard repeatedly until the guard's value equals \p pToGuard
                */
                template <typename T>
                T * guard( T * volatile & pToGuard )
                {
                    T * pRet ;
                    do {
                        base_class::operator =( atomics::load<membar_relaxed>(pToGuard) );
                    } while ( (pRet = get<T>()) != atomics::load<membar_acquire>(pToGuard) ) ;

                    return pRet ;
                }

                /// Guards a pointer of type \p atomic<T*>
                /**
                    Return the value of \p toGuard

                    The function tries to load \p toGuard and to store it 
                    to the guard repeatedly until the guard's value equals \p toGuard
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
                    base_class::clear() ;
                }

                /// Get current value guarded
                template <typename T>
                T * get() const
                {
                    return reinterpret_cast<T *>( base_class::get_guard()->pPost )   ;
                }
            };

            /// Array of Pass-the-Buck guards
            /**
                This class is a wrapper for ptb::GuardArray template. 
                Template parameter \p COUNT defines the size of PTB array.
            */
            template <size_t COUNT>
            class GuardArray: public ptb::GuardArray<COUNT>
            {
                //@cond
                typedef ptb::GuardArray<COUNT> base_class   ;
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
                    : base_class( threading::getGC<ptb_gc>() )
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
                    return reinterpret_cast<T *>( base_class::operator[](nIndex).get_guard()->pPost )   ;
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
                size_t nLiberateThreshold = 1024
                , size_t nInitialThreadGuardCount = 8
            )
            {
                GarbageCollector::Construct(
                    nLiberateThreshold,
                    nInitialThreadGuardCount
                )   ;
            }

            /// Terminates GarbageCollector singleton
            /**
                The destructor calls \code GarbageCollector::Destruct() \endcode
            */
            ~GC()
            {
                GarbageCollector::Destruct()  ;
            }
        };

}}} // namespace cds::gc::ptb
//@endcond

#endif // #ifndef __CDS_GC_PTB_PASS_THE_BUCK_GC_H
