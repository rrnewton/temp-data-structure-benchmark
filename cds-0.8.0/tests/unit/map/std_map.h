/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __UNIT_STD_MAP_H
#define __UNIT_STD_MAP_H

#if CDS_COMPILER == CDS_COMPILER_MSVC
#   include "map/std_map_vc.h"
#elif CDS_COMPILER == CDS_COMPILER_GCC
#   include "map/std_map_gcc.h"
#else
#   error "std::map is undefined for this compiler"
#endif

#endif // #ifndef __UNIT_STD_HASH_MAP_H
