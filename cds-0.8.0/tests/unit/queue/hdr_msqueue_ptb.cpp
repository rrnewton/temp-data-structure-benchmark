/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include <cds/queue/msqueue_ptb.h>

#include "queue/queue_test_header.h"

namespace queue {
    void Queue_TestHeader::MSQueue_PTB()
    {
        testNoItemCounter<cds::queue::MSQueue<cds::gc::ptb_gc, int> >()    ;
    }

    void Queue_TestHeader::MSQueue_PTB_Counted()
    {
        testWithItemCounter< cds::queue::MSQueue<cds::gc::ptb_gc, int, QueueCountedTraits > >() ;
    }
}
