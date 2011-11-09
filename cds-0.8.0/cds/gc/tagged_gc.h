/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_TAGGED_GC_SELECTOR_H
#define __CDS_GC_TAGGED_GC_SELECTOR_H

// The platform must support dword CAS primitive (CDS_DWORD_CAS_SUPPORTED must be defined)

namespace cds {
    /// Garbage collecting - Safe memory reclamation schemas
    namespace gc {

        /// Tag for Tagged Pointer reclamation schema
        /**
            For more information about tagged pointer schema see cds::gc::tagged namespace
        */
        struct tagged_gc
        {};
    } // namespace gc

} // namespace cds

#endif // #ifndef __CDS_GC_TAGGED_GC_SELECTOR_H
