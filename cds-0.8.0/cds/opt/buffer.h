/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_OPT_BUFFER_H
#define __CDS_OPT_BUFFER_H

#include <cds/details/defs.h>
#include <cds/user_setup/allocator.h>
#include <cds/details/allocator.h>

namespace cds { namespace opt {

    /// Option setter for user-provided plain buffer [type-option]
    /**
        This option is used by some container as a random access array for storing 
        cointainer's item; for example, a bounded queue may use
        this option to define underlying buffer implementation.
        
        The template parameter \p TYPE may be \ref rebinding "rebindable".

        Implementations:
            \li opt::v::static_buffer
            \li opt::v::dynamic_buffer
    */
    template <typename TYPE>
    struct buffer {
        //@cond
        template <typename BASE> struct pack: public BASE
        {
            typedef TYPE buffer ;
        };
        //@endcond
    };

    namespace v {

        /// Static buffer (\ref opt::buffer option)
        /**
            One of available type for opt::buffer type-option.

            This buffer maintains static array. No dynamic memory allocation performed.

            \par Template parameters:
                \li \p T - item type the buffer stores
                \li \p CAPACITY - the capacity of buffer. The value must be power of two.
        */
        template <typename T, size_t CAPACITY>
        class static_buffer
        {
        public:
            typedef T   value_type  ;   ///< value type
            static const size_t c_nCapacity = CAPACITY ;    ///< Capacity

            /// Supply \ref rebinding "rebindable" feature
            template <typename Q, size_t CAPACITY2 = c_nCapacity>
            struct rebind {
                //@cond
                typedef static_buffer<Q, CAPACITY2> other   ;   
                //@endcond
            };
        private:
            //@cond
            value_type  m_buffer[c_nCapacity]  ;
            //@endcond
        public:
            /// Construct static buffer
            static_buffer()
            {
                // Capacity must be power of 2
                CDS_STATIC_ASSERT( (c_nCapacity & (c_nCapacity - 1)) == 0 ) ;
            }
            /// Construct buffer of given capacity
            /**
                This ctor ignores \p nCapacity argument. The capacity of static buffer
                is defined by template argument \p CAPACITY
            */
            static_buffer( size_t nCapacity )
            {
                // Capacity must be power of 2
                CDS_STATIC_ASSERT( (c_nCapacity & (c_nCapacity - 1)) == 0 ) ;
                //assert( c_nCapacity == nCapacity )  ;
            }

            /// Get item \p i
            value_type& operator []( size_t i )
            {
                assert( i < capacity() )    ;
                return m_buffer[i]          ;
            }

            /// Returns buffer capacity 
            size_t capacity() const
            {
                return c_nCapacity  ;
            }

        private:
            //@cond
            // non-copyable
            static_buffer(const static_buffer&) ;
            void operator =(const static_buffer&);
            //@endcond
        };


        /// Dynamically allocated buffer
        /**
            One of available opt::buffer type-option.

            This buffer maintains dynamically allocated array.
            Allocation is performed at construction time.

            \par Template parameters:
                \li \p T - item type storing in the buffer
                \li \p ALLOC - an allocator used for allocating internal buffer (\p std::allocator interface)
        */
        template <typename T, class ALLOC = CDS_DEFAULT_ALLOCATOR>
        class dynamic_buffer
        {
        public:
            typedef T   value_type  ;   ///< Value type

            /// Supply \ref rebinding "rebindable" feature
            template <typename Q>
            struct rebind {
                //@cond
                typedef dynamic_buffer<Q, ALLOC> other   ; 
                //@endcond
            };

            //@cond
            typedef cds::details::Allocator<value_type, ALLOC>   allocator_type  ;
            //@endcond

        private:
            //@cond
            value_type *    m_buffer    ;
            size_t const    m_nCapacity ;
            //@endcond
        public:
            /// Allocates dynamic buffer of given \p nCapacity
            /**
                \p nCapacity must be power of two.
            */
            dynamic_buffer( size_t nCapacity )
                : m_nCapacity( nCapacity )
            {
                assert( nCapacity >= 2 )    ;
                // Capacity must be power of 2
                assert( (nCapacity & (nCapacity - 1)) == 0 ) ;

                allocator_type a    ;
                m_buffer = a.NewArray( nCapacity )  ;
            }

            /// Destroys dynamically allocated buffer
            ~dynamic_buffer()
            {
                allocator_type a    ;
                a.Delete( m_buffer, m_nCapacity )   ;
            }

            /// Get item \p i
            value_type& operator []( size_t i )
            {
                assert( i < capacity() )    ;
                return m_buffer[i]          ;
            }

            /// Returns buffer capacity 
            size_t capacity() const
            {
                return m_nCapacity  ;
            }

        private:
            //@cond
            // non-copyable
            dynamic_buffer(const dynamic_buffer&) ;
            void operator =(const dynamic_buffer&);
            //@endcond
        };

    }   // namespace v

}}  // namespace cds::opt

#endif // #ifndef __CDS_OPT_BUFFER_H
