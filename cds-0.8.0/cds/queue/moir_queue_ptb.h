/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_QUEUE_MOIR_QUEUE_PTB_H
#define __CDS_QUEUE_MOIR_QUEUE_PTB_H

/*
    Editions:
        2010.10.13    Maxim.Khiszinsky    Created
*/

#include <cds/queue/details/moir_queue_hp_impl.h>
#include <cds/queue/msqueue_ptb.h>

namespace cds {
    namespace queue {

        /// A variation of Michael & Scott lock-free queue based on Pass-The-Buck GC
        /**
            The algorithm is a variation of Michael & Scott's queue algorithm. It overloads dequeue function.

        \par Source:
            \li [2000] Simon Doherty, Lindsay Groves, Victor Luchangco, Mark Moir
                "Formal Verification of a practical lock-free queue algorithm"
            \li [2002] Maged M.Michael "Safe memory reclamation for dynamic lock-freeobjects using atomic reads and writes"
            \li [2003] Maged M.Michael "Hazard Pointers: Safe memory reclamation for lock-free objects"

        \par Template parameters:
            \li \p T        Type of data saved in queue's node
            \li \p TRAITS    Traits class (see @ref cds::queue::traits)
            \li \p ALLOCATOR Memory allocator class

        \par
            Interface: see \ref queue_common_interface.
        */
        template <typename T, class TRAITS, class ALLOCATOR>
        class MoirQueue< gc::ptb_gc, T, TRAITS, ALLOCATOR >:
            public details::moir_queue_implementation< MSQueue< gc::ptb_gc, T, TRAITS, ALLOCATOR > >
        {};

    } // namespace queue
} // namespace cds

#endif // #ifndef __CDS_QUEUE_MOIR_QUEUE_HZP_H
