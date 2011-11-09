/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_COMPILER_VC_AMD64_BACKOFF_H
#define __CDS_COMPILER_VC_AMD64_BACKOFF_H

//@cond none

namespace cds { namespace backoff {
    namespace vc { namespace amd64 {

#       define CDS_backoff_pause_defined
        static inline void backoff_pause( unsigned int nLoop = 0x000003FF )
        {
            for ( unsigned int i = 0; i < nLoop; i++ )
                __nop() ;
        }

#       define CDS_backoff_nop_defined
        static inline void backoff_nop()
        {
            __nop() ;
        }

    }} // namespace vc::amd64

    namespace platform {
        using namespace vc::amd64 ;
    }
}}  // namespace cds::backoff

//@endcond
#endif  // #ifndef __CDS_COMPILER_VC_AMD64_BACKOFF_H
