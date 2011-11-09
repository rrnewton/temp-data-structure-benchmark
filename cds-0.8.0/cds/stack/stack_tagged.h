/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_STACK_TAGGED_H
#define __CDS_STACK_TAGGED_H

#include <cds/stack/stack_base.h>
#include <cds/gc/tagged/gc.h>
#include <cds/details/void_selector.h>

#ifdef CDS_DWORD_CAS_SUPPORTED

namespace cds {
    namespace stack {
        //@cond
        namespace details {
            template <typename T>
            struct CDS_TAGGED_ALIGN_ATTRIBUTE node_tagged: public gc::tagged::ContainerNode
            {
                typedef gc::tagged::tagged_type< node_tagged *> tagged_ptr  ;

                tagged_ptr  m_Next    ;
                T           m_data    ;

                node_tagged()    
                {}

                node_tagged( const T& src ): m_data( src ) 
                {}

                void destroy_data()
                {
                    m_data.T::~T()   ;
                }

                void construct_data()
                {
                    new( &m_data ) T  ;
                }

                void construct_data(const T& src)
                {
                    new( &m_data ) T(src)  ;
                }
            };
        }    // namespace details
        //@endcond

        /// Lock-free stack based on IBM tag methodology
        /**
            Source:
                \par [2004] Maged Michael "ABA prevention using single-word instruction"

            This class is a specialization of stack::Stack template for gc::tagged_gc memory reclamation schema.
        */
        template < typename T, class TRAITS, class ALLOCATOR>
        class Stack< gc::tagged_gc, T, TRAITS, ALLOCATOR>
        {
        protected:
            typedef details::node_tagged<T>                Node        ;   ///< Node type

        public:
            typedef gc::tagged_gc    gc_schema    ;    ///< Memory reclamation schema
            typedef T                value_type    ;   ///< Value type

            /// Back-off strategy
            typedef typename cds::details::void_selector<
                typename TRAITS::backoff_strategy,
                cds::backoff::empty
            >::type  backoff_strategy ; 

            /// free list implementation            
            typedef typename cds::details::void_selector<
                typename TRAITS::free_list,
                gc::tagged::FreeList< Node >
            >::type::template rebind< Node>::other free_list   ;   

        protected:
            typedef typename Node::tagged_ptr       tagged_ptr  ;   ///< tagged node type
            //@cond
            typedef typename free_list::node_type * node_ptr    ;
            //@endcond

            tagged_ptr  m_Top       ;   ///< Stack top
            free_list   m_FreeList  ;   ///< Free list

            //@cond
            node_ptr    allocNode()                    { return m_FreeList.alloc()            ; }
            node_ptr    allocNode( const T& src )    { return m_FreeList.alloc( src )    ; }
            void        freeNode( node_ptr pNode )    { m_FreeList.free( pNode )            ; }
            //@endcond

        public:
            Stack()
            {}

            /// Push data to stack
            bool push( const T& data )
            {
                Node * pNew = allocNode( data )    ;
                Node * t    ;
                backoff_strategy bkoff ;
                while( true ) {
                    pNew->m_Next.m_data =
                        t = atomics::load<membar_acquire>( &m_Top.m_data ) ;
                    if ( atomics::cas<membar_release>( &m_Top.m_data, t, pNew ))
                        return true    ;
                    bkoff()    ;
                }
            }

            /// Pop data from top of stack. Returns \p false if stack is empty
            bool pop( T& dst ) {
                tagged_ptr t    ;
                tagged_ptr next    ;
                backoff_strategy bkoff ;
                while ( true ) {
                    t = atomics::load<membar_acquire>( &m_Top ) ;
                    if ( t.data() == NULL )
                        return false    ;
                    next = atomics::load<membar_acquire>( &( t.data()->m_Next ) ) ;
                    if ( gc::tagged::cas_tagged<membar_release>( m_Top, t, next.data() ) )
                        break   ;
                    bkoff() ;
                }

                dst = t.data()->m_data    ;
                freeNode( t.data() )    ;
                return true    ;
            }

            /// Checks ifthe stack is empty
            bool empty() const
            {
                return atomics::load<membar_acquire>( &m_Top.m_data ) == NULL  ;
            }

            /// Clears the stack in lock-free manner
            size_t clear()
            {
                size_t nRemoved = 0    ;
                T d    ;
                while ( !empty()) {
                    pop( d )    ;
                    ++nRemoved    ;
                }
                return nRemoved    ;
            }
        } ;
    }    // namespace stack
}    // namespace cds

#endif // #ifndef CDS_DWORD_CAS_SUPPORTED

#endif    // #ifndef __CDS_STACK_TAGGED_H
