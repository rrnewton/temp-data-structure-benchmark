/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_HRC_GC_H
#define __CDS_GC_HRC_GC_H

namespace cds {
    /// Garbage collecting - Safe memory reclamation schemas
    namespace gc {

        // Forward declarations
        namespace hrc {
            class ThreadGC  ;
        }

        /// Tag for Gidenstam's memory reclamation schema.
        /**
            For more information about Gidenstam's memory reclamation schema see cds::gc::hrc namespace
        */
        struct hrc_gc
        {
            /// Thread control data of Gidenstam's GC
            typedef gc::hrc::ThreadGC   thread_gc   ;
        };
    } // namespace gc

} // namespace cds


#endif // #ifndef __CDS_GC_HRC_GC_H
