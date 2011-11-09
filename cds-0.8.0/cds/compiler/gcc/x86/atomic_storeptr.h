/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_COMPILER_GCC_X86_ATOMIC_STOREPTR_H
#define __CDS_COMPILER_GCC_X86_ATOMIC_STOREPTR_H

//@cond none
//
// This file is used on x86 and amd64 architecture for GCC compiler
// It is included into appropriate namespace
// Do not use the file directly!!!
//

template <typename ORDER>
static inline void storeptr( pointer_t volatile * pMem, pointer_t val ) ;
template <>
inline void storeptr<membar_relaxed>( pointer_t volatile * pMem, pointer_t val )
{
    assert( pMem != NULL )  ;
    CDS_COMPILER_RW_BARRIER ;
    *pMem = val ;
}
template <>
inline void storeptr<membar_release>( pointer_t volatile * pMem, pointer_t val )
{
    assert( pMem != NULL )  ;
    CDS_COMPILER_RW_BARRIER ;
    *pMem = val ;
    CDS_COMPILER_RW_BARRIER ;
}
template <>
inline void storeptr<membar_seq_cst>( pointer_t volatile * pMem, pointer_t val )
{
    xchgptr<membar_seq_cst>( pMem, val ) ;
}
static inline void storeptr( pointer_t volatile * pMem, pointer_t val, memory_order order )
{
    switch (order ) {
        case membar_relaxed::order:
            storeptr<membar_relaxed>( pMem, val ) ;
            break;
        case membar_release::order:
            storeptr<membar_release>( pMem, val ) ;
            break;
        case membar_seq_cst::order:
            storeptr<membar_seq_cst>( pMem, val ) ;
            break;
        default:
            assert(false)   ;
            storeptr<membar_seq_cst>( pMem, val ) ;
            break;
    }
}

//@endcond
#endif // #ifndef __CDS_COMPILER_GCC_X86_ATOMIC_STOREPTR_H
