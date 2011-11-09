/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_OS_TIMER_H
#define __CDS_OS_TIMER_H

#include <cds/details/defs.h>

#if CDS_OS_TYPE == CDS_OS_WIN32 || CDS_OS_TYPE == CDS_OS_WIN64
#    include <cds/os/win/timer.h>
    namespace cds { namespace OS {
        typedef Win32::Timer    Timer    ;
    }}    // namespace cds::OS
#elif CDS_OS_TYPE == CDS_OS_LINUX
#    include <cds/os/linux/timer.h>
    namespace cds { namespace OS {
        typedef Linux::Timer    Timer    ;
    }}    // namespace cds::OS

#elif CDS_OS_TYPE == CDS_OS_SUN_SOLARIS
#    include <cds/os/sunos/timer.h>
    namespace cds { namespace OS {
        typedef Sun::Timer    Timer    ;
    }}    // namespace cds::OS
#elif CDS_OS_TYPE == CDS_OS_HPUX
#    include <cds/os/hpux/timer.h>
    namespace cds { namespace OS {
        typedef Hpux::Timer    Timer    ;
    }}    // namespace cds::OS
#elif CDS_OS_TYPE == CDS_OS_AIX
#    include <cds/os/aix/timer.h>
    namespace cds { namespace OS {
        typedef Aix::Timer    Timer    ;
    }}    // namespace cds::OS
#elif CDS_OS_TYPE == CDS_OS_FREE_BSD
#   include <cds/os/free_bsd/timer.h>    
    namespace cds { namespace OS {
        typedef Free_BSD::Timer    Timer    ;
    }}    // namespace cds::OS
#else
//************************************************************************
// Other OSes
//************************************************************************
#    error Unknown operating system. Compilation aborted.
#endif

#endif    // #ifndef __CDS_OS_TIMER_H
