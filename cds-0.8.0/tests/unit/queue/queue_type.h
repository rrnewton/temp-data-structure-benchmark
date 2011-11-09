/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __UNIT_QUEUE_TYPES_H
#define __UNIT_QUEUE_TYPES_H

#include <cds/queue/csqueue.h>

#include <cds/queue/moir_queue_hzp.h>
#include <cds/queue/moir_queue_hrc.h>
#include <cds/queue/moir_queue_ptb.h>
#ifdef CDS_DWORD_CAS_SUPPORTED
#   include <cds/queue/moir_queue_tagged.h>
#endif

#include <cds/queue/msqueue_hzp.h>
#include <cds/queue/msqueue_hrc.h>
#include <cds/queue/msqueue_ptb.h>
#ifdef CDS_DWORD_CAS_SUPPORTED
#   include <cds/queue/msqueue_tagged.h>
#endif

#include <cds/queue/lmsqueue_hzp.h>
#include <cds/queue/lmsqueue_ptb.h>

#include <cds/queue/rwqueue.h>
#include <cds/queue/tz_cyclic_queue.h>

#include "queue/std_queue.h"
#include "lock/win32_lock.h"
#include <boost/thread/mutex.hpp>

namespace queue {

    template <typename T>
    class HASQueue {
        struct node
        {
            T data              ;
            node * volatile next;
            explicit node()
                : next(NULL)
            {}
        } ;

        node * _begin  ;
        node * volatile _end    ;

        mutable cds::SpinLock   _cs ;
        cds::atomic32_t _size   ;

    public:
        typedef T       value_type  ;

    public:
        HASQueue()
        {
            _begin = new node();
            _end = new node();
            _end->next = _begin;
            _begin->next = _end;
            _size = 0;
        }

        ~HASQueue()
        {
            clear();
            delete _begin;
            delete _end;
        }

        void clear()
        {
            T   it   ;
            while ( pop(it) ) {}

            assert( empty() )   ;
        }


        bool enqueue(const T& c)
        {
            node * n = new node();
            n->next = _begin;

            node * iterator = NULL;

            cds::backoff::yield bkoff   ;
            do
            {
                if (iterator)
                    bkoff() ;

                iterator = cds::atomics::load<cds::membar_acquire>( &_end );

            } while ( iterator != cds::atomics::vcas<cds::membar_release>( &_end, iterator, n) );

            iterator->data = c  ;

            cds::atomics::store<cds::membar_release>( &iterator->next, n);
            cds::atomics::inc<cds::membar_release>( &_size );

            return true ;
        }

        bool push( const T& c )
        {
            return enqueue( c ) ;
        }

        bool empty() const
        {
            cds::lock::Auto< cds::SpinLock >    autoLock( _cs ) ;
            return _begin == cds::atomics::load<cds::membar_acquire>( &(_begin->next ) )->next ;
        }

        bool dequeue(T& c)
        {
            node * iterator = NULL;
            cds::backoff::yield bkoff   ;

            while ( true )
            {
                cds::lock::Auto< cds::SpinLock >    autoLock( _cs ) ;

                if (_begin == (iterator = cds::atomics::load<cds::membar_acquire>( &(_begin->next ) ))->next )
                    return false;

                if ( iterator == cds::atomics::vcas<cds::membar_release>( &_begin->next, iterator, _begin->next->next) )
                    break;

                bkoff() ;
            }

            assert(iterator);

            cds::atomics::dec<cds::membar_release>( &_size )  ;
            c = iterator->data;

            delete iterator;

            return true;
        }

        bool pop( T& c )
        {
            return dequeue(c)   ;
        }
    };

    struct CountedTraits: public cds::queue::traits {
        typedef cds::atomics::item_counter<cds::membar_release, cds::membar_acquire>        item_counter_type    ;
    };

