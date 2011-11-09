/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_COMPILER_GCC_X86_ATOMIC_STORE32_H
#define __CDS_COMPILER_GCC_X86_ATOMIC_STORE32_H

//@cond none
//
// This file is used on x86 and amd64 architecture for GCC compiler
// It is included into appropriate namespace
// Do not use the file directly!!!
//

template <typename ORDER>
static inline void store32( atomic32_t volatile * pMem, atomic32_t val )    ;
template <>
inline void store32<membar_relaxed>( atomic32_t volatile * pMem, atomic32_t val )
{
    assert( pMem != NULL )  ;
    CDS_COMPILER_RW_BARRIER ;
    *pMem = val ;
}
template <>
inline void store32<membar_release>( atomic32_t volatile * pMem, atomic32_t val )
{
    assert( pMem != NULL )  ;
    CDS_COMPILER_RW_BARRIER ;
    *pMem = val ;
    CDS_COMPILER_RW_BARRIER ;
}
template <>
inline void store32<membar_seq_cst>( atomic32_t volatile * pMem, atomic32_t val )
{
    xchg32<membar_seq_cst>( pMem, val )  ;
}
static inline void store32( atomic32_t volatile * pMem, atomic32_t val, memory_order order )
{
    switch (order ) {
        case membar_relaxed::order:
            store32<membar_relaxed>( pMem, val ) ;
            break;
        case membar_release::order:
            store32<membar_release>( pMem, val ) ;
            break;
        case membar_seq_cst::order:
            store32<membar_seq_cst>( pMem, val ) ;
            break;
        default:
            assert(false)   ;
            store32<membar_seq_cst>( pMem, val ) ;
            break;
    }
}


//@endcond
#endif // #ifndef __CDS_COMPILER_GCC_X86_ATOMIC_STORE32_H
