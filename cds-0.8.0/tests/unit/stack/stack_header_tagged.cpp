/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include <cds/stack/stack_tagged.h>
#include "stack/stack_test_header.h"

#ifdef CDS_DWORD_CAS_SUPPORTED

namespace stack {
    void StackTestHeader::Stack_tagged()
    {
        test< cds::stack::Stack< cds::gc::tagged_gc, int > >() ;
    }
}

#endif  // #ifdef CDS_DWORD_CAS_SUPPORTED
