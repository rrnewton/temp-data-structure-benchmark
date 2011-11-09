/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include <cds/queue/moir_queue_tagged.h>
#include "queue/queue_test_header.h"

namespace queue {

    void Queue_TestHeader::MoirQueue_tagged()
    {
#ifdef CDS_DWORD_CAS_SUPPORTED
        testNoItemCounter<cds::queue::MoirQueue<cds::gc::tagged_gc, int> >()    ;
#endif
    }

    void Queue_TestHeader::MoirQueue_tagged_Counted()
    {
#ifdef CDS_DWORD_CAS_SUPPORTED
        testWithItemCounter< cds::queue::MoirQueue<cds::gc::tagged_gc, int, QueueCountedTraits > >() ;
#endif
    }
}
