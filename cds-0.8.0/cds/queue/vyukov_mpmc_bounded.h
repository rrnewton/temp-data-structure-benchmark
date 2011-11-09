/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_QUEUE_VYUKOV_MPMC_BOUNDED_H
#define __CDS_QUEUE_VYUKOV_MPMC_BOUNDED_H

//#include <cds/queue/details/queue_base.h>
#include <cds/opt/options.h>
#include <cds/opt/buffer.h>
#include <cds/opt/value_cleaner.h>
#include <cds/atomic.h>
#include <cds/user_setup/cache_line.h>
#include <cds/details/aligned_type.h>
#include <cds/ref.h>
#include <cds/details/allocator.h>

namespace cds { namespace queue {

    /// Vyukov's MPMC bounded queue
    /**
        This algorithm is developed by Dmitry Vyukov (see http://www.1024cores.net)
        It's multi-producer multi-consumer (MPMC), array-based, fails on overflow, does not require GC, w/o priorities, causal FIFO, 
        blocking producers and consumers queue. The algorithm is pretty simple and fast. It's not lockfree in the official meaning, 
        just implemented by means of atomic RMW operations w/o mutexes. 

        The cost of enqueue/dequeue is 1 CAS per operation. 
        No dynamic memory allocation/management during operation. Producers and consumers are separated from each other (as in the two-lock queue), 
        i.e. do not touch the same data while queue is not empty. 

        \par Source:
            http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue

        \par Template parameters
            \li \p T - type stored in queue. Must be assignable.
            \li \p OPTIONS - queue's options

        Options \p OPTIONS are:
            \li cds::opt::buffer - buffer to store items. Mandatory option, see option description for full list of possible types.
            \li cds::opt::value_cleaner - a functor to clean item dequeued. Default value is \ref opt::v::empty_disposer

        \par License
            Simplified BSD license by Dmitry Vyukov (http://www.1024cores.net/site/1024cores/home/code-license)

        \par Example
        \code
        #include <cds/queue/vyukov_mpmc_bounded.h>

        // // Queue with 1024 item static buffer
        cds::queue::vyukov_mpmc_bounded<int, cds::opt::buffer< cds::opt::v::static_buffer<int, 1024> > > myQueue ;

        \endcode
    */
#ifdef CDS_COMPILER_SUPPORTS_VARIADIC_TEMPLATE
    template <typename T, typename... OPTIONS>
#else
    template <typename T
        , typename O1= opt::none
        , typename O2= opt::none
    >
#endif
    class vyukov_mpmc_bounded 
    {
    public:
        typedef T value_type  ;   ///< Value type stored in queue

    private:
        //@cond
        struct default_options 
        {
            typedef opt::v::empty_cleaner   value_cleaner ;
        };

#ifdef CDS_COMPILER_SUPPORTS_VARIADIC_TEMPLATE
        typedef typename opt::make_options<default_options, OPTIONS...>::type   options ;
#else
        typedef typename opt::make_options<default_options, O1, O2>::type   options ;
#endif
        typedef typename options::value_cleaner  value_cleaner   ;
        //@endcond
        
    protected:
        //@cond
        struct cell_type
        {
            cds::atomic<size_t>     sequence;
            value_type              data;
        };

        struct trivial_copy {
            void operator()( value_type& dest, value_type const& src )
            {
                dest = src  ;
            }
        };

        // Buffer storing queue items
        typedef typename options::buffer::template rebind<cell_type>::other     buffer  ;

        //@endcond

    protected:
        //@cond
        typename details::aligned_type<buffer, c_nCacheLineSize>::type  m_buffer    ;
        size_t const    m_nBufferMask   ;
        typename details::aligned_type<atomic<size_t>, c_nCacheLineSize>::type  m_posEnqueue    ;
        typename details::aligned_type<atomic<size_t>, c_nCacheLineSize>::type  m_posDequeue    ;
        //@endcond

