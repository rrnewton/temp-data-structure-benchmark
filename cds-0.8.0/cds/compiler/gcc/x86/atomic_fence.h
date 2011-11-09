/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_COMPILER_GCC_X86_ATOMIC_FENCE_H
#define __CDS_COMPILER_GCC_X86_ATOMIC_FENCE_H

//@cond none
//
// This file is used on x86 and amd64 architecture for GCC compiler
// It is included into appropriate namespace
// Do not use the file directly!!!
//

template <typename ORDER>
static inline void fence()
{
    CDS_COMPILER_RW_BARRIER ;
}

template <>
inline void fence<membar_seq_cst>()
{
    asm volatile ( "mfence" : : : "memory" )    ;
}

static inline void fence(memory_order order)
{
    switch( order ) {
                case membar_relaxed::order:
                case membar_release::order:
                case membar_consume::order:
                case membar_acquire::order:
                case membar_acq_rel::order:
                    break;
                default:
                    assert( false ) ;   // Are we forgot anything?..
                case membar_seq_cst::order:
                    fence<membar_seq_cst>()   ;
                    break   ;
    }
}

#endif // #ifndef __CDS_COMPILER_GCC_X86_ATOMIC_FENCE_H
