/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_LOCK_SPINLOCK_H
#define __CDS_LOCK_SPINLOCK_H

/*
    Defines spin-lock primitives
    Editions:
        2010.01.22  0.6.0 khizmax     Refactoring: use cds::atomic namespace
                                      Explicit memory ordering specification (atomic::memory_order_xxx)
        2006              khizmax     Created
*/

#include <cds/atomic.h>
#include <cds/os/thread.h>
#include <cds/backoff_strategy.h>

#include <boost/noncopyable.hpp>

namespace cds {
    /// Synchronization primitives
    namespace lock {

        //@cond none
        namespace details {

            template <typename T>
            struct primitives {
                static bool tryLock( T volatile * pDest )
                {
                    return atomics::cas<membar_acquire>( pDest, (T) 0, (T) 1 ) ;
                }

                static void release( T volatile * pDest )
                {
                    atomics::store<membar_release>( pDest, (T) 0 )   ;
                }
            };

#       ifdef CDS_spinlock_defined
            typedef atomics::platform::spinlock_t    spinlock_t  ;

            template <>
            struct primitives<spinlock_t> {
                static bool tryLock( spinlock_t volatile * pDest )
                {
                    return atomics::platform::spinlock_try_acquire( pDest )  ;
                }

                static void release( spinlock_t volatile * pDest )
                {
                    atomics::platform::spinlock_release( pDest )   ;
                }
            };
#       endif
        }   // namespace details
        //@endcond

        ///Spin lock.
        /**    Simple and light-weight spin-lock critical section
            It is useful to gain access to small (short-timed) code

            Algorithm:

                TATAS (test-and-test-and-lock)
                [1984] L. Rudolph, Z. Segall. Dynamic Decentralized Cache Schemes for MIMD Parallel Processors.

            No serialization performed - any of waiting threads may owns the spin-lock.
            This spin-lock is NOT recursive: the thread owned the lock cannot call lock() method withod deadlock.
            The method unlock() can call any thread

            DEBUG version: The spinlock stores owner thead id. Assertion is raised when:
                - double lock attempt encountered by same thread (deadlock)
                - unlock by another thread

            If spin-lock is locked the BACKOFF algorithm is called. Predefined backoff::LockDefault class yields current
            thread and repeats lock attempts later

            Template parameters:
                - @p ATOMIC    one of integral atomic type: atomic_t, atomic32_t, atomic32u_t and others
                - @p BACKOFF    backoff strategy. Used when spin lock is locked
        */
        template <typename ATOMIC, class BACKOFF >
        class SpinT
        {
        public:
            typedef        ATOMIC   atomic_type         ;        ///< atomic type
            typedef        BACKOFF  backoff_strategy    ;        ///< backoff strategy type
        private:
            volatile atomic_type    m_spin  ;       ///< Spin
#    ifdef CDS_DEBUG
            typename OS::ThreadId    m_dbgOwnerId        ;       ///< Owner thread id (only for debug mode)
#    endif

        public:
            /// Construct free (unlocked) spin-lock
            CDS_CONSTEXPR SpinT()
                : m_spin(0)
#    ifdef CDS_DEBUG
                , m_dbgOwnerId( OS::nullThreadId() )
#    endif
            {}

            /// Construct spin-lock in specified state
            /**
                In debug mode: if \p bLocked = true then spin-lock is made owned by current thread
            */
            SpinT( bool bLocked )
                : m_spin( bLocked ? 1 : 0)
#    ifdef CDS_DEBUG
                , m_dbgOwnerId( bLocked ? OS::getCurrentThreadId() : OS::nullThreadId() )
#    endif
            {}

            /// Dummy copy constructor
            /**
                In theory, spin-lock cannot be copied. However, it is not practical.
                Therefore, we provide dummy copy constructor that do no copy in fact. The ctor
                initializes the spin to free (unlocked) state like default ctor.
            */
            SpinT(const SpinT<ATOMIC, BACKOFF>& )
                : m_spin(0)
#    ifdef CDS_DEBUG
                , m_dbgOwnerId( OS::nullThreadId() )
#    endif
            {}