    template <typename VALUE>
    struct Types {
      typedef cds::queue::CircularFifo<VALUE, 64000>             CSQueue        ;
        typedef cds::queue::MoirQueue<cds::gc::hzp_gc, VALUE>           MoirQueue_HP        ;
        typedef cds::queue::MoirQueue<cds::gc::hrc_gc, VALUE>           MoirQueue_HRC       ;
        typedef cds::queue::MoirQueue<cds::gc::ptb_gc, VALUE>           MoirQueue_PTB       ;
#ifdef CDS_DWORD_CAS_SUPPORTED
        typedef cds::queue::MoirQueue<cds::gc::tagged_gc, VALUE>        MoirQueue_Tagged    ;
#endif
        typedef cds::queue::MSQueue<cds::gc::hrc_gc, VALUE>             MSQueue_HRC         ;
        typedef cds::queue::MSQueue<cds::gc::hzp_gc, VALUE>             MSQueue_HP          ;
        typedef cds::queue::MSQueue<cds::gc::ptb_gc, VALUE>             MSQueue_PTB         ;
#ifdef CDS_DWORD_CAS_SUPPORTED
        typedef cds::queue::MSQueue<cds::gc::tagged_gc, VALUE>          MSQueue_Tagged      ;
#endif
        typedef cds::queue::LMSQueue<cds::gc::hzp_gc, VALUE>            LMSQueue_HP         ;
        typedef cds::queue::LMSQueue<cds::gc::ptb_gc, VALUE>            LMSQueue_PTB        ;

        typedef cds::queue::TZCyclicQueue< VALUE >                        TZCyclicQueue        ;

        typedef cds::queue::MoirQueue<cds::gc::hzp_gc, VALUE, CountedTraits>           MoirQueue_HP_Counted        ;
        typedef cds::queue::MoirQueue<cds::gc::hrc_gc, VALUE, CountedTraits>           MoirQueue_HRC_Counted       ;
        typedef cds::queue::MoirQueue<cds::gc::hrc_gc, VALUE, CountedTraits>           MoirQueue_PTB_Counted       ;
#ifdef CDS_DWORD_CAS_SUPPORTED
        typedef cds::queue::MoirQueue<cds::gc::tagged_gc, VALUE, CountedTraits>        MoirQueue_Tagged_Counted    ;
#endif
        typedef cds::queue::MSQueue<cds::gc::hrc_gc, VALUE, CountedTraits>             MSQueue_HRC_Counted         ;
        typedef cds::queue::MSQueue<cds::gc::hzp_gc, VALUE, CountedTraits>             MSQueue_HP_Counted          ;
        typedef cds::queue::MSQueue<cds::gc::ptb_gc, VALUE, CountedTraits>             MSQueue_PTB_Counted         ;
#ifdef CDS_DWORD_CAS_SUPPORTED
        typedef cds::queue::MSQueue<cds::gc::tagged_gc, VALUE, CountedTraits>          MSQueue_Tagged_Counted      ;
#endif

        typedef cds::queue::LMSQueue<cds::gc::hzp_gc, VALUE, CountedTraits>            LMSQueue_HP_Counted          ;
        typedef cds::queue::LMSQueue<cds::gc::ptb_gc, VALUE, CountedTraits>            LMSQueue_PTB_Counted         ;

        typedef cds::queue::TZCyclicQueue< VALUE, CountedTraits >                       TZCyclicQueue_Counted       ;

        typedef cds::queue::RWQueue<VALUE, cds::lock::Spin>                             RWQueue_Spinlock           ;
        typedef cds::queue::RWQueue<VALUE, cds::lock::Spin, CountedTraits>              RWQueue_Spinlock_Counted   ;

        typedef StdQueue_deque<VALUE>               StdQueue_deque_Spinlock     ;
        typedef StdQueue_list<VALUE>                StdQueue_list_Spinlock      ;
        typedef StdQueue_deque<VALUE, boost::mutex> StdQueue_deque_BoostMutex   ;
        typedef StdQueue_list<VALUE, boost::mutex>  StdQueue_list_BoostMutex    ;
#ifdef UNIT_LOCK_WIN_CS
        typedef StdQueue_deque<VALUE, lock::win::CS>    StdQueue_deque_WinCS    ;
        typedef StdQueue_list<VALUE, lock::win::CS>     StdQueue_list_WinCS     ;
        typedef StdQueue_deque<VALUE, lock::win::Mutex> StdQueue_deque_WinMutex    ;
        typedef StdQueue_list<VALUE, lock::win::Mutex>  StdQueue_list_WinMutex     ;
#endif

        typedef HASQueue<VALUE>                         HASQueue_Spinlock           ;
    };
}

#endif // #ifndef __UNIT_QUEUE_TYPES_H
