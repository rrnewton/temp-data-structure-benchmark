/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_STACK_HZP_H
#define __CDS_STACK_HZP_H

#include <cds/stack/stack_base.h>
#include <cds/gc/hzp/hp_object.h>
#include <cds/details/void_selector.h>

namespace cds {
    /// Various stack implementations
    namespace stack {
        namespace details {

            /// Stack's node type for Hazard Pointer memory reclamation schema
            template <typename T>
            struct node_hp {
                atomic< node_hp<T> * >  m_pNext ;   ///< Pointer to next node in the stack
                T                        m_Data  ;   ///< Data stored in the node

                //@cond
                node_hp(): m_pNext( NULL ) {}
                node_hp( const T& data ): m_pNext( NULL ), m_Data( data ) {}
                //@endcond
            };
        }

        /// Lock-free stack based on Hazard Pointer schema
           /**
            Source:
                [2002] Maged M.Michael "Safe memory reclamation for dynamic lock-freeobjects using atomic reads and writes"

            This class is a specialization of stack::Stack template for gc::hzp_gc memory reclamation schema.

        */
        template < typename T, class TRAITS, class ALLOCATOR>
        class Stack< gc::hzp_gc, T, TRAITS, ALLOCATOR >: public gc::hzp::Container< details::node_hp<T>, ALLOCATOR >
        {
            typedef gc::hzp::Container< details::node_hp<T>, ALLOCATOR >    base_class   ;   ///< Base class

        public:
            typedef gc::hzp_gc    gc_schema    ;    ///< Garbage collection schema
            typedef T            value_type    ;   ///< Value type

            typedef typename cds::details::void_selector<
                typename TRAITS::backoff_strategy,
                cds::backoff::empty
            >::type                 backoff_strategy ; ///< Back-off strategy

        protected:
            typedef typename base_class::Node     Node    ;   ///< Node type

        protected:
            atomic<Node *>      m_pTop  ;   ///< Top of the stack

        public:
            Stack(): m_pTop( NULL ) {}

            /// Push data to stack
            bool push( const T& data )
            {
                Node * pNew = allocNode( data )    ;
                Node * t    ;
                backoff_strategy bkoff ;
                while( true ) {
                    t = m_pTop.template load<membar_acquire>() ;
                    pNew->m_pNext.template store<membar_relaxed>( t )  ;   // Next CAS orders memory
                    if ( m_pTop.template cas<membar_acq_rel>( t, pNew ))
                        return true    ;
                    bkoff()    ;
                }
            }

            /// Pop data from the stack. Returns \p false if stack is empty
            bool pop( T& data )
            {
                backoff_strategy bkoff ;
                gc::hzp::AutoHPGuard hpGuard( base_class::getGC() )    ;

                while ( true ) {
                    Node * t = m_pTop.template load<membar_acquire>()  ;
                    if ( t == NULL )
                        return false    ;    // stack is empty

                    hpGuard = t        ;
                    if ( t != m_pTop.template load<membar_relaxed>() )
                        continue    ;
                    Node * pNext = t->m_pNext.template load<membar_acquire>()  ;
                    if ( m_pTop.template cas<membar_acq_rel>( t, pNext ) ) {
                        data = t->m_Data        ;
                        hpGuard.getGC().retirePtr( t, base_class::deferral_node_deleter::free ) ;
                        return true    ;
                    }
                    bkoff()    ;
                }
            }

            /// Checks if the stack is empty
            bool empty() const
            {
                return m_pTop.template load<membar_acquire>() == NULL;
            }

            /// Clears the stack in lock-free manner
            size_t clear()
            {
                backoff_strategy bkoff ;
                gc::hzp::AutoHPGuard hpGuard( base_class::getGC() )    ;
                Node * pTop    ;
                while ( true ) {
                    pTop = m_pTop.template load<membar_acquire>()  ;
                    if ( pTop == NULL )
                        return 0    ;
                    hpGuard = pTop        ;
                    if ( pTop != m_pTop.template load<membar_relaxed>())
                        continue    ;
                    if ( m_pTop.template cas<membar_acq_rel>( pTop, (Node *) NULL ))
                        break    ;
                    bkoff()    ;
                }

                size_t nRemoved = 0    ;
                while( pTop ) {
                    Node * p = pTop    ;
                    pTop = p->m_pNext.template load<membar_relaxed>()  ;
                    hpGuard.getGC().retirePtr( p, base_class::deferral_node_deleter::free ) ;
                    ++nRemoved    ;
                }
                return nRemoved    ;
            }
        };
    }    // namespace stack
}    // namespace cds

#endif    // #ifndef __CDS_HZSTACK_H
