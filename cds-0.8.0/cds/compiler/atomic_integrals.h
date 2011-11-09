/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_COMPILER_ATOMIC_INTEGRALS_H
#define __CDS_COMPILER_ATOMIC_INTEGRALS_H

#include <cds/details/defs.h>

namespace cds {

    /// Integral types for atomic operations
    namespace atomic_integrals {
        /// Atomic pointer
        typedef void *            pointer_t   ;

#   if CDS_BUILD_BITS == 32
        /// 64bit unaligned int
        typedef long long           atomic64_unaligned      ;

        /// 64bit unaligned unsigned int
        typedef unsigned long long  atomic64u_unaligned     ;

        /// 64bit aligned int
        typedef atomic64_unaligned CDS_TYPE_ALIGNMENT(8)    atomic64_aligned    ;

        /// 64bit aligned unsigned int
        typedef atomic64u_unaligned CDS_TYPE_ALIGNMENT(8)   atomic64u_aligned   ;

        /// 64bit atomic int (aligned)
        typedef atomic64_aligned    atomic64_t      ;

        /// 64bit atomic unsigned int (aligned)
        typedef atomic64u_aligned   atomic64u_t     ;

        /// 32bit atomic int
        typedef long                atomic32_t      ;

        /// 32bit atomic unsigned int
        typedef unsigned long       atomic32u_t     ;

        /// atomic int
        typedef atomic32_t          atomic_t        ;

        /// atomic unsigned int
        typedef atomic32u_t         unsigned_atomic_t ;

        /// atomic int sized as pointer
        /**
            This type is synonym for atomic32_t for 32bit platforms and for atomic64_t for 64bit platforms
        */
        typedef    atomic32_t       ptr_atomic_t    ;

        /// atomic unsigned int sized as pointer
        /**
            This type is synonym for atomic32u_t for 32bit platforms and for atomic64u_t for 64bit platforms
        */
        typedef atomic32u_t         uptr_atomic_t   ;

#   elif CDS_BUILD_BITS == 64
        //@cond none
        typedef long long               atomic64_unaligned  ;   // 64bit unaligned int
        typedef unsigned long long      atomic64u_unaligned ;   // 64bit unaligned unsigned int
        typedef atomic64_unaligned      atomic64_aligned    ;   // 64bit aligned int
        typedef atomic64u_unaligned     atomic64u_aligned   ;   // 64bit aligned unsigned int

        typedef atomic64_aligned        atomic64_t          ;    // 64bit atomic int
        typedef atomic64u_aligned       atomic64u_t         ;    // 64bit atomic unsigned int

        typedef int                     atomic32_t          ;    // 32bit atomic int
        typedef unsigned int            atomic32u_t         ;    // 32bit atomic unsigned int

        typedef atomic32_t              atomic_t            ;    // atomic int
        typedef atomic32u_t             unsigned_atomic_t   ;    // atomic unsigned int
        typedef    atomic64_t           ptr_atomic_t        ;    // atomic int sized as pointer
        typedef atomic64u_t             uptr_atomic_t       ;    // atomic unsigned int sized as pointer
        //@endcond
#   endif

        //@cond none
        template <int> struct atomic_long_selector    ;

        template <> struct atomic_long_selector<4>
        {
            typedef atomic32_t  atomic_long ;
        };

        template <> struct atomic_long_selector<8>
        {
            typedef atomic64_t  atomic_long ;
        };
        //@endcond

        /// Atomic type for long int
        /**
            The typedef selects appropriate atomic type depends on sizeof(long):
            \li if sizeof(long) == 4 (for 32bit platforms) then \p atomic_long is \ref atomic32_t
            \li if sizeof(long) == 8 (for 32bit platforms) then \p atomic_long is \ref atomic64_t
        */
        typedef atomic_long_selector<sizeof(long)>::atomic_long    atomic_long ;

        /************************************************************************/
        /* atomic_cast conversions                                              */
        /************************************************************************/
        //@cond none
        template <typename ATOMIC, typename T>
        static inline ATOMIC atomic_cast( T val )
        {
            CDS_STATIC_ASSERT( sizeof( T ) == sizeof( ATOMIC ) )        ;
            return *(ATOMIC *) &val   ;
        }

        template <typename P, typename T>
        static inline P * atomic_cast( T * val )
        {
            return (P *) val  ;
        }

        template <>
        inline atomic32_t atomic_cast<atomic32_t>( int val )
        {
            return (atomic32_t) val ;
        }

        template <>
        inline atomic32_t atomic_cast<atomic32_t>( unsigned int val )
        {
            return (atomic32_t) val ;
        }

        template <>
        inline atomic64_unaligned atomic_cast<atomic64_unaligned>( long long val )
        {
            return (atomic64_unaligned) val ;
        }

        template <>
        inline atomic64_unaligned atomic_cast<atomic64_unaligned>( unsigned long long val )
        {
            return (atomic64_unaligned) val ;
        }

        template <>
        inline atomic_long atomic_cast<atomic_long>( long val )
        {
            return (atomic_long) val ;
        }

        template <>
        inline atomic_long atomic_cast<atomic_long>( unsigned long val )
        {
            return (atomic_long) val ;
        }
        //@endcond

    }   // namespace atomic_integrals

    using namespace atomic_integrals ;
} // namespace cds

#endif // #ifndef __CDS_COMPILER_ATOMIC_INTEGRALS_H
