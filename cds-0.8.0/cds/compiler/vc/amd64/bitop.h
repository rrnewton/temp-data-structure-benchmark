/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_COMPILER_VC_AMD64_BITOP_H
#define __CDS_COMPILER_VC_AMD64_BITOP_H

#include <intrin.h>
#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse64)
#pragma intrinsic(_BitScanForward64)

//@cond none
namespace cds {
    namespace bitop { namespace platform { namespace vc { namespace amd64 {

        // MSB - return index (1..32) of most significant bit in nArg. If nArg == 0 return 0
#        define cds_bitop_msb32_DEFINED
        static inline int msb32( atomic32u_t nArg )
        {
            unsigned long nIndex    ;
            if ( _BitScanReverse( &nIndex, nArg ))
                return (int) nIndex + 1   ;
            return 0    ;
        }

#        define cds_bitop_msb32nz_DEFINED
        static inline int msb32nz( atomic32u_t nArg )
        {
            assert( nArg != 0 )    ;
            unsigned long nIndex    ;
            _BitScanReverse( &nIndex, nArg )    ;
            return (int) nIndex       ;
        }

        // LSB - return index (1..32) of least significant bit in nArg. If nArg == 0 return -1U
#        define cds_bitop_lsb32_DEFINED
        static inline int lsb32( atomic32u_t nArg )
        {
            unsigned long nIndex    ;
            if ( _BitScanForward( &nIndex, nArg ))
                return (int) nIndex + 1   ;
            return 0    ;
        }

#        define cds_bitop_lsb32nz_DEFINED
        static inline int lsb32nz( atomic32u_t nArg )
        {
            assert( nArg != 0 )    ;
            unsigned long nIndex    ;
            _BitScanForward( &nIndex, nArg )    ;
            return (int) nIndex       ;
        }


#        define cds_bitop_msb64_DEFINED
        static inline int msb64( atomic64u_unaligned nArg )
        {
            unsigned long nIndex    ;
            if ( _BitScanReverse64( &nIndex, nArg ))
                return (int) nIndex + 1   ;
            return 0    ;
        }

#        define cds_bitop_msb64nz_DEFINED
        static inline int msb64nz( atomic64u_unaligned nArg )
        {
            assert( nArg != 0 )    ;
            unsigned long nIndex    ;
            _BitScanReverse64( &nIndex, nArg )    ;
            return (int) nIndex       ;
        }

#        define cds_bitop_lsb64_DEFINED
        static inline int lsb64( atomic64u_unaligned nArg )
        {
            unsigned long nIndex    ;
            if ( _BitScanForward64( &nIndex, nArg ))
                return (int) nIndex + 1   ;
            return 0    ;
        }

#        define cds_bitop_lsb64nz_DEFINED
        static inline int lsb64nz( atomic64u_unaligned nArg )
        {
            assert( nArg != 0 )    ;
            unsigned long nIndex    ;
            _BitScanForward64( &nIndex, nArg )    ;
            return (int) nIndex       ;
        }

    }} // namespace vc::amd64

    using namespace vc::amd64   ;

}}}    // namespace cds::bitop::platform
//@endcond

#endif    // #ifndef __CDS_COMPILER_VC_AMD64_BITOP_H
