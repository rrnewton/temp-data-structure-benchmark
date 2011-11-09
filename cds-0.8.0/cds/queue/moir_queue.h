/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_QUEUE_MOIR_QUEUE_H
#define __CDS_QUEUE_MOIR_QUEUE_H

/*
    Moir lock-free queue algorithm. It is enhancement of Michael & Scott lock-free queue

    Editions:
        2008.03.30    Maxim.Khiszinsky    Created
        2008.10.02    Maxim.Khiszinsky    Refactoring: GC parameter included to template declaration
*/

#include <cds/queue/msqueue.h>

namespace cds {
    /// Various queue implementations
    namespace queue {
        /// Moir queue
        /*
            Declaration of Moir queue algorithm. This algorithm is enhancement of Michael & Scott lock-free queue MSQueue
            \par Template parameters
                \li \p GC    lock-free reclamation schema (garbage collector)
                \li \p T    type of itema stored in queue
                \li \p TRAITS class traits. The default is @ref cds::queue::traits
                \li \p ALLOCATOR node allocator. The default is \ref CDS_DEFAULT_ALLOCATOR

            There are specialization for each appropriate reclamation schema \p GC.
        */
        template < typename GC,
            typename T,
            typename TRAITS = traits,
            class ALLOCATOR = CDS_DEFAULT_ALLOCATOR >
        class MoirQueue    ;


    }    // namespace queue
}    // namespace cds

#endif // #ifndef __CDS_QUEUE_MOIR_QUEUE_H
