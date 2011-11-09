/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_HZP_DETAILS_HP_TYPE_H
#define __CDS_GC_HZP_DETAILS_HP_TYPE_H

#include <cds/gc/details/retired_ptr.h>

namespace cds {
    namespace gc {
        namespace hzp {

            /// Hazard pointer
            typedef void *    HazardPtr    ;

            /// Free hazard pointer constant
            static const HazardPtr NullHazardPtr = NULL ;

            /// Pointer to function to free (destruct and deallocate) retired pointer of specific type
            typedef cds::gc::details::free_retired_ptr_func free_retired_ptr_func ;
        }
    }
}

#endif // #ifndef __CDS_GC_HZP_DETAILS_HP_TYPE_H


