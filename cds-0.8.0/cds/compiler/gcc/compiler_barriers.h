/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_COMPILER_GCC_COMPILER_BARRIERS_H
#define __CDS_COMPILER_GCC_COMPILER_BARRIERS_H

#define CDS_COMPILER_RW_BARRIER  __asm__ __volatile__ ( "" ::: "memory" )
#define CDS_COMPILER_R_BARRIER   CDS_COMPILER_RW_BARRIER
#define CDS_COMPILER_W_BARRIER   CDS_COMPILER_RW_BARRIER

#endif  // #ifndef __CDS_COMPILER_GCC_COMPILER_BARRIERS_H
