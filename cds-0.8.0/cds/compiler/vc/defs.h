/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_COMPILER_VC_DEFS_H
#define __CDS_COMPILER_VC_DEFS_H

// Compiler version
#define CDS_COMPILER_VERSION    _MSC_VER

// Compiler name
// Supported compilers: MS VC 2005 (?), 2008, 2010
//
#if _MSC_VER == 1400
#   define  CDS_COMPILER__NAME  "MS Visual C++ 2005"
#   define  CDS_COMPILER__NICK  "msvc2005"
#elif _MSC_VER == 1500
#   define  CDS_COMPILER__NAME  "MS Visual C++ 2008"
#   define  CDS_COMPILER__NICK  "msvc2008"
#elif _MSC_VER == 1600
#   define  CDS_COMPILER__NAME  "MS Visual C++ 2010"
#   define  CDS_COMPILER__NICK  "msvc2010"
#else
#   define  CDS_COMPILER__NAME  "MS Visual C++"
#   define  CDS_COMPILER__NICK  "msvc"
#endif

// OS interface
#define CDS_OS_INTERFACE CDS_OSI_WINDOWS

// OS name
#if defined(_WIN64)
#   define CDS_OS_TYPE      CDS_OS_WIN64
#   define CDS_OS__NAME     "Win64"
#   define CDS_OS__NICK     "Win64"
#elif defined(_WIN32)
#   define CDS_OS_TYPE      CDS_OS_WIN32
#   define CDS_OS__NAME     "Win32"
#   define CDS_OS__NICK     "Win32"
#endif

// Processor architecture
#ifdef _M_IX86
#   define CDS_BUILD_BITS       32
#   define CDS_PROCESSOR_ARCH   CDS_PROCESSOR_X86
#   define CDS_PROCESSOR__NAME  "Intel x86"
#   define CDS_PROCESSOR__NICK  "x86"
#elif _M_X64
#   define CDS_BUILD_BITS       64
#   define CDS_PROCESSOR_ARCH   CDS_PROCESSOR_AMD64
#   define CDS_PROCESSOR__NAME  "AMD64"
#   define CDS_PROCESSOR__NICK  "amd64"
#else
#   define CDS_BUILD_BITS        -1
#   define CDS_PROCESSOR_ARCH    CDS_PROCESSOR_UNKNOWN
#   define CDS_PROCESSOR__NAME    "<<Undefined>>"
#   error Microsoft Visual C++ compiler is supported for x86 only
#endif


#define  __attribute__( _x )

#define  CDS_STDCALL    __stdcall

#ifdef CDS_BUILD_LIB
#   define CDS_EXPORT_API          __declspec(dllexport)
#else
#   define CDS_EXPORT_API          __declspec(dllimport)
#endif

#define alignof     __alignof

// Memory leaks detection (debug build only)
#ifdef _DEBUG
#   define _CRTDBG_MAP_ALLOC
#   define _CRTDBG_MAPALLOC
#   include <stdlib.h>
#   include <crtdbg.h>
#   define CDS_MSVC_MEMORY_LEAKS_DETECTING_ENABLED
#endif

// constexpr is not yet supported 
#define CDS_CONSTEXPR

// Variadic template support
//#define CDS_COMPILER_SUPPORTS_VARIADIC_TEMPLATE     1

// *************************************************
// Alignment macro

// VC 2005 generates error C2719 "formal parameter with __declspec(align('#')) won't be aligned"
// for function's formal parameter with align declspec
#define CDS_TYPE_ALIGNMENT(n)     __declspec( align(n) )
#define CDS_DATA_ALIGNMENT(n)     __declspec( align(n) )
#define CDS_CLASS_ALIGNMENT(n)    __declspec( align(n) )

#include <cds/compiler/atomic_integrals.h>
#include <cds/compiler/vc/compiler_barriers.h>

#endif // #ifndef __CDS_COMPILER_VC_DEFS_H
