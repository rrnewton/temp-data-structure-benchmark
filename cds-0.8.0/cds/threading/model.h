/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_THREADING_MODEL_H
#define __CDS_THREADING_MODEL_H

#include <cds/threading/details/_common.h>
#include <cds/user_setup/threading.h>
#include <cds/threading/details/auto_detect.h>

namespace cds { namespace threading {

    /// Returns thread specific data of \p GC garbage collector
    template <class GC> typename GC::thread_gc&  getGC()  ;

    /// Returns gc::hzp::ThreadGC object of current thread
    /**
        The object returned may be uninitialized if you did not call attachThread in the beginning of thread execution
        or do not use gc::hzp::GarbageCollector.
        To initialize global gc::hzp::GarbageCollector object you must call cds::gc::hzp::GarbageCollector::Construct()
        in the beginning of your application
    */
    template <>
    inline gc::hzp_gc::thread_gc&   getGC<gc::hzp_gc>()
    {
        return Manager::getHZPGC()  ;
    }

    /// Returns gc::hrc::ThreadGC object of current thread
    /**
        The object returned may be uninitialized if you did not call attachThread in the beginning of thread execution
        or do not use gc::hrc::GarbageCollector.
        To initialize global gc::hrc::GarbageCollector object you must call cds::gc::hrc::GarbageCollector::Construct()
        in the beginning of your application
    */
    template <>
    inline gc::hrc_gc::thread_gc&   getGC<gc::hrc_gc>()
    {
        return Manager::getHRCGC()  ;
    }

    /// Returns gc::ptb::ThreadGC object of current thread
    /**
        The object returned may be uninitialized if you did not call attachThread in the beginning of thread execution
        or do not use gc::ptb::GarbageCollector.
        To initialize global gc::ptb::GarbageCollector object you must call cds::gc::ptb::GarbageCollector::Construct()
        in the beginning of your application
    */
    template <>
    inline gc::ptb_gc::thread_gc&   getGC<gc::ptb_gc>()
    {
        return Manager::getPTBGC()  ;
    }

}} // namespace cds::threading

#endif // #ifndef __CDS_THREADING_MODEL_H
