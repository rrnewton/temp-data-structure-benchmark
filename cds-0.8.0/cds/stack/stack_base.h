/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_STACK_BASE
#define __CDS_STACK_BASE

#include <cds/atomic.h>
#include <cds/backoff_strategy.h>
#include <cds/details/allocator.h>

namespace cds { namespace stack {

    /// Lock-free stack traits
    struct Traits {

        ///< Back-off strategy. The default is cds::backoff::empty
        typedef cds::backoff::empty     backoff_strategy    ;

        /// FreeList implementation (only for stacks based on gc::tagged memory reclamation schema)
        /**
            The type \p void means standard gc::tagged::FreeList implementation
        */
        typedef void                    free_list            ;
    };

    /// Lock-free stack
    /**
        Declaration of simple lock-free stack

        \par Template parameters
            \li \p GC    lock-free memory reclamation schema (garbage collector)
            \li \p T    type of item stored in stack
            \li \p TRAITS class traits. The default is cds::stack::Traits
            \li \p ALLOCATOR node allocator. The default is \ref CDS_DEFAULT_ALLOCATOR

        There are specialization for each appropriate reclamation schema \p GC.
    */
    template <
        class GC,
        typename T,
        class TRAITS = Traits,
        class ALLOCATOR = CDS_DEFAULT_ALLOCATOR
    > class Stack ;

} } // namespace cds::stack

#endif // #ifndef __CDS_STACK_BASE
