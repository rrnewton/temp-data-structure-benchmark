/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_OS_SYSERROR_H
#define __CDS_OS_SYSERROR_H

#include <cds/details/defs.h>

#if CDS_OS_TYPE == CDS_OS_WIN32
#    include <cds/os/win/syserror.h>
#elif CDS_OS_TYPE == CDS_OS_LINUX 
#    include <cds/os/posix/syserror.h>
#elif CDS_OS_TYPE == CDS_OS_SUN_SOLARIS
#    include <cds/os/posix/syserror.h>
#elif CDS_OS_TYPE == CDS_OS_HPUX
#    include <cds/os/posix/syserror.h>
#elif CDS_OS_TYPE == CDS_OS_AIX
#    include <cds/os/posix/syserror.h>
#elif CDS_OS_TYPE == CDS_OS_FREE_BSD || CDS_OS_TYPE == CDS_OS_OPEN_BSD || CDS_OS_TYPE == CDS_OS_NET_BSD
#    include <cds/os/posix/syserror.h>
#else
/************************************************************************/
/* Other OSes
/************************************************************************/
#    error Unknown operating system. Compilation aborted.
#endif

#endif    // #ifndef __CDS_OS_SYSERROR_H