    public:
        /// Constructs the queue of capacity \p nCapacity
        /**
            For cds::opt::v::static_buffer the \p nCapacity parameter is ignored.
        */
        vyukov_mpmc_bounded(
            size_t nCapacity = 0
            )
            : m_buffer( nCapacity )
            , m_nBufferMask( m_buffer.capacity() - 1 )
        {
            nCapacity = m_buffer.capacity()  ;

            // Buffer capacity must be power of 2
            assert( nCapacity >= 2 && (nCapacity & (nCapacity - 1)) == 0 ) ;

            for (size_t i = 0; i != nCapacity; i += 1)
                m_buffer[i].sequence.template store<membar_relaxed>(i)   ;

            m_posEnqueue.store<membar_relaxed>(0)   ;
            m_posDequeue.store<membar_relaxed>(0)   ;
        }

        ~vyukov_mpmc_bounded()
        {
            clear() ;
        }

        /// Enqueues \p data to queue using copy functor
        /**
            \p FUNC is a functor called to copy value \p data of type \p SOURCE
            which may be differ from type \p T stored in the queue.
            The functor's interface is:
            \code
                struct myFunctor {
                    void operator()(T& dest, SOURCE const& data)
                    {
                        // // Code to copy \p data to \p dest
                        dest = data ;
                    }
                };
            \endcode
            <b>Requirements</b> The functor \p FUNC should not throw any exception.
        */
        template <typename SOURCE, typename FUNC>
        bool enqueue(SOURCE const& data, FUNC func)
        {
            cell_type* cell;
            size_t pos = m_posEnqueue.load<membar_relaxed>();

            for (;;)
            {
                cell = &m_buffer[pos & m_nBufferMask];
                size_t seq = cell->sequence.template load<membar_acquire>();

                intptr_t dif = (intptr_t)seq - (intptr_t)pos;

                if (dif == 0)
                {
                    if ( m_posEnqueue.cas<membar_relaxed>(pos, pos + 1) )
                        break;
                }
                else if (dif < 0)
                    return false;
                else
                    pos = m_posEnqueue.load<membar_relaxed>();
            }

            unref(func)( cell->data, data ) ;
            cell->sequence.template store<membar_release>(pos + 1);

            return true;
        }

        /// Enqueues \p data to queue
        bool enqueue(value_type const& data )
        {
            return enqueue( data, trivial_copy() )  ;
        }

        /// Dequeues an item from queue
        /**
            \p FUNC is a functor called to copy dequeued value of type \p T to \p dest of type \p DEST.
            The functor's interface is:
            \code
            struct myFunctor {
            void operator()(DEST& dest, T const& data)
            {
                // // Code to copy \p data to \p dest
                dest = data ;
            }
            };
            \endcode
            <b>Requirements</b> The functor \p FUNC should not throw any exception.
        */
        template <typename DEST, typename FUNC>
        bool dequeue( DEST& data, FUNC func )
        {
            cell_type * cell;
            size_t pos = m_posDequeue.load<membar_relaxed>();

            for (;;)
            {
                cell = &m_buffer[pos & m_nBufferMask];
                size_t seq = cell->sequence.template load<membar_acquire>();
                intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);

                if (dif == 0)
                {
                    if ( m_posDequeue.cas<membar_relaxed>(pos, pos + 1))
                        break;
                }
                else if (dif < 0)
                    return false;
                else
                    pos = m_posDequeue.load<membar_relaxed>();
            }

            unref(func)( data, cell->data ) ;
            value_cleaner()( cell->data )   ;
            cell->sequence.template store<membar_release>( pos + m_nBufferMask + 1 );

            return true;
        }

        /// Dequeues an item from queue to \p data
        /**
            If queue is empty, returns \p false, \p data is unchanged.
        */
        bool dequeue(value_type & data )
        {
            return dequeue( data, trivial_copy() )  ;
        }

        /// Synonym of \ref enqueue
        bool push(value_type const& data)
        {
            return enqueue(data)    ;
        }

        /// Synonym of \ref dequeue
        bool pop(value_type& data)
        {
            return dequeue(data)    ;
        }

        /// Clears the queue
        void clear()
        {
            value_type v        ;
            while ( pop(v) )    ;
        }

    };

}}  // namespace cds::queue



#endif // #ifndef __CDS_QUEUE_VYUKOV_MPMC_BOUNDED_H
