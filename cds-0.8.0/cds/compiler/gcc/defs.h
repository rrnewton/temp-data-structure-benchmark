/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_COMPILER_GCC_DEFS_H
#define __CDS_COMPILER_GCC_DEFS_H

// Compiler version
#define CDS_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#if CDS_COMPILER_VERSION < 40300
#   error "Compiler version error. GCC version 4.3.0 and above is supported"
#endif

// Compiler name
#ifdef __VERSION__
#   define  CDS_COMPILER__NAME    ("GNU C++ " __VERSION__)
#else
#   define  CDS_COMPILER__NAME    "GNU C++"
#endif
#define  CDS_COMPILER__NICK        "gcc"

// OS interface && OS name
#if defined( __linux__ )
#   define CDS_OS_INTERFACE     CDS_OSI_UNIX
#   define CDS_OS_TYPE          CDS_OS_LINUX
#   define CDS_OS__NAME         "linux"
#   define CDS_OS__NICK         "linux"
#elif defined( __sun__ )
#   define CDS_OS_INTERFACE     CDS_OSI_UNIX
#   define CDS_OS_TYPE          CDS_OS_SUN_SOLARIS
#   define CDS_OS__NAME         "Sun Solaris"
#   define CDS_OS__NICK         "sun"
#elif defined( __hpux__ )
#   define CDS_OS_INTERFACE     CDS_OSI_UNIX
#   define CDS_OS_TYPE          CDS_OS_HPUX
#   define CDS_OS__NAME         "HP-UX"
#   define CDS_OS__NICK         "hpux"
#elif defined( _AIX )
#   define CDS_OS_INTERFACE     CDS_OSI_UNIX
#   define CDS_OS_TYPE          CDS_OS_AIX
#   define CDS_OS__NAME         "AIX"
#   define CDS_OS__NICK         "aix"
#elif defined( __FreeBSD__ )
#   define CDS_OS_INTERFACE     CDS_OSI_UNIX
#   define CDS_OS_TYPE          CDS_OS_FREE_BSD
#   define CDS_OS__NAME         "FreeBSD"
#   define CDS_OS__NICK         "freebsd"
#elif defined( __OpenBSD__ )
#   define CDS_OS_INTERFACE     CDS_OSI_UNIX
#   define CDS_OS_TYPE          CDS_OS_OPEN_BSD
#   define CDS_OS__NAME         "OpenBSD"
#   define CDS_OS__NICK         "openbsd"
#elif defined( __NetBSD__ )
#   define CDS_OS_INTERFACE     CDS_OSI_UNIX
#   define CDS_OS_TYPE          CDS_OS_NET_BSD
#   define CDS_OS__NAME         "NetBSD"
#   define CDS_OS__NICK         "netbsd"
#else
#    error GCC: unknown OS type. Compilation aborted
#endif

// Processor architecture
#if defined(__x86_64__) || defined(__amd64__) || defined(__amd64)
#   define CDS_PROCESSOR_ARCH    CDS_PROCESSOR_AMD64
#   define CDS_BUILD_BITS        64
#   define CDS_PROCESSOR__NAME   "Intel x86-64"
#   define CDS_PROCESSOR__NICK   "amd64"
#elif defined(__i386__)
#   define CDS_PROCESSOR_ARCH    CDS_PROCESSOR_X86
#   define CDS_BUILD_BITS        32
#   define CDS_PROCESSOR__NAME   "Intel x86"
#   define CDS_PROCESSOR__NICK   "x86"
#elif defined(sparc) || defined (__sparc__)
#   define CDS_PROCESSOR_ARCH    CDS_PROCESSOR_SPARC
#   define CDS_PROCESSOR__NAME   "Sparc"
#   define CDS_PROCESSOR__NICK   "sparc"
#   ifdef __arch64__
#       define CDS_BUILD_BITS   64
#    else
#       error Sparc 32bit is not supported
#    endif
#elif defined( __ia64__)
#    define CDS_PROCESSOR_ARCH    CDS_PROCESSOR_IA64
#    define CDS_BUILD_BITS        64
#    define CDS_PROCESSOR__NAME   "Intel IA64"
#    define CDS_PROCESSOR__NICK   "ia64"
#elif defined(_ARCH_PPC64)
#    define CDS_PROCESSOR_ARCH    CDS_PROCESSOR_PPC64
#    define CDS_BUILD_BITS        64
#    define CDS_PROCESSOR__NAME   "IBM PowerPC64"
#    define CDS_PROCESSOR__NICK   "ppc64"
#else
#    error "GCC: unknown processor architecture. Compilation aborted"
#endif

#ifndef __declspec
#   define __declspec( _x )
#endif

#if CDS_PROCESSOR_ARCH == CDS_PROCESSOR_X86
#   define CDS_STDCALL __attribute__((stdcall))
#else
#   define CDS_STDCALL
#endif

#define alignof __alignof__

// constexpr is not yet supported 
#define CDS_CONSTEXPR

// Variadic template support (only if -std=c++0x compile-line option provided)
#ifdef __GXX_EXPERIMENTAL_CXX0X__
#define CDS_COMPILER_SUPPORTS_VARIADIC_TEMPLATE
#endif

// *************************************************
// Alignment macro

#define CDS_TYPE_ALIGNMENT(n)   __attribute__ ((aligned (n)))
#define CDS_CLASS_ALIGNMENT(n)  __attribute__ ((aligned (n)))
#define CDS_DATA_ALIGNMENT(n)   __attribute__ ((aligned (n)))

#include <cds/compiler/atomic_integrals.h>
#include <cds/compiler/gcc/compiler_barriers.h>

#endif // #ifndef __CDS_COMPILER_GCC_DEFS_H
