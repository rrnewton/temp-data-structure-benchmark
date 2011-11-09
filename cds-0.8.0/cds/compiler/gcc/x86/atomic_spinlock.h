/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_COMPILER_GCC_X86_ATOMIC_SPINLOCK_H
#define __CDS_COMPILER_GCC_X86_ATOMIC_SPINLOCK_H

//@cond none
//
// This file is used on x86 and amd64 architecture for GCC compiler
// It is included into appropriate namespace
// Do not use the file directly!!!
//

#define CDS_spinlock_defined
typedef unsigned char    spinlock_t    ;
static inline bool spinlock_try_acquire( spinlock_t volatile * pDest )
{
#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_1
    return __sync_bool_compare_and_swap_1( pDest, 0, 1 );
#else
    bool bRet    ;
    asm volatile (
        "xorw %%ax, %%ax    ;    \n\t"
        "orb  $1, %%ah         ;    \n\t"
        "lock; cmpxchgb %%ah, %[dest];    \n\t"
        "setz %[ret]        ;   \n\t"
        : [dest] "+m"(*pDest), [ret] "=a"(bRet)
        : "m"(*pDest)
        : "memory", "cc"
        );
    return bRet    ;
#endif
}
static inline void spinlock_release( spinlock_t volatile * pDest )
{
    CDS_COMPILER_RW_BARRIER ;
    *pDest = 0    ;
    CDS_COMPILER_RW_BARRIER ;
}

//@endcond
#endif // #ifndef __CDS_COMPILER_GCC_X86_ATOMIC_SPINLOCK_H
