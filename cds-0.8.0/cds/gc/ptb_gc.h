/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_PTB_GC_H
#define __CDS_GC_PTB_GC_H

namespace cds {
    namespace gc {

        // Forward declarations
        namespace ptb {
            class GarbageCollector  ;
            class ThreadGC  ;
        }

        /// Tag for "Pass the Buck" reclamation schema
        /**
            For more information about Pass the Buck reclamation schema see cds::gc::ptb namespace.
        */
        struct ptb_gc
        {
            /// Thread control data of Pass-The-Buck GC
            typedef gc::ptb::ThreadGC   thread_gc   ;
        };
    }   // namespace gc
} // namespace cds


#endif // #ifndef __CDS_GC_PTB_GC_H
