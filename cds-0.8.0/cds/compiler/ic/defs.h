/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_COMPILER_IC_DEFS_H
#define __CDS_COMPILER_IC_DEFS_H

#error "Intel C++ compiler is not supported yet"

#define    CDS_COMPILER_VERSION    __INTEL_COMPILER
#define  CDS_COMPILER__NAME    "Intel C++ "
#define  CDS_COMPILER__NICK    "ic"

#if defined( _WIN64 )
#   define CDS_OS_INTERFACE     CDS_OSI_WINDOWS
#    define CDS_OS_TYPE            CDS_OS_WIN64
#    define CDS_OS__NAME            "win64"
#    define CDS_OS__NICK            "win64"
#elif defined( _WIN32 )
#   define CDS_OS_INTERFACE      CDS_OSI_WINDOWS
#    define CDS_OS_TYPE            CDS_OS_WIN32
#    define CDS_OS__NAME            "win32"
#    define CDS_OS__NICK            "win32"
#else
#   error "Intel C++: unknown OS type. Compilation aborted"
#endif

#if defined(_M_AMD64)
#   define    CDS_PROCESSOR_ARCH    CDS_PROCESSOR_X86
#   define  CDS_BUILD_BITS       32
#    define    CDS_PROCESSOR__NAME    "AMD x86-64"
#    define    CDS_PROCESSOR__NICK    "AMD64"
#elif defined(_M_X86)
#   define    CDS_PROCESSOR_ARCH    CDS_PROCESSOR_X86
#   define  CDS_BUILD_BITS       32
#   define    CDS_PROCESSOR__NAME    "Intel x86-64"
#    define    CDS_PROCESSOR__NICK    "x86"
#elif defined(_M_IX86)
#   define    CDS_PROCESSOR_ARCH    CDS_PROCESSOR_X86
#   define  CDS_BUILD_BITS       32
#   define    CDS_PROCESSOR__NAME    "Intel x86"
#   define    CDS_PROCESSOR__NICK    "x86"
#else
#   error "Intel C++: unknown processor architecture. Compilation aborted"
#endif

// constexpr is not yet supported 
#define CDS_CONSTEXPR


#endif // #ifndef __CDS_COMPILER_IC_DEFS_H
