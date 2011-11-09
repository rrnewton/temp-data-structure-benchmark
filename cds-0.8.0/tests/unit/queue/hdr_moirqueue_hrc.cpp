/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include <cds/queue/moir_queue_hrc.h>

#include "queue/queue_test_header.h"

//
// TestCase class
//
namespace queue {
    void Queue_TestHeader::MoirQueue_HRC()
    {
        testNoItemCounter<cds::queue::MoirQueue<cds::gc::hrc_gc, int> >()    ;
    }

    void Queue_TestHeader::MoirQueue_HRC_Counted()
    {
        testWithItemCounter< cds::queue::MoirQueue<cds::gc::hrc_gc, int, QueueCountedTraits > >() ;
    }

}
