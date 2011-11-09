/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_COMPILER_GCC_X86_ATOMIC_XADD32_H
#define __CDS_COMPILER_GCC_X86_ATOMIC_XADD32_H

//@cond none
//
// This file is used on x86 and amd64 architecture for GCC compiler
// It is included into appropriate namespace
// Do not use the file directly!!!
//

#define CDS_xadd32_defined
template <typename ORDER>
static inline atomic32_t xadd32( atomic32_t volatile * pMem, atomic32_t val )
{
    atomic32_t ret;
    asm volatile (
        " lock; xaddl %2, %0"
        : "=m" (*pMem), "=a" (ret)
        : "a" (val), "m" (*pMem)
        : "cc", "memory"
        );
    return (ret);
}
static inline atomic32_t xadd32( atomic32_t volatile * pMem, atomic32_t val, memory_order order )
{
    return xadd32<membar_relaxed>( pMem, val )    ;
}


//@endcond
#endif // #ifndef __CDS_COMPILER_GCC_X86_ATOMIC_XADD32_H
