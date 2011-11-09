/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_THREADING_AUTO_DETECT_H
#define __CDS_THREADING_AUTO_DETECT_H

#if defined(CDS_THREADING_AUTODETECT)
    // Auto-detect appropriate threading model
#   if CDS_COMPILER == CDS_COMPILER_MSVC
        // For MSVC, CDS_THREADING_MSVC and CDS_THREADING_WIN_TLS is supported. 
        // CDS_THREADING_WIN_TLS must be explicitly defined if needed
#       if !defined(CDS_THREADING_WIN_TLS)
#           define CDS_THREADING_MSVC
#       endif
#   elif CDS_COMPILER == CDS_COMPILER_GCC
#       if !defined(CDS_THREADING_PTHREAD)
            // For GCC, either CDS_THREADING_PTHREAD or CDS_THREADING_GCC is supported
#           if CDS_PROCESSOR_ARCH == CDS_PROCESSOR_SPARC
                // It seems, GCC does not support __thread keyword for Sparc 
#               define CDS_THREADING_PTHREAD
#           elif CDS_PROCESSOR_ARCH == CDS_PROCESSOR_AMD64 || CDS_PROCESSOR_ARCH == CDS_PROCESSOR_X86
#               if CDS_COMPILER_VERSION < 40400
#                   define CDS_THREADING_GCC
#               elif CDS_COMPILER_VERSION >= 40400 && CDS_COMPILER_VERSION < 40500
                    // It seems, GCC 4.4.x has a bug with __thread keyword for amd64 and x86 platforms (see libcds bug 3128161)
                    // At least, GCC 4.4.3 does not support __thread (no compiler/linker error, core file when running)
#                   define CDS_THREADING_PTHREAD
#               else
#                   define CDS_THREADING_GCC
#               endif
#           else
#               define CDS_THREADING_GCC
#           endif   // CDS_PROCESSOR_ARCH
#       endif   // !defined(CDS_THREADING_PTHREAD) 
#   endif   // CDS_COMPILER == CDS_COMPILER_GCC
#endif  // CDS_THREADING_AUTODETECT


#if defined(CDS_THREADING_MSVC)
#   include <cds/threading/details/msvc.h>
#elif defined(CDS_THREADING_WIN_TLS)
#   include <cds/threading/details/wintls.h>
#elif defined(CDS_THREADING_PTHREAD) 
#   include <cds/threading/details/pthread.h>
#elif defined(CDS_THREADING_GCC)
#   include <cds/threading/details/gcc.h>
#elif !defined(CDS_THREADING_USER_DEFINED)
#   error "You must define one of CDS_THREADING_xxx macro before compiling the application"
#endif

#endif // #ifndef __CDS_THREADING_AUTO_DETECT_H
