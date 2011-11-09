/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_COMPILER_GCC_IA64_BACKOFF_H
#define __CDS_COMPILER_GCC_IA64_BACKOFF_H

//@cond none

namespace cds { namespace backoff {
    namespace gcc { namespace ia64 {

#       define CDS_backoff_pause_defined
        static inline void backoff_pause( unsigned int nLoop = 0x000003FF )
        {
            asm volatile ( "hint @pause" ) ;
        }

#       define CDS_backoff_hint_defined
        static inline void backoff_hint()
        {
            asm volatile ( "hint @pause;;" ) ;
        }

#       define CDS_backoff_nop_defined
        static inline void backoff_nop()
        {
            asm volatile ( "nop;;" ) ;
        }

    }} // namespace gcc::ia64

    namespace platform {
        using namespace gcc::ia64 ;
    }
}}  // namespace cds::backoff

//@endcond
#endif  // #ifndef __CDS_COMPILER_GCC_IA64_BACKOFF_H
