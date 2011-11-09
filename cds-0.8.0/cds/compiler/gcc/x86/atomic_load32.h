/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_COMPILER_GCC_X86_ATOMIC_LOAD32_H
#define __CDS_COMPILER_GCC_X86_ATOMIC_LOAD32_H

//@cond none
//
// This file is used on x86 and amd64 architecture for GCC compiler
// It is included into appropriate namespace
// Do not use the file directly!!!
//

template <typename ORDER>
static inline atomic32_t load32( atomic32_t volatile const * pMem ) ;
template <>
inline atomic32_t load32<membar_relaxed>( atomic32_t volatile const * pMem )
{
    CDS_COMPILER_RW_BARRIER ;
    return *pMem        ;
}
template <>
inline atomic32_t load32<membar_consume>( atomic32_t volatile const * pMem )
{
    return load32<membar_relaxed>( pMem )   ;
}
template <>
inline atomic32_t load32<membar_acquire>( atomic32_t volatile const * pMem )
{
    CDS_COMPILER_RW_BARRIER     ;
    atomic32_t ret =  *pMem     ;
    CDS_COMPILER_RW_BARRIER     ;
    return ret  ;
}

template <>
inline atomic32_t load32<membar_seq_cst>( atomic32_t volatile const * pMem )
{
    atomic32_t ret;
    asm volatile (
        " lock; xaddl %1, %2"
        : "=a" (ret)
        : "a" (0), "m" (*pMem)
        : "cc", "memory"
        );
    return (ret);
/*
    fence<membar_seq_cst>() ;
    return *pMem        ;
*/
}
static inline atomic32_t load32( atomic32_t volatile const * pMem, memory_order order )
{
    switch (order ) {
        case membar_relaxed::order:
            return load32<membar_relaxed>( pMem ) ;
        case membar_consume::order:
            return load32<membar_consume>( pMem ) ;
        case membar_acquire::order:
            return load32<membar_acquire>( pMem ) ;
        case membar_seq_cst::order:
            return load32<membar_seq_cst>( pMem ) ;
        default:
            assert(false)   ;
            return load32<membar_seq_cst>( pMem ) ;
    }
}


//@endcond
#endif // #ifndef __CDS_COMPILER_GCC_X86_ATOMIC_LOAD32_H
