/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ATOMIC_ATOMIC_DETAILS_H
#define __CDS_ATOMIC_ATOMIC_DETAILS_H

#include <cds/details/defs.h>
#include <cds/atomic/memory_order.h>
#include <cds/compiler/atomic_impl.h>
#include <cds/details/is_aligned.h>

//@cond none
namespace cds {
    namespace atomics { namespace details {

    template <typename T, unsigned int SIZE>
    struct operations ;

    // 32bit atomic operations
    template <typename T>
    struct operations<T, sizeof(atomic32_t)> {
        typedef T           operand_type    ;
        typedef atomic32_t  atomic_type     ;
        typedef atomic32_t  atomic_unaligned    ;
        static const unsigned int operand_size = sizeof(atomic_type)  ;

        // Mandatory atomic primitives

        static inline T load( T volatile const * pAddr, memory_order order )
        {
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            atomic_type n = platform::load32( reinterpret_cast< atomic_type volatile const *>( pAddr ), order )    ;
            return *(T *) &n ;
        }

        static inline T& load( T& dest, T volatile const * pAddr, memory_order order )
        {
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            *((atomic_type *) &dest) = platform::load32( reinterpret_cast< atomic_type volatile const *>( pAddr ), order )    ;
            return dest ;
        }

        static inline T store( T volatile * pAddr, const T val, memory_order order )
        {
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            platform::store32( reinterpret_cast< atomic_type volatile *>( pAddr ), atomic_cast<atomic_type>( val ), order )    ;
            return val ;
        }

        static inline bool cas( T volatile * pDest, const T expected, const T desired, memory_order success_order, memory_order failure_order )
        {
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            return platform::cas32(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                atomic_cast<atomic_type>( expected ),
                atomic_cast<atomic_type>( desired ),
                success_order,
                failure_order
                )   ;
        }

        static inline T vcas( T volatile * pDest, const T expected, const T desired, memory_order success_order, memory_order failure_order )
        {
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            return (T) platform::vcas32(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                atomic_cast<atomic_type>( expected ),
                atomic_cast<atomic_type>( desired ),
                success_order,
                failure_order
                )   ;
        }

        // Optional atomic primitives (can be emulated by CAS)

        static inline T xchg( T volatile * pDest, const T val, memory_order order )
        {
#ifdef CDS_xchg32_defined
            // Processor has native xchg implementation
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            return (T) platform::xchg32(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                atomic_cast<atomic_type>( val ),
                order
                ) ;
#else
            // CAS-based emulation
            operand_type cur ;
            do {
                cur = load( pDest, membar_relaxed::order )  ;
            } while ( !cas( pDest, cur, val, order, membar_relaxed::order ))    ;
            return cur  ;
#endif
        }
    };

    // 64bit atomic operations for user-defined aligned types
    template <typename T>
    struct operations<T, sizeof(atomic64_t)>
    {
        typedef T                   operand_type        ;
        typedef atomic64_t          atomic_type         ;
        typedef atomic64_unaligned  atomic_unaligned    ;

        static const unsigned int operand_size = sizeof(atomic_type)  ;

        // Mandatory atomic primitives

        static inline T load( T volatile const * pAddr, memory_order order )
        {
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            assert( cds::details::is_aligned<8>( pAddr )) ;
            atomic_type v = platform::load64( reinterpret_cast< atomic_type volatile const *>( pAddr ), order )    ;
            return *(( T *) &v) ;
        }

        static inline T& load( T& dest, T volatile const * pAddr, memory_order order )
        {
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            assert( cds::details::is_aligned<8>( pAddr )) ;
            *((atomic_type *) &dest) = platform::load64( reinterpret_cast< atomic_type volatile const *>( pAddr ), order )    ;
            return dest ;
        }

        static inline const T& store( T volatile * pAddr, const T& val, memory_order order )
        {
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            assert( cds::details::is_aligned<8>( pAddr )) ;
            platform::store64( reinterpret_cast< atomic_type volatile *>( pAddr ), atomic_cast<atomic_type>( val ), order )    ;
            return val ;
        }

