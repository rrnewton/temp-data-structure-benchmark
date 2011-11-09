/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_TAGGED_DETAILS_FREE_LIST_H
#define __CDS_GC_TAGGED_DETAILS_FREE_LIST_H

/*
    Editions:
        2007.03.03  Maxim.Khiszinsky    Created
        2008.10.01    Maxim.Khiszinsky    Refactoring
*/

#include <cds/gc/tagged/container_node.h>
#include <cds/backoff_strategy.h>
#include <cds/details/aligned_allocator.h>

#ifdef CDS_DWORD_CAS_SUPPORTED

namespace cds { namespace gc { namespace tagged {

    /// FreeList of Tagged pointer reclamation schema
    /**
        FreeList is list of free nodes. To prevent ABA-problem tagged reclamation schema implies
        that the node is never freed physically. Instead of deallocation the node is placed in special container
        named free-list. So free-list may be considered as an allocator for data of type \p NODE.
        The implementation of free-list is canonical stack.

        \par Template parameters:
            \li \p NODE - type of node
            \li \p BACKOFF - back-off cshema. The default is backoff::empty
            \li \p ALLOCATOR - aligned memory allocator. The default class is defined by \ref CDS_DEFAULT_ALIGNED_ALLOCATOR macro

        \par
        The main problem of tagged reclamation schema is destroying a node. The node of tagged containers
        is the structure like this:
        \code
            template <typename T>
            struct Node: public cds::gc::tagged::ContainerNode
            {
                tagged_type< Node<T>* >     m_next    ;
                T                        m_data    ;
            }
        \endcode
        cds::gc::tagged::ContainerNode provides linking unused items to free-list.

        When the node is excluding from container it must be placed into appropriate free-list. However,
        on the destroying time the destructor of type NODE must not be called because of the access
        to deleted node can be possible (the free-list must be a type-stable container). So the destructor NODE::~NODE
        of data may be safely called in FreeList::pop method \b before returning the node cached in free-list.
        This is achieved by calling \p destroy_data member function of type \p NODE. See \ref ContainerNode
        class for further explanation of \p NODE requirements.
    */
    template <typename NODE,
        class BACKOFF = backoff::empty
        ,class ALLOCATOR = CDS_DEFAULT_ALIGNED_ALLOCATOR
    >
    class FreeList
    {
        //@cond
        typedef cds::details::AlignedAllocator< NODE, ALLOCATOR > aligned_allocator    ;
        //@endcond

    public:
        typedef NODE        node_type          ;   ///< type of node
        typedef BACKOFF     backoff_strategy    ;   ///< Back-off strategy
        typedef typename    aligned_allocator::allocator_type      allocator_type   ; ///< Aligned allocator

        /// To support rebind to type \p OTHER_NODE, and back-off strategy \p OTHER_BACKOFF, and allocator \p OTHER_ALLOC
        template <typename OTHER_NODE, typename OTHER_BACKOFF = BACKOFF, typename OTHER_ALLOC = ALLOCATOR >
        struct rebind {
            typedef FreeList< OTHER_NODE, OTHER_BACKOFF, OTHER_ALLOC >    other ;   ///< Rebinding result
        };

    private:

        //@cond
        typedef ContainerNode               container_node  ;
        //@endcond

        typedef tagged_type< node_type * >  tagged_node    ;    ///< Tagged node

        tagged_node         m_Top   ;   ///< top of the stack of free nodes
        aligned_allocator   m_Alloc ;   ///< aligned allocator

    private:
        /// Places the node \p node to internal stack
        void push( node_type * pNode )
        {
            assert( cds::details::is_aligned<CDS_TAGGED_ALIGNMENT>( pNode ) )   ;

            BACKOFF backoff    ;
            tagged_node t    ;
            while ( true ) {
                t = atomics::load<membar_acquire>( &m_Top )    ;
                static_cast< container_node *>( pNode )->m_pNextFree.set<membar_relaxed>( static_cast<container_node *>( t.data() ) )   ;
                if ( cas_tagged<membar_release>( m_Top, t, pNode ))
                    break        ;
                backoff();
            }
        }

