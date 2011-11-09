/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_COMPILER_GCC_AMD64_BACKOFF_H
#define __CDS_COMPILER_GCC_AMD64_BACKOFF_H

//@cond none

namespace cds { namespace backoff {
    namespace gcc { namespace amd64 {

#       define CDS_backoff_pause_defined
        static inline void backoff_pause( unsigned int nLoop = 0x000003FF )
        {
            asm volatile (
                "andl %[nLoop], %%ecx;      \n\t"
                "cmovzl %[nLoop], %%ecx;    \n\t"
                "rep; "
                "nop;   \n\t"
                : /*no output*/
                : [nLoop] "r" (nLoop)
                : "ecx", "cc"
                )    ;
        }

#       define CDS_backoff_nop_defined
        static inline void backoff_nop()
        {
            asm volatile ( "nop;" );
        }

    }} // namespace gcc::amd64

    namespace platform {
        using namespace gcc::amd64 ;
    }
}}  // namespace cds::backoff

//@endcond
#endif  // #ifndef __CDS_COMPILER_GCC_AMD64_BACKOFF_H
