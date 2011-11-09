/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_VERSION_H
#define __CDS_VERSION_H

// cds library version

#define CDS_VERSION        0x000800

#define CDS_VERSION_MAJOR  ((CDS_VERSION & 0xFF0000)>> 16)
#define CDS_VERSION_MINOR  ((CDS_VERSION & 0x00FF00) >> 8)
#define CDS_VERSION_PATCH  (CDS_VERSION & 0x0000FF)

// CDS_VERSION == CDS_VERSION_MAJOR '.' CDS_VERSION_MINOR '.' CDS_VERSION_PATCH
#define CDS_VERSION_STRING        "0.8.0"

#endif  // #ifndef __CDS_VERSION_H

