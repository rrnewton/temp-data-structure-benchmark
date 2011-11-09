/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include <cds/queue/rwqueue.h>

#include "queue/queue_test_header.h"

namespace queue {
    void Queue_TestHeader::RWQueue_()
    {
        testNoItemCounter<cds::queue::RWQueue<int, cds::SpinLock> >()    ;
    }

    void Queue_TestHeader::RWQueue_Counted()
    {
        testWithItemCounter< cds::queue::RWQueue<int, cds::SpinLock, QueueCountedTraits> >() ;
    }
}