        /** Pops cached node from internal stack. If free-list is not empty this method calls \p destroy_data of
            popped node before return.
        */
        node_type * pop()
        {
            tagged_node t    ;
            BACKOFF backoff    ;
            while ( true ) {
                t = atomics::load<membar_acquire>( &m_Top )    ;
                if ( t.data() == NULL )
                    return NULL            ;
                if ( cas_tagged<membar_release>( m_Top, t, static_cast< node_type * >( static_cast<container_node *>( t.data() )->m_pNextFree.data() ) ))
                    break    ;
                backoff();
            }
            assert( t.m_data != NULL )    ;
            node_type * p = static_cast< node_type *>( t.data() )   ;
            assert( cds::details::is_aligned<CDS_TAGGED_ALIGNMENT>( p ) )   ;
            p->destroy_data()   ;
            static_cast<container_node *>(p)->m_pNextFree.ref() = NULL      ;
            return p            ;
        }

        /// Clears internal stack. This method physically deletes all cached nodes.
        void clear()
        {
            tagged_node t    ;
            do {
                t = atomics::load<membar_acquire>( &m_Top ) ;
                if ( t.data() == NULL )
                    break    ;
            } while ( !cas_tagged<membar_release>( m_Top, t, (node_type *) NULL ) )    ;

            // t contains the list's head and it is private for current thread
            node_type * p = t.data()    ;
            while ( p ) {
                node_type * pNext = static_cast<node_type *>( static_cast<container_node *>(p)->m_pNextFree.data() )   ;
                m_Alloc.Delete( p )    ;
                p = pNext    ;
            }
        }

    public:
        FreeList()
        {
            CDS_STATIC_ASSERT( sizeof(node_type) >= sizeof(void *) )    ;
        }

        ~FreeList()
        {
            clear()    ;
        }

        /// Get new node from free-list. If free-list is empty the new node is allocated
        node_type * alloc()
        {
            node_type * pNew = pop()    ;
            if ( pNew )
                pNew->construct_data()  ;
            else {
                pNew = m_Alloc.New( CDS_TAGGED_ALIGNMENT )    ;
                assert( cds::details::is_aligned<CDS_TAGGED_ALIGNMENT>( pNew ) )   ;
            }
            return pNew ;
        }

        /// Get new node from free-list and call copy constructor. If free-list is empty the new node is allocated
        template <typename T1>
        node_type * alloc( const T1& init)
        {
            node_type * pNew = pop()    ;
            if ( pNew ) {
                assert( cds::details::is_aligned<CDS_TAGGED_ALIGNMENT>( pNew ) )   ;
                pNew->construct_data( init )    ;
            }
            else {
                pNew = m_Alloc.New( CDS_TAGGED_ALIGNMENT, init )    ;
                assert( cds::details::is_aligned<CDS_TAGGED_ALIGNMENT>( pNew ) )   ;
            }
            return pNew ;
        }

        /// Get new node from free-list and call two-argument constructor. If free-list is empty the new node is allocated
        template <typename T1, typename T2>
        node_type * alloc( const T1& init1, const T2& init2)
        {
            node_type * pNew = pop()    ;
            if ( pNew ) {
                assert( cds::details::is_aligned<CDS_TAGGED_ALIGNMENT>( pNew ) )   ;
                pNew->construct_data( init1, init2 )    ;
            }
            else {
                pNew = m_Alloc.New( CDS_TAGGED_ALIGNMENT, init1, init2 )    ;
                assert( cds::details::is_aligned<CDS_TAGGED_ALIGNMENT>( pNew ) )   ;
            }
            return pNew ;
        }

        /// Place node \p p to free-list. The node is not physically destructed
        void free( node_type * p )
        {
            assert( cds::details::is_aligned<CDS_TAGGED_ALIGNMENT>( p ) )   ;
            push( p )    ;
        }
    };
}}} // namespace cds::gc::tagged

#endif // #ifdef CDS_DWORD_CAS_SUPPORTED

#endif    // #ifndef __CDS_GC_TAGGED_DETAILS_FREE_LIST_H
