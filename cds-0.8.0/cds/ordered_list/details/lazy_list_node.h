/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ORDERED_LIST_LAZY_LIST_NODE_H
#define __CDS_ORDERED_LIST_LAZY_LIST_NODE_H

/*
    Editions:

0.7.0   2010.09.16 Maxim.Khiszinsky     Common implementation for lazy list node

*/

#include <cds/details/defs.h>
#include <cds/user_setup/allocator.h>   // CDS_DEFAULT_ALLOCATOR
#include <cds/concept.h>
#include <cds/details/markptr.h>
#include <cds/details/allocator.h>
#include <cds/lock/spinlock.h>

//@cond
namespace cds { namespace ordered_list { namespace details { namespace lazy_list {
    /// Lazy list node
    /**
        Template parameters:
            \li \p GC        Memory reclamation schema
            \li \p KEY        type of key stored in list
            \li \p VALUE    type of value stored in list
            \li \p LOCK        lock primitive
    */
    template <typename GC, typename KEY, typename VALUE, typename LOCK = SpinLock, typename ALLOCATOR = CDS_DEFAULT_ALLOCATOR >
    class Node
    {
    public:
        typedef GC      gc_type     ;   ///< Garbage collector type
        typedef KEY     key_type    ;   ///< Key type
        typedef VALUE   value_type  ;   ///< Value type
        typedef cds::details::Allocator< Node, ALLOCATOR>   node_allocator  ;   ///< Node allocator type
        typedef LOCK                                        lock_type       ;    ///< node locker type

        /// Rebind key/value type
        template <typename KEY2, typename VALUE2, typename LOCK2=lock_type>
        struct rebind {
            typedef Node<gc_type, KEY2, VALUE2, LOCK2, ALLOCATOR>    other ; ///< Rebinding result
        };

    public:
        atomic<Node *>  m_pNext     ;    ///< Pointer to next node in list
    private:
        lock_type	    m_Lock      ;    ///< Node locker
        atomic<bool>    m_bMarked   ;    ///< Logical deletion mark

    public:
        // Caveats: order of fields is important!
        // The split-list implementation uses different node types, dummy node contains only the hash and no key and data field
        // Therefore, key and data must be last fields in the node
        const key_type  m_key        ;    ///< Node's key
        value_type      m_data       ;    ///< Node's data

    public:
        /// Default ctor
        Node()
            : m_key()
            , m_pNext( NULL )
            , m_bMarked( false )
        {}

        /// Key ctor
        Node(
            const key_type& key     ///< node's key
            )
            : m_key(key)
            , m_pNext( NULL )
            , m_bMarked( false )
        {}

        /// Key and value ctor
        Node(
            const key_type& key,    ///< node's key
            const value_type& data  ///< node's value
            )
            : m_key(key)
            , m_pNext( NULL )
            , m_bMarked( false )
            , m_data( data )
        {}

        /// Locks the node
        void    lock()                { m_Lock.lock();    }
        /// Unlocks the node
        void    unlock()            { m_Lock.unlock();    }

        /// Checks if node is marked (logically deleted)
        template <typename ORDER>
        bool    isMarked() const
        {
            return m_bMarked.load<ORDER>() ;
        }
        bool    isMarked() const
        {
            return isMarked<membar_acquire>() ;
        }

        /// Mark node as logically deleted ones
        template <typename ORDER>
        void    mark()
        {
            m_bMarked.store<ORDER>( true ) ;
        }
        void    mark()
        {
            mark<membar_release>()    ;
        }

        template <typename ORDER>
        Node * next() const
        {
            return m_pNext.template load<ORDER>()   ;
        }

        Node * next() const
        {
            return next<membar_acquire>()   ;
        }

        /// Returns node's key
        const key_type& key()    { return m_key ; }

        /// Returns node's data
        value_type&     value()    { return m_data;    }
    };
}}}}    // namespace cds::ordered_list::details::lazy_list
//@endcond

#endif  // #ifndef __CDS_ORDERED_LIST_LAZY_LIST_NODE_H