/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_COMPILER_VC_COMPILER_BARRIERS_H
#define __CDS_COMPILER_VC_COMPILER_BARRIERS_H

#include <intrin.h>

#pragma intrinsic(_ReadWriteBarrier)
#pragma intrinsic(_ReadBarrier)
#pragma intrinsic(_WriteBarrier)

#define CDS_COMPILER_RW_BARRIER  _ReadWriteBarrier()
#define CDS_COMPILER_R_BARRIER   _ReadBarrier()
#define CDS_COMPILER_W_BARRIER   _WriteBarrier()

#endif  // #ifndef __CDS_COMPILER_VC_COMPILER_BARRIERS_H
