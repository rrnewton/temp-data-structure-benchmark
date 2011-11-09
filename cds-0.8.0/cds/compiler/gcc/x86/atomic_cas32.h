/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_COMPILER_GCC_X86_ATOMIC_CAS32_H
#define __CDS_COMPILER_GCC_X86_ATOMIC_CAS32_H

//@cond none
//
// This file is used on x86 and amd64 architecture for GCC compiler
// It is included into appropriate namespace
// Do not use the file directly!!!
//

template <typename SUCCESS_ORDER>
static inline bool cas32( atomic32_t volatile * pMem, atomic32_t expected, atomic32_t desired )
{
#       if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4) || CDS_COMPILER_VERSION > 40099
    return __sync_bool_compare_and_swap( pMem, expected, desired ) ;
#       else
    bool bRet    ;
    asm volatile (
        "lock; cmpxchgl %2, %0; setz %1"
        : "+m" (*pMem), "=q" (bRet)
        : "r" (desired), "m" (*pMem), "a" (expected)
        : "cc", "memory"
        );
    return bRet;
#       endif
}
static inline bool cas32( atomic32_t volatile * pMem, atomic32_t expected, atomic32_t desired, memory_order success_order, memory_order failure_order )
{
    return cas32<membar_relaxed>( pMem, expected, desired ) ;
}

template <typename SUCCESS_ORDER>
static inline atomic32_t vcas32(  atomic32_t volatile * pMem, atomic32_t expected, atomic32_t desired )
{
#       if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4) || CDS_COMPILER_VERSION > 40099
    return __sync_val_compare_and_swap( pMem, expected, desired ) ;
#       else
    atomic32_t nRet    ;
    asm volatile (
        "lock; cmpxchgl %2, %0;"
        : "+m" (*pMem), "=a" (nRet)
        : "r" (desired), "m" (*pMem), "a" (expected)
        : "cc", "memory"
        );
    return nRet;
#       endif
}
static inline atomic32_t vcas32(  atomic32_t volatile * pMem, atomic32_t expected, atomic32_t desired, memory_order success_order, memory_order failure_order )
{
    return vcas32<membar_relaxed>( pMem, expected, desired ) ;
}

//@endcond
#endif // #ifndef __CDS_COMPILER_GCC_X86_ATOMIC_CAS32_H
