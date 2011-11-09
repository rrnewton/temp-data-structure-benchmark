/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __UNIT_BAG_TYPES_H
#define __UNIT_BAG_TYPES_H

#include <cds/bag/sbag_hrc_gccthread.h>


#include "lock/win32_lock.h"
#include <boost/thread/mutex.hpp>

namespace bag {

    template <typename VALUE, int NR_THREADS>
    struct Types {
        typedef cds::bag::SBag<cds::gc::hrc_gc, VALUE, NR_THREADS>           SBag_HRC        ;
    };
}

#endif // #ifndef __UNIT_BAG_TYPES_H
