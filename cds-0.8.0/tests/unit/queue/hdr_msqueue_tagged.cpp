/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include <cds/queue/msqueue_tagged.h>
#include "queue/queue_test_header.h"

namespace queue {
    void Queue_TestHeader::MSQueue_tagged()
    {
#ifdef CDS_DWORD_CAS_SUPPORTED
        testNoItemCounter<cds::queue::MSQueue<cds::gc::tagged_gc, int> >()    ;
#endif
    }

    void Queue_TestHeader::MSQueue_tagged_Counted()
    {
#ifdef CDS_DWORD_CAS_SUPPORTED
        testWithItemCounter< cds::queue::MSQueue<cds::gc::tagged_gc, int, QueueCountedTraits > >() ;
#endif
    }
}

