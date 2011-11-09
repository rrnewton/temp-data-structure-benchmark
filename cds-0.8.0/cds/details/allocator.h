/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_DETAILS_ALLOCATOR_H
#define __CDS_DETAILS_ALLOCATOR_H

/*
    Allocator class for the library. Supports allocating and constructing of objects

    Editions:
        2008.03.08    Maxim.Khiszinsky    Created
*/

#include <cds/user_setup/allocator.h>
#include <memory>
#include <boost/type_traits/has_trivial_destructor.hpp>

namespace cds {
    namespace details {

        /// Extends \p std::allocator interface to provide semantics like operator \p new and \p delete
        /**
            The class is the wrapper around underlying \p ALLOCATOR class. \p ALLOCATOR provides the
            interface defined in C++ standard.
        */
        template <typename T, class ALLOCATOR = CDS_DEFAULT_ALLOCATOR >
        class Allocator: public ALLOCATOR::template rebind<T>::other
        {
        public:
            /// Underlying allocator type
            typedef typename ALLOCATOR::template rebind<T>::other   allocator_type   ;

#       ifdef CDS_COMPILER_SUPPORTS_VARIADIC_TEMPLATE
            /// Analogue of operator new T(\p src... )
            template <typename... S>
            T *  New( const S&... src )
            {
                return Construct( allocator_type::allocate(1), src... ) ;
            }
#       else
            //@cond
            /// Analogue of operator new T
            T *  New()
            {
                return Construct( allocator_type::allocate(1) ) ;
            }

            /// Analogue of operator new T(\p src )
            template <typename S>
            T *  New( const S& src )
            {
                return Construct( allocator_type::allocate(1), src ) ;
            }

            /// Analogue of operator new T( \p s1, \p s2 )
            template <typename S1, typename S2>
            T *  New( const S1& s1, const S2& s2 )
            {
                return Construct( allocator_type::allocate(1), s1, s2 ) ;
            }

            /// Analogue of operator new T( \p s1, \p s2, \p s3 )
            template <typename S1, typename S2, typename S3>
            T *  New( const S1& s1, const S2& s2, const S3& s3 )
            {
                return Construct( allocator_type::allocate(1), s1, s2, s3 ) ;
            }
            //@endcond
#       endif

            /// Analogue of operator new T[\p nCount ]
            T * NewArray( size_t nCount )
            {
                T * p = allocator_type::allocate( nCount )  ;
                for ( size_t i = 0; i < nCount; ++i )
                    Construct( p + i )  ;
                return p    ;
            }

            /// Analogue of operator new T[\p nCount ].
            /**
                Each item of array of type T is initialized by parameter \p src: T( src )
            */
            template <typename S>
            T * NewArray( size_t nCount, const S& src )
            {
                T * p = allocator_type::allocate( nCount )  ;
                for ( size_t i = 0; i < nCount; ++i )
                    Construct( p + i, src )  ;
                return p    ;
            }

            /// Analogue of operator delete
            void Delete( T * p )
            {
                allocator_type::destroy( p )    ;
                allocator_type::deallocate( p, 1 )    ;
            }

            /// Analogue of operator delete []
            void Delete( T * p, size_t nCount )
            {
                 for ( size_t i = 0; i < nCount; ++i )
                     allocator_type::destroy( p + i )    ;
                allocator_type::deallocate( p, nCount )    ;
            }

#       ifdef CDS_COMPILER_SUPPORTS_VARIADIC_TEMPLATE
            /// Analogue of placement operator new( \p p ) T( src... )
            template <typename... S>
            T * Construct( void * p, const S&... src )
            {
                return new( p ) T( src... )    ;
            }
#       else
            //@cond
            /// Analogue of placement operator new( \p p ) T
            T * Construct( void * p )
            {
                return new( p ) T ;
            }


            /// Analogue of placement operator new( \p p ) T( \p src )
            template <typename S>
            T * Construct( void * p, const S& src )
            {
                return new( p ) T( src )    ;
            }

            /// Analogue of placement operator new( \p p ) T( \p s1, \p s2 )
            template <typename S1, typename S2>
            T *  Construct( void * p, const S1& s1, const S2& s2 )
            {
                return new( p ) T( s1, s2 ) ;
            }

            /// Analogue of placement operator new( \p p ) T( \p s1, \p s2, \p s3 )
            template <typename S1, typename S2, typename S3>
            T *  Construct( void * p, const S1& s1, const S2& s2, const S3& s3 )
            {
                return new( p ) T( s1, s2, s3 ) ;
            }
            //@endcond
#       endif

            /// Rebinds allocator to other type \p Q instead of \p T
            template <typename Q>
            struct rebind {
                typedef Allocator< Q, typename ALLOCATOR::template rebind<Q>::other >    other ; ///< Rebinding result
            };
        };

        //@cond
        namespace {
            template <class T>
            static inline void impl_call_dtor(T* p, const boost::false_type&)
            {
                p->T::~T()  ;
            }

            template <class T>
            static inline void impl_call_dtor(T* p, const boost::true_type&)
            {}
        }
        //@endcond

        /// Helper function to call destructor of type T
        /**
            This function is empty for the type T that has trivial destructor.
        */
        template <class T>
        static inline void call_dtor( T* p )
        {
            impl_call_dtor( p, ::boost::has_trivial_destructor<T>() );
        }


        /// Deferral removing of the object of type \p T. Helper class
        template <typename T, typename ALLOCATOR = CDS_DEFAULT_ALLOCATOR>
        struct deferral_deleter {
            typedef T               type            ;   ///< Type
            typedef ALLOCATOR       allocator_type  ;   ///< Allocator for removing

            /// Frees the object \p p
            /**
                Caveats: this function uses temporary object of type \ref cds::details::Allocator<T, ALLOCATOR> to free the node \p p.
                So, the node allocator should be stateless. It is standard requirement for \p std::allocator class objects.

                Do not use this function directly.
            */
            static void free( T * p )
            {
                Allocator<T, ALLOCATOR> a   ;
                a.Delete( p )   ;
            }
        };

    }    // namespace details
}    // namespace cds

#endif    // #ifndef __CDS_DETAILS_ALLOCATOR_H