            /// Destructor. On debug time it checks whether spin-lock is free
            ~SpinT()
            {
                assert( m_spin == 0 )   ;
            }

            /// Try to lock the object. Return @p true if success. Debug version: deadlock may be detected
            bool tryLock() throw()
            {
                // Deadlock detected
                assert( m_dbgOwnerId != OS::getCurrentThreadId() ) ;

                bool bOk = details::primitives<atomic_type>::tryLock( &m_spin )    ;

                CDS_DEBUG_DO(
                    if ( bOk ) {
                        m_dbgOwnerId = OS::getCurrentThreadId()    ;
                    }
                )
                return bOk    ;
            }

            /// Try to lock the object, repeat @p nTryCount times if failed
            bool tryLock( unsigned int nTryCount )
            {
                BACKOFF backoff    ;
                while ( nTryCount-- ) {
                    if ( tryLock() )
                        return true    ;
                    backoff()    ;
                }
                return false ;
            }

            /// Lock the spin-lock. Waits infinitely while spin-lock is locked. Debug version: deadlock may be detected
            void lock()
            {
                BACKOFF backoff    ;

                // Deadlock detected
                assert( m_dbgOwnerId != OS::getCurrentThreadId() ) ;

                // TATAS algorithm
                while ( !tryLock() ) {
                    while ( m_spin ) {
                        backoff()    ;
                    }
                }
                assert( m_dbgOwnerId == OS::getCurrentThreadId() ) ;
            }

            /// Unlock the spin-lock. Debug version: deadlock may be detected
            void unlock()
            {
                assert( m_spin > 0 ) ;

                assert( m_dbgOwnerId == OS::getCurrentThreadId() )      ;
                CDS_DEBUG_DO( m_dbgOwnerId = OS::nullThreadId() ;)        

                details::primitives<atomic_type>::release( &m_spin )    ;
            }
        };

#    ifdef CDS_spinlock_defined
        typedef details::spinlock_t atomic_spin_t            ;   ///< atomic spin type (platform-dependent)
#    else
        typedef atomic32_t          atomic_spin_t            ;
#    endif

        /// Spin-lock based on smallest atomic type defined for platform
        template <typename BACKOFF>
        class Spinlock: public SpinT<atomic_spin_t, BACKOFF>
        {
        //@cond
            typedef SpinT<atomic_spin_t, BACKOFF> base_class    ;
        public:
            CDS_CONSTEXPR Spinlock()
                : base_class()
            {}

            Spinlock( bool bLocked )
                : base_class( bLocked )
            {}

            template <typename A2, typename B2>
            Spinlock(const SpinT<A2, B2>& )
                : base_class()
            {}
        //@endcond
        };

        /// Spin-lock implementation default for the current platform 
        typedef Spinlock<backoff::LockDefault >                 Spin    ;

        /// 32bit spin-lock
        typedef SpinT<atomic32_t, backoff::LockDefault >        Spin32  ;

        /// 64bit spin-lock
        typedef SpinT<atomic64_t, backoff::LockDefault >        Spin64  ;

        /// Recursive spin lock.
        /**
            Allows recursive calls: the owner thread may recursive enter to critical section guarded by the spin-lock.

            Template parameters:
                - @p ATOMIC    one of integral atomic type: atomic_t, atomic32_t, atomic32u_t and others
                - @p BACKOFF    backoff strategy. Used when spin lock is locked
        */
        template <typename ATOMIC, class BACKOFF>
        class ReentrantSpinT
        {
            typedef OS::ThreadId    thread_id    ;        ///< The type of thread id

        public:
            typedef ATOMIC          atomic_type         ; ///< The atomic type
            typedef BACKOFF         backoff_strategy    ; ///< The backoff type

