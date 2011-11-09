/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_HRC_SCHEMA_GC_H
#define __CDS_GC_HRC_SCHEMA_GC_H

#include <cds/gc/hrc/hrc.h>
#include <cds/threading/model.h>

//@cond
namespace cds { namespace gc { namespace hrc {

        /// User-space Gidenstam's garbage collector
        /**
            This class is a wrapper for Gidenstam's memory reclamation schema (HRC) internal implementation.
            It simplifies usage of \p libcds HRC hiding implementation-specific details.

            \par Usage
            In your \p main function you declare a object of class cds::gc::hrc::GC. This declaration
            initializes internal GarbageCollector singleton.
            \code
            #include <cds/gc/hrc/gc.h>

            int main(int argc, char** argv) 
            {
                // Initialize libcds
                cds::Initialize() ;

                {
                    // Initialize HRC singleton
                    cds::gc::hrc::GC hpGC() ;

                    // Some useful work
                    ...
                }

                // Terminate libcds
                cds::Terminate()    ;
            }
            \endcode

            Each thread that uses cds::gc::hrc_gc -based containers must be attached to HRC
            singleton. To make attachment you should declare a object of class GC::thread_gc:
            \code
            #include <cds/gc/hrc/gc.h>

            int myThreadEntryPoint()
            {
                // Attach the thread to HRC singleton                    
                cds::gc::hrc::GC::thread_gc myThreadGC()    ;

                // Do some work
                ...

                // The destructor of myThreadGC object detaches the thread from HRC singleton
            }
            \endcode

            In some cases, you should work in a external thread. For example, your application
            is a plug-in for a server that calls your code in the threads that has been created by server. 
            In this case, you may use persistent mode of GC::thread_gc. In this mode, the thread attaches
            to the HRC singleton only if it not yet attached and never call detaching:
            \code
            #include <cds/gc/hrc/gc.h>

            int myThreadEntryPoint()
            {
            // Attach the thread in persistent mode
            cds::gc::hrc::GC::thread_gc myThreadGC( true )    ;

            // Do some work
            ...

            // The destructor of myThreadGC object does NOT detach the thread from HRC singleton
            }
            \endcode

        */
        class GC 
        {
        public:

            /// Wrapper for ThreadGC class
            /**
                This class performs automatically attaching/detaching Gidenstam's GC 
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
                    The constructor attaches the current thread to the HRC GC 
                    if it is not yet attached.
                    The \p bPersistent parameter specifies attachment persistence: 
                    - \p true - the class destructor will not detach the thread from HRC GC.
                    - \p false (default) - the class destructor will detach the thread from HRC GC.
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
                    Otherwise it detaches the current thread from HRC GC.
                */
                ~thread_gc()
                {
                    if ( !m_bPersistent )
                        cds::threading::Manager::detachThread() ;
                }
            };

            /// Base for container node
            struct container_node 
            {
                unsigned_ref_counter    m_RC        ;    ///< reference counter
                atomic<bool>            m_bTrace    ;    ///< @a true - node is tracing by HRC
                atomic<bool>            m_bDeleted  ;    ///< @ true - node is deleted
            };

            /// HRC guard
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
                    : base_class( threading::getGC<hrc_gc>() )
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
                    return ThreadGC::derefLink( &pToGuard, *this )  ;
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
                    return ThreadGC::derefLink( toGuard, *this )  ;
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
                    return static_cast<T *>( *base_class::m_hzp )   ;
                }
            };

            /// Array of guards
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
                    : base_class( threading::getGC<hrc_gc>() )
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
                    return ThreadGC::derefLink( &pToGuard, base_class::operator[](nIndex) ) ;
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
                    return ThreadGC::derefLink( toGuard, base_class::operator[](nIndex) ) ;
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
                    return static_cast<T *>( base_class::operator[](nIndex) )   ;
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
                size_t nHazardPtrCount = 0,     ///< number of hazard pointers
                size_t nMaxThreadCount = 0,     ///< max threads count
                size_t nMaxNodeLinkCount = 0,   ///< max number of links a @ref ContainerNode can contain
                size_t nMaxTransientLinks = 0   ///< max number of links in live nodes that may transiently point to a deleted node
            )
            {
                GarbageCollector::Construct(
                    nHazardPtrCount,
                    nMaxThreadCount,
                    nMaxNodeLinkCount,
                    nMaxTransientLinks
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

}}} // namespace cds::gc::hrc
//@endcond

#endif // #ifndef __CDS_GC_HRC_SCHEMA_GC_H
