/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ARH_COMPILER_DEFS_H
#define __CDS_ARH_COMPILER_DEFS_H

#if CDS_COMPILER == CDS_COMPILER_MSVC
#   include <cds/compiler/vc/defs.h>
#elif CDS_COMPILER == CDS_COMPILER_GCC
#   include <cds/compiler/gcc/defs.h>
#elif CDS_COMPILER == CDS_COMPILER_INTEL
#   include <cds/compiler/ic/defs.h>
#elif CDS_COMPILER == CDS_COMPILER_UNKNOWN
#    error Unknown compiler. Compilation aborted
#else
#   error Unknown value of CDS_COMPILER macro
#endif

#ifndef CDS_STDCALL
#   define CDS_STDCALL
#endif

#ifndef CDS_EXPORT_API
#   define CDS_EXPORT_API
#endif

#endif  // #ifndef __CDS_ARH_COMPILER_DEFS_H
