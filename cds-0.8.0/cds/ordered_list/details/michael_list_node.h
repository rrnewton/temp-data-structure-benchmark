/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ORDERED_LIST_MICHAEL_LIST_NODE_H
#define __CDS_ORDERED_LIST_MICHAEL_LIST_NODE_H

/*
    Editions:

0.7.0   2010.09.16 Maxim.Khiszinsky     Common implementation for HP-like GC (hzp_gc, ptd_gc) is added

*/

#include <cds/details/defs.h>
#include <cds/user_setup/allocator.h>   // CDS_DEFAULT_ALLOCATOR
#include <cds/concept.h>
#include <cds/details/markptr.h>
#include <cds/details/allocator.h>

//@cond
namespace cds { namespace ordered_list {
    namespace details {

        /// Michael's ordered list implementation details
        namespace michael_list {

            /// Michael's list node
            /**
                This class has the specialization for each type of \p GC template parameter

                Template parameters:
                    \li \p GC  Garbage collector (see cds::gc namespace)
                    \li \p KEY Key type
                    \li \p VALUE Value type
                    \li \p ALLOCATOR Memory allocator; default is \ref CDS_DEFAULT_ALLOCATOR
            */
            template <class GC, typename KEY, typename VALUE, typename ALLOCATOR=CDS_DEFAULT_ALLOCATOR>
            class Node
            {
            public:
                typedef GC      gc_type     ;   ///< Garbage collector type
                typedef KEY     key_type    ;   ///< Key type
                typedef VALUE   value_type  ;   ///< Value type
                typedef cds::details::Allocator< Node, ALLOCATOR>   node_allocator  ;   ///< Node allocator type
                typedef    cds::details::marked_ptr< Node, 1 >            marked_node        ;    ///< Marked node type; the least significant bit is used as deletion mark

                /// Rebind key/value type to \p KEY2 and \p VALUE2
                template <typename KEY2, typename VALUE2>
                struct rebind {
                    typedef Node<gc_type, KEY2, VALUE2, ALLOCATOR>   other   ;   ///< Rebinding result
                };

                // Caveats: order of fields is important!
                // The split-list implementation uses different node types, dummy node contains only the hash and no key and data field
                // Therefore, key and data must be last fields in the node
                marked_node        m_pNext ;    ///< Pointer to next node in the list
                const key_type  m_key   ;   ///< Node's key
                value_type      m_value ;   ///< Node's value

            public:
                /// Default ctor
                Node() {}
                /// Node's key initializer
                Node( const key_type& key )
                    : m_key( key )
                {}
                /// Node's key and value initializer
                Node( const key_type& key, const value_type& val )
                    : m_key( key )
                    , m_value( val )
                {}

                /// Returns node's key
                const key_type& key()    { return m_key ; }

                /// Returns node's data
                value_type&     value()    { return m_value;    }

                /// Checks if the node is marked as logically deleted one
                bool isMarked() const
                {
                    return m_pNext.isMarked() ;
                }

                /// Returns next node with deletion mark cleared
                Node *  next()
                {
                    return m_pNext.ptr()    ;
                }
            };


    }}    // namespace details::michael_list
}} // namespace cds::ordered_list
//@endcond

#endif // #ifndef __CDS_ORDERED_LIST_MICHAEL_LIST_NODE_H
