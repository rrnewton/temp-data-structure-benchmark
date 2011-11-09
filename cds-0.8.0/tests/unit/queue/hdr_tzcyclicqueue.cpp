/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include <cds/queue/tz_cyclic_queue.h>

#include "queue/queue_test_header.h"

namespace queue {
    void Queue_TestHeader::TZCyclicQueue_()
    {
        testNoItemCounter<cds::queue::TZCyclicQueue<int> >()    ;
    }

    void Queue_TestHeader::TZCyclicQueue_Counted()
    {
        testWithItemCounter< cds::queue::TZCyclicQueue<int, QueueCountedTraits > >() ;
    }
}