        private:
            atomic<atomic_type>     m_spin      ; ///< spin-lock atomic
            atomic<thread_id>       m_OwnerId   ; ///< Owner thread id. If spin-lock is not locked it usually equals to OS::nullThreadId()

        private:
            //@cond
            void beOwner( thread_id tid )           { m_OwnerId.store<membar_relaxed>( tid ) ; }
            void free()                             { m_OwnerId.store<membar_relaxed>( OS::nullThreadId() ); }
            bool isOwned( thread_id tid ) const     { return m_OwnerId.load<membar_relaxed>() == tid ; }

            bool    tryLockOwned( thread_id tid )
            {
                if ( isOwned( tid )) {
                    m_spin.template inc<membar_relaxed>()  ;
                    return true    ;
                }
                return false    ;
            }

            bool tryAcquireLock() throw()
            {
                return m_spin.template cas<membar_acquire>( 0, 1 )   ;
            }

            bool tryAcquireLock( unsigned int nTryCount )
            {
                BACKOFF bkoff   ;

                while ( nTryCount-- ) {
                    if ( tryAcquireLock() )
                        return true    ;
                    bkoff()    ;
                }
                return false ;
            }

            void acquireLock()
            {
                // TATAS algorithm
                BACKOFF bkoff   ;
                while ( !tryAcquireLock() ) {
                    while ( m_spin.template load<membar_relaxed>() )
                        bkoff()    ;
                }
            }
            //@endcond

        public:
            /// Default constructor initializes spin to free (unlocked) state
            CDS_CONSTEXPR ReentrantSpinT()
                : m_OwnerId( OS::nullThreadId() )
                , m_spin(0)
            {}

            /// Dummy copy constructor
            /**
                In theory, spin-lock cannot be copied. However, it is not practical.
                Therefore, we provide dummy copy constructor that do no copy in fact. The ctor
                initializes the spin to free (unlocked) state like default ctor.
            */
            ReentrantSpinT(const ReentrantSpinT<ATOMIC, BACKOFF>& )
                : m_OwnerId( OS::nullThreadId() )
                , m_spin(0)
            {}

            /// Construct object for specified state
            ReentrantSpinT(bool bLocked)
                : m_spin(0),
                m_OwnerId( OS::nullThreadId() )
            {
                if ( bLocked )
                    lock() ;
            }

            /// Try to lock the spin-lock. If spin-lock is free the current thread owns it. Return @p true if lock is success
            bool tryLock()
            {
                thread_id tid = OS::getCurrentThreadId()    ;
                if ( tryLockOwned( tid ) )
                    return true    ;
                if ( tryAcquireLock()) {
                    beOwner( tid ) ;
                    return true    ;
                }
                return false    ;
            }

            /// Try to lock the object.
            /** If spin-lock is locked the method repeats attempts to own spin-lock up to @p nTryCount times.
                Between attempts @p backoff() is called.
                Return @p true if current thread owns the lock @p false otherwise
            */
            bool tryLock( unsigned int nTryCount )
            {
                thread_id tid = OS::getCurrentThreadId()    ;
                if ( tryLockOwned( tid ) )
                    return true    ;
                if ( tryAcquireLock( nTryCount )) {
                    beOwner( tid ) ;
                    return true    ;
                }
                return false    ;
            }

            /// Lock the object waits if it is busy
            void lock()
            {
                thread_id tid = OS::getCurrentThreadId()    ;
                if ( !tryLockOwned( tid ) ) {
                    acquireLock()        ;
                    beOwner( tid )        ;
                }
            }

            /// Unlock the spin-lock. Return @p true if the current thread is owner of spin-lock @p false otherwise
            bool unlock()
            {
                if ( isOwned( OS::getCurrentThreadId() ) ) {
                    if ( m_spin.template load<membar_relaxed>() > 1 )
                        m_spin.template dec<membar_relaxed>() ;
                    else {
                        free() ;
                        m_spin.template store<membar_release>( 0 ) ;
                    }
                    return true    ;
                }
                return false    ;
            }