        static inline bool cas( T volatile * pDest, const T& expected, const T& desired, memory_order success_order, memory_order failure_order )
        {
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            assert( cds::details::is_aligned<8>( pDest )) ;
            return platform::cas64(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                atomic_cast<atomic_type>( expected ),
                atomic_cast<atomic_type>( desired ),
                success_order,
                failure_order
                )   ;
        }

        static inline T vcas( T volatile * pDest, const T& expected, const T& desired, memory_order success_order, memory_order failure_order )
        {
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            assert( cds::details::is_aligned<8>( pDest )) ;
            return (T) platform::vcas64(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                atomic_cast<atomic_type>( expected ),
                atomic_cast<atomic_type>( desired ),
                success_order,
                failure_order
                )   ;
        }

        // Optional atomic primitives (can be emulated by CAS)

        static inline T xchg( T volatile * pDest, const T& val, memory_order order )
        {
#ifdef CDS_xchg64_defined
            // Processor has native xchg implementation
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            assert( cds::details::is_aligned<8>( pDest )) ;
            return (T) platform::xchg64(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                atomic_cast<atomic_type>( val ),
                order
                ) ;
#else
            // CAS-based emulation
            operand_type cur ;
            do {
                cur = load( pDest, membar_relaxed::order )  ;
            } while ( !cas( pDest, cur, val, order, membar_relaxed::order ))    ;
            return cur  ;
#endif
        }
    };

#ifdef CDS_128bit_atomic_defined
    // 128bit atomic operations for user-defined aligned types
    using platform::atomic128_t ;

    template <typename T>
    struct operations<T, sizeof(atomic128_t)> {
        typedef T            operand_type    ;
        typedef atomic128_t  atomic_type     ;
        static const unsigned int operand_size = sizeof(atomic_type)  ;

        // Mandatory atomic primitives

        static inline T load( T volatile const * pAddr, memory_order order )
        {
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            assert( cds::details::is_aligned<16>( pAddr )) ;
            atomic_type v = platform::load128( reinterpret_cast< atomic_type volatile const *>( pAddr ), order )    ;
            return *(( T *) &v) ;
        }

        static inline T& load( T& dest, T volatile const * pAddr, memory_order order )
        {
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            assert( cds::details::is_aligned<16>( pAddr )) ;
            *((atomic_type *) &dest) = platform::load128( reinterpret_cast< atomic_type volatile const *>( pAddr ), order )    ;
            return dest ;
        }

        static inline void store( T volatile * pAddr, const T& val, memory_order order )
        {
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            assert( cds::details::is_aligned<16>( pAddr )) ;
            platform::store128( reinterpret_cast< atomic_type volatile *>( pAddr ), atomic_cast<atomic_type>( val ), order )    ;
        }

        static inline bool cas( T volatile * pDest, const T& expected, const T& desired, memory_order success_order, memory_order failure_order )
        {
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            assert( cds::details::is_aligned<16>( pDest )) ;
            return platform::cas128(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                atomic_cast<atomic_type>( expected ),
                atomic_cast<atomic_type>( desired ),
                success_order,
                failure_order
                )   ;
        }

        static inline T vcas( T volatile * pDest, const T& expected, const T& desired, memory_order success_order, memory_order failure_order )
        {
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            assert( cds::details::is_aligned<16>( pDest )) ;
            return (T) platform::vcas128(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                atomic_cast<atomic_type>( expected ),
                atomic_cast<atomic_type>( desired ),
                success_order,
                failure_order
                )   ;
        }

        // Optional atomic primitives (can be emulated by CAS)

        static inline T xchg( T volatile * pDest, const T& val, memory_order order )
        {
#ifdef CDS_xchg128_defined
            // Processor has native xchg implementation
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            assert( cds::details::is_aligned<16>( pDest )) ;
            return (T) platform::xchg128(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                atomic_cast<atomic_type>( val ),
                order
                ) ;
#else
            // CAS-based emulation
            operand_type cur ;
            do {
                cur = load( pDest, membar_relaxed::order )  ;
            } while ( !cas( pDest, cur, val, order, membar_relaxed::order ))    ;
            return cur  ;
#endif
        }
    };
#endif  // #ifdef CDS_128bit_atomic_defined

