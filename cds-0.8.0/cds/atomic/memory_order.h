/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ATOMIC_MEMORY_ORDER_H
#define __CDS_ATOMIC_MEMORY_ORDER_H

namespace cds {

    /// C++0x memory order enum
    /**
        At that moment, the compilers do not implement C++0x Memory Model proposal.
        Therefore, we need define C++0x \p memory_order enum in the CDS library.
    */
    enum memory_order {
        memory_order_relaxed,   ///< relaxed memory semantics
        memory_order_consume,   ///< consume memory semantics
        memory_order_acquire,   ///< acquire memory semantics
        memory_order_release,   ///< release memory semantics
        memory_order_acq_rel,   ///< acquire-release memory semantics
        memory_order_seq_cst,   ///< sequentially consistent memory semantics

        // Insert new members before this line
        end_of_memory_order     ///< helper value, for test purpose only. Do not use directly
    };

    /// Wrapper for \ref memory_order_relaxed
    struct membar_relaxed {
        static const memory_order order = memory_order_relaxed;     ///< Corresponding memory order constant
    };
    /// Wrapper for \ref memory_order_consume
    struct membar_consume {
        static const memory_order order = memory_order_consume;     ///< Corresponding memory order constant
    };
    /// Wrapper for \ref memory_order_acquire
    struct membar_acquire {
        static const memory_order order = memory_order_acquire;     ///< Corresponding memory order constant
    };
    /// Wrapper for \ref memory_order_release
    struct membar_release {
        static const memory_order order = memory_order_release;     ///< Corresponding memory order constant
    };
    /// Wrapper for \ref memory_order_acq_rel
    struct membar_acq_rel {
        static const memory_order order = memory_order_acq_rel;     ///< Corresponding memory order constant
    };
    /// Wrapper for \ref memory_order_seq_cst
    struct membar_seq_cst {
        static const memory_order order = memory_order_seq_cst;     ///< Corresponding memory order constant
    };

}   // namespace cds

#endif  // #ifndef __CDS_ATOMIC_MEMORY_ORDER_H
