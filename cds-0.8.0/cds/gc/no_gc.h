/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_NO_GC_H
#define __CDS_GC_NO_GC_H

namespace cds {
    /// Garbage collecting - Safe memory reclamation schemas
    namespace gc {
        /// Tag for empty GC schema for lock-free objects without deletion of items (persistent data structure)
        /**
            Sometimes, a task requires a persistent data structure that does not support deletion of its nodes.
            For this purpose the marker tag no_gc is used for GC template parameter of persistent data structure.
            The no_gc specialization of the classes is usually lighter that its full functionality analogues.
        */
        struct no_gc
        {};
    }    // namespace gc
} // namespace cds

#endif // #ifndef __CDS_GC_NO_GC_H