    template <typename T, int>  struct integral_operations  ;

    // 32bit atomic operations for predefined integral types
    template <typename T>
    struct integral_operations<T, sizeof(atomic32_t)>: public operations<T, sizeof(atomic32_t)>
    {
        typedef operations<T, sizeof(atomic32_t)>        base_class  ;

        typedef typename base_class::operand_type        operand_type;
        typedef typename base_class::atomic_type         atomic_type ;
        typedef typename base_class::atomic_unaligned    atomic_unaligned    ;

        static inline T xadd( T volatile * pDest, const T val, memory_order order )
        {
#ifdef CDS_xadd32_defined
            // Processor has native xadd implementation
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            return (T) platform::xadd32(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                atomic_cast<atomic_type>( val ),
                order
                ) ;
#else
            // CAS-based emulation
            operand_type cur ;
            do {
                cur = load( pDest, membar_relaxed::order )  ;
            } while ( !cas( pDest, cur, cur + val, order, membar_relaxed::order ))    ;
            return cur  ;
#endif
        }

        static inline T inc( T volatile * pDest, memory_order order )
        {
#ifdef CDS_inc32_defined
            // Processor has native inc implementation
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            return (T) platform::inc32(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                order
                ) ;
#else
            return xadd( pDest, 1, order )  ;
#endif
        }

        static inline T dec( T volatile * pDest, memory_order order )
        {
#ifdef CDS_dec32_defined
            // Processor has native dec implementation
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            return (T) platform::dec32(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                order
                ) ;
#else
            return xadd( pDest, T(0-1), order )  ;
#endif
        }

        static inline T bitwise_and( T volatile * pDest, const T val, memory_order order )
        {
#ifdef CDS_and32_defined
            // Processor has native xand implementation
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            return (T) platform::and32(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                atomic_cast<atomic_type>( val ),
                order
                ) ;
#else
            // CAS-based emulation
            operand_type cur ;
            do {
                cur = load( pDest, membar_relaxed::order )  ;
            } while ( !cas( pDest, cur, cur & val, order, membar_relaxed::order ))    ;
            return cur  ;
#endif
        }

        static inline T bitwise_or( T volatile * pDest, const T val, memory_order order )
        {
#ifdef CDS_or32_defined
            // Processor has native xand implementation
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            return (T) platform::or32(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                atomic_cast<atomic_type>( val ),
                order
                ) ;
#else
            // CAS-based emulation
            operand_type cur ;
            do {
                cur = load( pDest, membar_relaxed::order )  ;
            } while ( !cas( pDest, cur, cur | val, order, membar_relaxed::order ))    ;
            return cur  ;
#endif
        }

        static inline T bitwise_xor( T volatile * pDest, const T val, memory_order order )
        {
#ifdef CDS_xor32_defined
            // Processor has native xand implementation
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            return (T) platform::xor32(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                atomic_cast<atomic_type>( val ),
                order
                ) ;
#else
            // CAS-based emulation
            operand_type cur ;
            do {
                cur = load( pDest, membar_relaxed::order )  ;
            } while ( !cas( pDest, cur, cur ^ val, order, membar_relaxed::order ))    ;
            return cur  ;
#endif
        }
    };

    // 64bit atomic operations for predefined integral types
    template <typename T>
    struct integral_operations<T, sizeof(atomic64_t)>: public operations<T, sizeof(atomic64_t)>
    {
        typedef operations<T, sizeof(atomic64_t)>        base_class  ;

        typedef typename base_class::operand_type        operand_type;
        typedef typename base_class::atomic_type         atomic_type ;
        typedef typename base_class::atomic_unaligned    atomic_unaligned    ;

