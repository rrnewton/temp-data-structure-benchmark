/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_MEMORY_MICHAEL_ALLOCATOR_OSALLOC_STAT_H
#define __CDS_MEMORY_MICHAEL_ALLOCATOR_OSALLOC_STAT_H

#include <cds/atomic.h>

namespace cds { namespace memory { namespace michael {

    /// Statistics for large  (allocated directly from %OS) block
    template <typename INC_FENCE = membar_release, typename READ_FENCE = membar_acquire >
    struct os_allocated_atomic
    {
        ///@cond
        atomic<size_t>          nAllocCount         ;   ///< Event count of large block allocation from %OS
        atomic<size_t>          nFreeCount          ;   ///< Event count of large block deallocation to %OS
        atomic<atomic64u_t>     nBytesAllocated     ;   ///< Total size of allocated large blocks, in bytes
        atomic<atomic64u_t>     nBytesDeallocated   ;   ///< Total size of deallocated large blocks, in bytes
        ///@endcond

        /// Adds \p nSize to nBytesAllocated counter
        void incBytesAllocated( size_t nSize )
        {
            nAllocCount.inc<INC_FENCE>()    ;
            nBytesAllocated.xadd<INC_FENCE>( nSize )    ;
        }

        /// Adds \p nSize to nBytesDeallocated counter
        void incBytesDeallocated( size_t nSize )
        {
            nFreeCount.inc<INC_FENCE>() ;
            nBytesDeallocated.xadd<INC_FENCE>( nSize )  ;
        }

        /// Returns count of \p alloc and \p alloc_aligned function call (for large block allocated directly from %OS)
        size_t allocCount() const
        {
            return nAllocCount.load<READ_FENCE>()   ;
        }

        /// Returns count of \p free and \p free_aligned function call (for large block allocated directly from %OS)
        size_t freeCount() const
        {
            return nFreeCount.load<READ_FENCE>()   ;
        }

        /// Returns current value of nBytesAllocated counter
        atomic64u_t allocatedBytes() const
        {
            return nBytesAllocated.load<READ_FENCE>()   ;       
        }

        /// Returns current value of nBytesAllocated counter
        atomic64u_t deallocatedBytes() const
        {
            return nBytesDeallocated.load<READ_FENCE>() ;
        }
    };

    /// Dummy statistics for large (allocated directly from %OS) block
    /**
        This class does not gather any statistics. 
        Class interface is the same as \ref os_allocated_atomic.
    */
    struct os_allocated_empty
    {
    //@cond
        /// Adds \p nSize to nBytesAllocated counter
        void incBytesAllocated( size_t nSize )
        {}

        /// Adds \p nSize to nBytesDeallocated counter
        void incBytesDeallocated( size_t nSize )
        {}

        /// Returns count of \p alloc and \p alloc_aligned function call (for large block allocated directly from OS)
        size_t allocCount() const
        {
            return 0    ;
        }

        /// Returns count of \p free and \p free_aligned function call (for large block allocated directly from OS)
        size_t freeCount() const
        {
            return 0    ;
        }

        /// Returns current value of nBytesAllocated counter
        atomic64u_t allocatedBytes() const
        {
            return 0    ;
        }

        /// Returns current value of nBytesAllocated counter
        atomic64u_t deallocatedBytes() const
        {
            return 0    ;
        }
    //@endcond
    };


}}} // namespace cds::memory::michael

#endif  /// __CDS_MEMORY_MICHAEL_ALLOCATOR_OSALLOC_STAT_H
