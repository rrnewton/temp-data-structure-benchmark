/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_COMPILER_GCC_X86_ATOMIC_H
#define __CDS_COMPILER_GCC_X86_ATOMIC_H

/*
    Sources:
        load/store atomics: A.Terekhov http://www.decadent.org.uk/pipermail/cpp-threads/2008-December/001933.html
*/

#ifndef __SSE2__
#   error SSE2 is not detected. CDS library must be compiled with SSE2 instruction set
#endif

// Platform supports dword CAS primitive
#define    CDS_DWORD_CAS_SUPPORTED    1

// For atomic operations the template-based alternatives are defined
#define CDS_ATOMIC_TEMPLATE_DEFINED 1

//@cond none
namespace cds { namespace atomics {
    namespace gcc { namespace x86 {

#include <cds/compiler/gcc/x86/atomic_fence.h>

        //
        // 32bit atomic primitives
        //
#include <cds/compiler/gcc/x86/atomic_cas32.h>
#include <cds/compiler/gcc/x86/atomic_xchg32.h>
#include <cds/compiler/gcc/x86/atomic_xadd32.h>
#include <cds/compiler/gcc/x86/atomic_load32.h>
#include <cds/compiler/gcc/x86/atomic_store32.h>

        //
        // Atomic pointer primitives
        //
        template <typename SUCCESS_ORDER>
        static inline bool casptr( pointer_t volatile * pMem, pointer_t expected, pointer_t desired )
        {
            return cas32<SUCCESS_ORDER>( (atomic32_t volatile *) pMem, (atomic32_t) expected, (atomic32_t) desired );
        }
        static inline bool casptr( pointer_t volatile * pMem, pointer_t expected, pointer_t desired, memory_order success_order, memory_order failure_order )
        {
            return casptr<membar_relaxed>( pMem, expected, desired );
        }

        template <typename SUCCESS_ORDER>
        static inline pointer_t vcasptr(  pointer_t volatile * pMem, pointer_t expected, pointer_t desired )
        {
            return (pointer_t) vcas32<SUCCESS_ORDER>( (atomic32_t volatile *) pMem, (atomic32_t) expected, (atomic32_t) desired );
        }
        static inline pointer_t vcasptr(  pointer_t volatile * pMem, pointer_t expected, pointer_t desired, memory_order success_order, memory_order failure_order )
        {
            return vcasptr<membar_relaxed>( pMem, expected, desired );
        }

#       define CDS_xchgptr_defined
        template <typename ORDER>
        static inline pointer_t xchgptr( pointer_t volatile * pMem, pointer_t val )
        {
            return (pointer_t) xchg32<ORDER>( (atomic32_t volatile *) pMem, *((atomic32_t *) &val) )   ;
        }
        static inline pointer_t xchgptr( pointer_t volatile * pMem, pointer_t val, memory_order order )
        {
            return xchgptr<membar_relaxed>( pMem, val )   ;
        }

#include <cds/compiler/gcc/x86/atomic_loadptr.h>
#include <cds/compiler/gcc/x86/atomic_storeptr.h>

        //
        // 64bit atomic primitives
        //
        template <typename SUCCESS_ORDER>
        static inline bool cas64( atomic64_t volatile * pMem, atomic64_unaligned expected, atomic64_unaligned desired )
        {
#       if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8) || CDS_COMPILER_VERSION > 40200
            // GCC 4.2 and above has a built-in function implementing CAS64 primitive on x86 32bit.
            return __sync_bool_compare_and_swap( pMem, expected, desired ) ;
#       else
            // This code is not compiled in debug mode
            bool bRet ;
            asm volatile (
                "pushl    %%ebx   ;\n\t"
                "pushl    %%eax   ;\n\t"
                "pushl    %%edx   ;\n\t"
                :
            : "A"(desired)
                );
            asm volatile (
                "popl   %%ecx               ;\n\t"
                "popl   %%ebx               ;\n\t"
                "lock;    cmpxchg8b (%[dest])    ;\n\t"
                "setz    %[ret]                ;\n\t"
                "popl   %%ebx               ;\n\t"
                : "+m"(*pMem), [ret] "=a"(bRet)
                : [dest] "D"(pMem), "A"(expected)
                : "ecx", "memory", "cc" );
            return bRet;
#       endif
        }
        static inline bool cas64( atomic64_t volatile * pMem, atomic64_unaligned expected, atomic64_unaligned desired, memory_order success_order, memory_order failure_order )
        {
            return cas64<membar_relaxed>( pMem, expected, desired )   ;
        }

        template <typename SUCCESS_ORDER>
        static inline atomic64_t vcas64(  atomic64_t volatile * pMem, atomic64_unaligned expected, atomic64_unaligned desired )
        {
#       if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8) || CDS_COMPILER_VERSION > 40200
            return __sync_val_compare_and_swap( pMem, expected, desired ) ;
#       else
            atomic64_t    nRet    ;
            asm volatile (
                "pushl    %%ebx   ;\n\t"
                "pushl    %%eax   ;\n\t"
                "pushl    %%edx   ;\n\t"
                :
                : "A"(desired)
            );
            asm volatile (
                "popl   %%ecx               ;\n\t"
                "popl   %%ebx               ;\n\t"
                "lock;    cmpxchg8b (%[dest])    ;\n\t"
                "popl   %%ebx               ;\n\t"
                : "+m"(*pMem), [ret] "=A"(nRet)
                : [dest] "D"(pMem), "A"(expected)
                : "ecx", "memory", "cc"
            );
            return nRet;
#       endif
        }
        static inline atomic64_t vcas64( atomic64_t volatile * pMem, atomic64_unaligned expected, atomic64_unaligned desired, memory_order success_order, memory_order failure_order )
        {
            return vcas64<membar_relaxed>( pMem, expected, desired )   ;
        }