        static inline T xadd( T volatile * pDest, const T& val, memory_order order )
        {
#ifdef CDS_xadd64_defined
            // Processor has native xadd implementation
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            assert( cds::details::is_aligned<8>( pDest )) ;
            return (T) platform::xadd64(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                atomic_cast<atomic_unaligned>( val ),
                order
                ) ;
#else
            // CAS-based emulation
            operand_type cur ;
            do {
                cur = load( pDest, membar_relaxed::order )  ;
            } while ( !cas( pDest, cur, cur + val, order, membar_relaxed::order ))    ;
            return cur  ;
#endif
        }

        static inline T inc( T volatile * pDest, memory_order order )
        {
#ifdef CDS_inc64_defined
            // Processor has native xadd implementation
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            assert( cds::details::is_aligned<8>( pDest )) ;
            return (T) platform::inc64(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                order
                ) ;
#else
            return xadd( pDest, 1, order )  ;
#endif
        }

        static inline T dec( T volatile * pDest, memory_order order )
        {
#ifdef CDS_dec64_defined
            // Processor has native xadd implementation
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            assert( cds::details::is_aligned<8>( pDest )) ;
            return (T) platform::dec64(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                order
                ) ;
#else
            return xadd( pDest, T(0-1), order )  ;
#endif
        }

        static inline T bitwise_and( T volatile * pDest, const T& val, memory_order order )
        {
#ifdef CDS_and64_defined
            // Processor has native atomic AND implementation
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            assert( cds::details::is_aligned<8>( pDest )) ;

            return (T) platform::and64(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                atomic_cast<atomic_unaligned>( val ),
                order
                ) ;
#else
            // CAS-based emulation
            operand_type cur ;
            do {
                cur = load( pDest, membar_relaxed::order )  ;
            } while ( !cas( pDest, cur, cur & val, order, membar_relaxed::order ))    ;
            return cur  ;
#endif
        }

        static inline T bitwise_or( T volatile * pDest, const T& val, memory_order order )
        {
#ifdef CDS_or64_defined
            // Processor has native atomic OR implementation
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            assert( cds::details::is_aligned<8>( pDest )) ;

            return (T) platform::or64(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                atomic_cast<atomic_unaligned>( val ),
                order
                ) ;
#else
            // CAS-based emulation
            operand_type cur ;
            do {
                cur = load( pDest, membar_relaxed::order )  ;
            } while ( !cas( pDest, cur, cur | val, order, membar_relaxed::order ))    ;
            return cur  ;
#endif
        }

        static inline T bitwise_xor( T volatile * pDest, const T& val, memory_order order )
        {
#ifdef CDS_xor64_defined
            // Processor has native atomic XOR implementation
            CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
            assert( cds::details::is_aligned<8>( pDest )) ;

            return (T) platform::xor64(
                reinterpret_cast< atomic_type volatile *>( pDest ),
                atomic_cast<atomic_unaligned>( val ),
                order
                ) ;
#else
            // CAS-based emulation
            operand_type cur ;
            do {
                cur = load( pDest, membar_relaxed::order )  ;
            } while ( !cas( pDest, cur, cur ^ val, order, membar_relaxed::order ))    ;
            return cur  ;
#endif
        }
    };

    template <typename T, int> struct integral_operations_selector  ;

    template <typename T>
    struct integral_operations_selector<T, 4>
    {
        typedef integral_operations<T, 4>    op_impl    ;
    };
    template <typename T>
    struct integral_operations_selector<T, 8>
    {
        typedef integral_operations<T, 8>    op_impl    ;
    };


    // Pointer atomic operations
    template <typename T>
    struct operations<T *, sizeof(pointer_t)>
    {
        typedef T *         operand_type    ;
        typedef pointer_t   atomic_type     ;
        static const unsigned int operand_size = sizeof(pointer_t)  ;

        // Mandatory atomic primitives

        static inline T * load( T * volatile const * pAddr, memory_order order )
        {
            CDS_STATIC_ASSERT( sizeof(T *) == sizeof(atomic_type) )  ;
            return reinterpret_cast< T *>( platform::loadptr( reinterpret_cast< pointer_t volatile const *>( pAddr ), order )) ;
        }