            /// Change the owner of locked spin-lock. May be called by thread that is owner of the spin-lock
            bool changeOwner( OS::ThreadId newOwnerId )
            {
                if ( isOwned( OS::getCurrentThreadId() ) ) {
                    assert( newOwnerId != OS::nullThreadId() ) ;
                    m_OwnerId = newOwnerId ;
                    return true    ;
                }
                return false    ;
            }
        } ;

        /// Recursive spin-lock based on atomic32u_t
        typedef ReentrantSpinT<atomic32u_t, backoff::LockDefault>    ReentrantSpin32    ;


        /// Recursive spin-lock based on atomic64u_t type
        typedef ReentrantSpinT<atomic64u_t, backoff::LockDefault>    ReentrantSpin64    ;

        /// Recursive spin-lock based on atomic32_t type
        typedef ReentrantSpin32                                        ReentrantSpin    ;

        /// Auto locker.
        /**
            @p T is lock type with two methods: lock and unlock.
            The constructor locks the wrapped lock object, the destructor unlocks it.

            Auto locker is not copy-constructible.
        */
        template <class T>
        class Auto: boost::noncopyable
        {
            T&      m_lock  ;            ///< The lock object
        public:
            /// Lock the @p src lock object
            Auto( T& src )
                : m_lock( src )
            {
                m_lock.lock() ;
            }

            /// Unlock m_lock object
            ~Auto()        { m_lock.unlock() ; }

            /// Return pointer to lock object m_lock
            T *  operator ->()    { return &m_lock   ; }
        };

        /// The best (for the current platform) auto spin-lock
        typedef Auto<Spin>        AutoSpin    ;

        /// Auto unlocker
        /**
            @p T is lock type with one method: unlock.
            The destructor calls unlock method. Unlike Auto class, the constructor of the class does not call lock function of its argument.

            Auto unlocker is not copy-constructible.

            The class is intended for exception-safe usage of lock primitive, for example:
            \code
            cds::lock::Spin sp  ;
            ...
            if ( sp.tryLock() ) {
                // sp is locked
                cds::lock::AutoUnlock<cds::lock::Spin> aus(sp) ;
                // Now even if a exception has been thrown, the destructor of \p aus object unlocks the \p sp spin-lock correctly
                ...
            }
            \endcode
        */
        template <class T>
        class AutoUnlock: boost::noncopyable
        {
            T&      m_lock  ;            ///< The lock object
        public:
            /// Constructor The \p src object must be locked
            /**
                The constructor does not call \p src.lock().
            */
            AutoUnlock( T& src )
                : m_lock( src )
            {}

            /// Unlock m_lock object
            ~AutoUnlock()        { m_lock.unlock() ; }

            /// Return pointer to lock object m_lock
            T *  operator ->()    { return &m_lock   ; }
        };

    }    // namespace lock

    /// Standard (best for the current platform) spin-lock implementation
    typedef lock::Spin              SpinLock    ;

    /// 32bit spin lock shortcut
    typedef lock::Spin32            SpinLock32  ;

    /// 64bit spin-lock shortcut
    typedef lock::Spin64            SpinLock64  ;

    /// Standard (best for the current platform) recursive spin-lock implementation
    typedef lock::ReentrantSpin     RecursiveSpinLock   ;

    /// 32bit recursive spin-lock shortcut
    typedef lock::ReentrantSpin32   RecursiveSpinLock32 ;

    /// 64bit recursive spin-lock shortcut
    typedef lock::ReentrantSpin64   RecursiveSpinLock64 ;

    /// Auto spin-lock shortcut
    typedef lock::AutoSpin          AutoSpinLock        ;

}    // namespace cds

#endif  // #ifndef __CDS_LOCK_SPINLOCK_H
