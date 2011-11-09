/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_HZP_GC_H
#define __CDS_GC_HZP_GC_H

namespace cds {
    /// Garbage collecting - Safe memory reclamation schemas
    /**
        The namespace declares different memory reclamation schemas
    */
    namespace gc {

        // Forward declarations
        namespace hzp {
            class ThreadGC  ;
        }

        /// Tag for Michael's Hazard Pointer reclamation schema
        /**
            For more information about Michael's Hazard Pointer reclamation schema see cds::gc::hzp namespace.
        */
        struct hzp_gc
        {
            /// Thread control data of Hazard pointer GC
            typedef gc::hzp::ThreadGC   thread_gc   ;
        };
    }   // namespace gc
} // namespace cds


#endif // #ifndef __CDS_GC_HZP_GC_H
