/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_HZP_HP_OBJECT_H
#define __CDS_GC_HZP_HP_OBJECT_H

#include <cds/gc/hzp/hzp.h>
#include <cds/threading/model.h>

#include <cds/details/allocator.h>
namespace cds { namespace gc {
    namespace hzp {

        ///  The base for all containers based on the Hazard Pointers GC
        /**
            Template parameters:
            \li \p NODE            Container's node type
            \li \p ALLOCATOR    Node allocator. Default is \ref CDS_DEFAULT_ALLOCATOR
        */
        template <typename NODE, class ALLOCATOR = CDS_DEFAULT_ALLOCATOR >
        class Container
        {
        public:
            typedef hzp_gc    gc_schema            ;    ///< type of garbage collector

        public:
            typedef NODE                                                Node            ;    ///< container's node type
            typedef typename ALLOCATOR::template rebind<Node>::other    TAllocator        ;    ///< type of node allocator
            typedef cds::details::Allocator< Node, TAllocator >            node_allocator    ;   ///< wrapper for node allocator

        protected:
            typedef gc::hzp::ThreadGC                                    thread_gc        ;    ///< Generic Hazard Pointer Manager
            node_allocator                                                m_NodeAllocator    ;    ///< Node allocator

        public:
            /// Default functor to delete retired node
            /**
                The functor for deleting retired node relates to the type_traits of a container. The container should
                define its own functor if it may contain the nodes of various type.

                When a node is being retired the Hazard Pointer GC places to GC's internal buffer (of type details::retired_vector)
                a pair <the pointer to the node, the pointer to node's freeing function>.
                Then, in GarbageCollector::Scan phase, the function stored is called to delete the node physically
                if the deletion is safe (i.e. the node is not guarded of any Hazard Pointer of any thread).

                Do not use the functor directly.
            */
            typedef cds::details::deferral_deleter<Node, ALLOCATOR>     deferral_node_deleter ;

        protected:
            /// Returns Hazard Pointer manager object for current thread
            static thread_gc&    getGC()
            {
                return cds::threading::getGC<gc_schema>() ;
            }

            /// allocates node and initializes it by default ctor
            Node * allocNode()
            {
                return m_NodeAllocator.New()    ;
            }

            /// allocates node and initializes it by node's ctor with parameters
            template <typename T>
            Node * allocNode( const T& data )
            {
                return m_NodeAllocator.New( data )    ;
            }

            /// allocates node and initializes it by node's ctor with two parameters
            template <typename K, typename V>
            Node * allocNode( const K& k, const V& v )
            {
                return m_NodeAllocator.New( k, v )    ;
            }

            /// frees node (call dtor and free memory)
            void freeNode( Node * pNode )
            {
                m_NodeAllocator.Delete( pNode )    ;
            }
        };
    }    // namespace hzp
}} // namespace cds::gc

#endif // #ifndef __CDS_GC_HZP_HP_OBJECT_H