        template <typename ORDER>
        static inline atomic64_t load64( atomic64_t volatile const * pMem ) ;
        template <>
        inline atomic64_t load64<membar_relaxed>( atomic64_t volatile const * pMem )
        {
            // Atomically loads 64bit value by SSE instruction
            atomic64_t CDS_DATA_ALIGNMENT(16) v[2]  ;
            asm (
                "movq   %[pMem], %%xmm0  ;   \n\t"
                "movdqa %%xmm0, %[v]     ;   \n\t"
                : [v] "=m" (v)
                : [pMem] "m" (*pMem)
                : "xmm0", "memory"
                ) ;
            return v[0] ;
        }
        template <>
        inline atomic64_t load64<membar_consume>( atomic64_t volatile const * pMem )
        {
            return load64<membar_relaxed>( pMem )   ;
        }
        template <>
        inline atomic64_t load64<membar_acquire>( atomic64_t volatile const * pMem )
        {
            // Atomically loads 64bit value by SSE instruction
            atomic64_t CDS_DATA_ALIGNMENT(16) v[2]  ;
            asm volatile (
                "movq   %[pMem], %%xmm0  ;   \n\t"
                "movdqa %%xmm0, %[v]     ;   \n\t"
                : [v] "=m" (v)
                : [pMem] "m" (*pMem)
                : "xmm0", "memory"
                ) ;
            return v[0] ;
        }
        template <>
        inline atomic64_t load64<membar_seq_cst>( atomic64_t volatile const * pMem )
        {
            // Atomically loads 64bit value by SSE instruction
            atomic64_t v = load64<membar_relaxed>( pMem ) ;
            fence<membar_seq_cst>()   ;
            return v;
        }
        static inline atomic64_t load64( atomic64_t volatile const * pMem, memory_order order )
        {
            switch (order ) {
                case membar_relaxed::order:
                    return load64<membar_relaxed>( pMem ) ;
                case membar_consume::order:
                    return load64<membar_consume>( pMem ) ;
                case membar_acquire::order:
                    return load64<membar_acquire>( pMem ) ;
                case membar_seq_cst::order:
                    return load64<membar_seq_cst>( pMem ) ;
                default:
                    assert(false)   ;
                    return load64<membar_seq_cst>( pMem ) ;
            }
        }


        template <typename ORDER>
        static inline void store64( atomic64_t volatile * pMem, atomic64_unaligned val )    ;
        template <>
        inline void store64<membar_relaxed>( atomic64_t volatile * pMem, atomic64_unaligned val )
        {
            // Atomically stores 64bit value by SSE instruction
            asm (
                "movq       %[val], %[pMem]   ;   \n\t"
                : [pMem] "=m" (*pMem)
                : [val] "x" (val)
                : "memory"
                )   ;
        }
        template <>
        inline void store64<membar_release>( atomic64_t volatile * pMem, atomic64_unaligned val )
        {
            // Atomically stores 64bit value by SSE instruction
            asm volatile (
                "movq       %[val], %[pMem]   ;   \n\t"
                : [pMem] "=m" (*pMem)
                : [val] "x" (val)
                : "memory"
                )   ;
        }
        template <>
        inline void store64<membar_seq_cst>( atomic64_t volatile * pMem, atomic64_unaligned val )
        {
            // CAS-based emulation xchg64
            atomic64_t cur ;
            do {
                cur = load64<membar_relaxed>( pMem )  ;
            } while ( !cas64<membar_seq_cst>( pMem, cur, val ))    ;
        }
        static inline void store64( atomic64_t volatile * pMem, atomic64_unaligned val, memory_order order )
        {
            switch (order ) {
                case membar_relaxed::order:
                    store64<membar_relaxed>( pMem, val ) ;
                    break;
                case membar_release::order:
                    store64<membar_release>( pMem, val ) ;
                    break;
                case membar_seq_cst::order:
                    store64<membar_seq_cst>( pMem, val ) ;
                    break;
                default:
                    assert(false)   ;
                    store64<membar_seq_cst>( pMem, val ) ;
                    break;
            }
        }

        //
        // Spin-lock primitives
        //
#include <cds/compiler/gcc/x86/atomic_spinlock.h>

        // Atomic bool
#include <cds/compiler/gcc/x86/atomic_bool.h>

    }} // namespace gcc::x86
}} // cds::atomics

namespace cds { namespace atomics {
    namespace platform {
        using namespace gcc::x86 ;
    }   // namespace platform
}} // cds::atomics
//@endcond

#endif // #ifndef __CDS_COMPILER_GCC_X86_ATOMIC_H