        static inline T * store( T * volatile * pAddr, T * val, memory_order order )
        {
            CDS_STATIC_ASSERT( sizeof(T *) == sizeof(atomic_type) )  ;
            platform::storeptr( reinterpret_cast< pointer_t volatile *>( pAddr ), reinterpret_cast<pointer_t>( val ), order )    ;
            return val ;
        }

        static inline bool cas( T * volatile * pDest, T * expected, T * desired, memory_order success_order, memory_order failure_order )
        {
            CDS_STATIC_ASSERT( sizeof(T *) == sizeof(atomic_type) )  ;
            return platform::casptr(
                reinterpret_cast< pointer_t volatile *>( pDest ),
                reinterpret_cast<pointer_t>( expected ),
                reinterpret_cast<pointer_t>( desired ),
                success_order,
                failure_order
                )   ;
        }

        static inline T * vcas( T * volatile * pDest, T * expected, T * desired, memory_order success_order, memory_order failure_order )
        {
            CDS_STATIC_ASSERT( sizeof(T *) == sizeof(atomic_type) )  ;
            return reinterpret_cast<T *>( platform::vcasptr(
                reinterpret_cast< pointer_t volatile *>( pDest ),
                reinterpret_cast<pointer_t>( expected ),
                reinterpret_cast<pointer_t>( desired ),
                success_order,
                failure_order
                ))   ;
        }

        // Optional atomic primitives (can be emulated by CAS)

        static inline T * xchg( T * volatile * pDest, T * val, memory_order order )
        {
#ifdef CDS_xchgptr_defined
            // Processor has native xchgptr implementation
            CDS_STATIC_ASSERT( sizeof(T *) == sizeof(atomic_type) )  ;
            return reinterpret_cast<T *>( platform::xchgptr(
                reinterpret_cast< pointer_t volatile *>( pDest ),
                reinterpret_cast<T *>( val ),
                order
                )) ;
#else
            // CAS-based emulation
            atomic_type cur ;
            do {
                cur = load( pDest, membar_relaxed::order )  ;
            } while ( !cas( pDest, cur, val, order, membar_relaxed::order ))    ;
            return cur  ;
#endif
        }
    } ;

    template <typename T, int SIZE> struct atomic_type ;

    template <typename T>
    struct atomic_type<T, 4>
    {
        typedef atomic32_t  type            ;
        typedef atomic32_t  unaligned_type  ;
        typedef T           value_type      ;
        typedef T           aligned_value_type  ;
    };

    template <typename T>
    struct atomic_type<T, 8>
    {
        typedef atomic64_t          type;
        typedef atomic64_unaligned  unaligned_type   ;
        typedef T                   value_type          ;
        //TODO: GCC 4.1, GCC 4.2: error: sorry, applying attributes to template parameter is not implemented
        // Any workaround?..
        typedef T CDS_TYPE_ALIGNMENT(8)     aligned_value_type  ;
    };

#ifdef CDS_128bit_atomic_defined
    template <typename T>
    struct atomic_type<T, 16>
    {
        typedef atomic128_t type                ;
        typedef atomic128_t unaligned_type      ;
        typedef T           value_type          ;
        typedef T CDS_TYPE_ALIGNMENT(16)    aligned_value_type  ;
    };
#endif

    template <typename T>
    struct atomic_type_selector {
        typedef typename atomic_type<T, sizeof(T)>::type                  type                  ;
        typedef typename atomic_type<T, sizeof(T)>::unaligned_type        unaligned_type        ;
        typedef typename atomic_type<T, sizeof(T)>::value_type            value_type            ;
        typedef typename atomic_type<T, sizeof(T)>::aligned_value_type    aligned_value_type    ;
    };
}}} // namespace cds::atomics::details
//@endcond

#endif  // #ifndef __CDS_ATOMIC_ATOMIC_DETAILS_H
