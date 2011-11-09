/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_DETAILS_IS_ALIGNED_H
#define __CDS_DETAILS_IS_ALIGNED_H

#include <cds/compiler/atomic_integrals.h>

namespace cds { namespace details {

    /// Checks if the pointer \p p \p ALIGN is aligned
    /**
        \p ALIGN must be power of 2.

        The function is mostly intended for run-time assertion
    */
    template <int ALIGN, typename T>
    static inline bool is_aligned(T const * p)
    {
        return (((uptr_atomic_t)p) & uptr_atomic_t(ALIGN - 1)) == 0   ;
    }

    /// Checks if the pointer \p p \p nAlign is aligned
    /**
        \p nAlign must be power of 2.

        The function is mostly intended for run-time assertion
    */
    template <typename T>
    static inline bool is_aligned(T const * p, size_t nAlign)
    {
        return (((uptr_atomic_t)p) & uptr_atomic_t(nAlign - 1)) == 0   ;
    }

}} // namespace cds::details

#endif // #ifndef __CDS_DETAILS_IS_ALIGNED_H
