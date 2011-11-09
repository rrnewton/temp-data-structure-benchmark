/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ATOMIC_ATOMIC_TEMPLATE_H
#define __CDS_ATOMIC_ATOMIC_TEMPLATE_H

#include <cds/atomic/atomic_details.h>

namespace cds { namespace atomics {

    //@cond none
    namespace details {

        template <typename ORDER, typename T, unsigned int SIZE>
        struct tmpl_ops ;

        // 32bit atomic operations
        template <typename ORDER, typename T>
        struct tmpl_ops<ORDER, T, sizeof(atomic32_t)>
        {
            typedef integral_operations<T, sizeof(atomic32_t)>   std_ops   ;

            typedef typename std_ops::operand_type     operand_type    ;
            typedef typename std_ops::atomic_type      atomic_type     ;

            static const unsigned int operand_size = std_ops::operand_size  ;

            template <typename OTHER_ORDER>
            struct rebind_order {
                typedef tmpl_ops<OTHER_ORDER, T, sizeof(T)> other   ;
            };

            // Mandatory atomic primitives

            static inline T load( T volatile const * pAddr )
            {
                CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                atomic_type n = platform::load32<ORDER>( reinterpret_cast< atomic_type volatile const *>( pAddr ) )    ;
                return *(T *) &n ;
#           else
                return std_ops::load( pAddr, ORDER::order )  ;
#           endif
            }

            static inline T& load( T& dest, T volatile const * pAddr )
            {
                CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                *((atomic_type *) &dest) = platform::load32<ORDER>( reinterpret_cast< atomic_type volatile const *>( pAddr ) )    ;
                return dest ;
#           else
                return std_ops::load( dest, pAddr, ORDER::order )  ;
#           endif
            }

            static inline T store( T volatile * pAddr, const T val )
            {
                CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                platform::store32<ORDER>( reinterpret_cast< atomic_type volatile *>( pAddr ), atomic_cast<atomic_type>( val ) )    ;
#           else
                std_ops::store( pAddr, val, ORDER::order )   ;
#           endif
                return val ;
            }

            static inline bool cas( T volatile * pDest, const T expected, const T desired )
            {
                CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                return platform::cas32<ORDER>(
                    reinterpret_cast< atomic_type volatile *>( pDest ),
                    atomic_cast<atomic_type>( expected ),
                    atomic_cast<atomic_type>( desired )
                    )   ;
#           else
                return std_ops::cas( pDest, expected, desired, ORDER::order, membar_relaxed::order )    ;
#           endif
            }

            static inline T vcas( T volatile * pDest, const T expected, const T desired )
            {
                CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                return (T) platform::vcas32<ORDER>(
                    reinterpret_cast< atomic_type volatile *>( pDest ),
                    atomic_cast<atomic_type>( expected ),
                    atomic_cast<atomic_type>( desired )
                    )   ;
#           else
                return std_ops::vcas( pDest, expected, desired, ORDER::order, membar_relaxed::order )    ;
#           endif
            }

            // Optional atomic primitives (can be emulated by CAS)

            static inline T xchg( T volatile * pDest, const T val )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
#               ifdef CDS_xchg32_defined
                    // Processor has native xchg implementation
                    CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                    return (T) platform::xchg32<ORDER>( reinterpret_cast< atomic_type volatile *>( pDest ), atomic_cast<atomic_type>( val ) ) ;
#               else
                    // CAS-based emulation
                    operand_type cur ;
                    do {
                        cur = rebind_order<membar_relaxed>::other::load( pDest )  ;
                    } while ( !cas( pDest, cur, val ))    ;
                    return cur  ;
#               endif
#           else
                return std_ops::xchg( pDest, val, ORDER::order )   ;
#           endif
            }

            static inline T xadd( T volatile * pDest, const T val )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
#               ifdef CDS_xadd32_defined
                    // Processor has native xadd implementation
                    return (T) platform::xadd32<ORDER>( reinterpret_cast< atomic_type volatile *>( pDest ), atomic_cast<atomic_type>( val ) ) ;
#               else
                    // CAS-based emulation
                    operand_type cur ;
                    do {
                        cur = rebind_order<membar_relaxed>::other::load( pDest )  ;
                    } while ( !cas( pDest, cur, cur + val ))    ;
                    return cur  ;
#               endif
#           else
                return std_ops::xadd( pDest, val, ORDER::order )   ;
#           endif
            }

            static inline T inc( T volatile * pDest )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
#               ifdef CDS_inc32_defined
                    // Processor has native inc implementation
                    CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                    return (T) platform::inc32<ORDER>( reinterpret_cast< atomic_type volatile *>( pDest ) ) ;
#               else
                    return xadd( pDest, 1 )  ;
#               endif
#           else
                return std_ops::inc( pDest, ORDER::order )   ;
#           endif
            }

            static inline T dec( T volatile * pDest )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
#               ifdef CDS_dec32_defined
                    // Processor has native dec implementation
                    CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                    return (T) platform::dec32<ORDER>( reinterpret_cast< atomic_type volatile *>( pDest ) ) ;
#               else
                    return xadd( pDest, T(0-1) )  ;
#               endif
#           else
                return std_ops::dec( pDest, ORDER::order )   ;
#           endif
            }

            static inline T bitwise_and( T volatile * pDest, const T val )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
#               ifdef CDS_and32_defined
                    // Processor has native xand implementation
                    CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                    return (T) platform::and32<ORDER>( reinterpret_cast< atomic_type volatile *>( pDest ), atomic_cast<atomic_type>( val ) ) ;
#               else
                    // CAS-based emulation
                    operand_type cur ;
                    do {
                        cur = rebind_order<membar_relaxed>::other::load( pDest )  ;
                    } while ( !cas( pDest, cur, cur & val ))    ;
                    return cur  ;
#               endif
#           else
                return std_ops::bitwise_and( pDest, val, ORDER::order )   ;
#           endif
            }

            static inline T bitwise_or( T volatile * pDest, const T val )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
#               ifdef CDS_or32_defined
                    // Processor has native bitwise_or implementation
                    CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                    return (T) platform::or32<ORDER>( reinterpret_cast< atomic_type volatile *>( pDest ), atomic_cast<atomic_type>( val ) ) ;
#               else
                    // CAS-based emulation
                    operand_type cur ;
                    do {
                        cur = rebind_order<membar_relaxed>::other::load( pDest )  ;
                    } while ( !cas( pDest, cur, cur | val ))    ;
                    return cur  ;
#               endif
#           else
                return std_ops::bitwise_or( pDest, val, ORDER::order )   ;
#           endif
            }

            static inline T bitwise_xor( T volatile * pDest, const T val )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
#               ifdef CDS_xor32_defined
                    // Processor has native bitwise_or implementation
                    CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                    return (T) platform::xor32<ORDER>( reinterpret_cast< atomic_type volatile *>( pDest ), atomic_cast<atomic_type>( val ) ) ;
#               else
                    // CAS-based emulation
                    operand_type cur ;
                    do {
                        cur = rebind_order<membar_relaxed>::other::load( pDest )  ;
                    } while ( !cas( pDest, cur, cur ^ val ))    ;
                    return cur  ;
#               endif
#           else
                return std_ops::bitwise_xor( pDest, val, ORDER::order )   ;
#           endif
            }
        };

        // Pointer atomic operations
        template <typename ORDER, typename T>
        struct tmpl_ops<ORDER, T *, sizeof(pointer_t)>
        {
            typedef operations<T *, sizeof(pointer_t)>   std_ops   ;

            typedef typename std_ops::operand_type     operand_type    ;
            typedef typename std_ops::atomic_type      atomic_type     ;

            static const unsigned int operand_size = std_ops::operand_size  ;

            template <typename OTHER_ORDER>
            struct rebind_order {
                typedef tmpl_ops<OTHER_ORDER, T *, sizeof(pointer_t)> other   ;
            };

            // Mandatory atomic primitives

            static inline T * load( T * volatile const * pAddr )
            {
                CDS_STATIC_ASSERT( sizeof(T *) == sizeof(atomic_type) )  ;
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                return reinterpret_cast< T *>( platform::loadptr<ORDER>( reinterpret_cast< pointer_t volatile const *>( pAddr ) )) ;
#           else
                return std_ops::load( pAddr, ORDER::order )    ;
#           endif
            }

            static inline T * store( T * volatile * pAddr, T * val )
            {
                CDS_STATIC_ASSERT( sizeof(T *) == sizeof(atomic_type) )  ;
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                platform::storeptr<ORDER>( reinterpret_cast< pointer_t volatile *>( pAddr ), reinterpret_cast<pointer_t>( val ) )    ;
#           else
                std_ops::store( pAddr, val, ORDER::order )    ;
#           endif
                return val  ;
            }

            static inline bool cas( T * volatile * pDest, T * expected, T * desired )
            {
                CDS_STATIC_ASSERT( sizeof(T *) == sizeof(atomic_type) )  ;
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                return platform::casptr<ORDER>(
                    reinterpret_cast< pointer_t volatile *>( pDest ),
                    reinterpret_cast<pointer_t>( expected ),
                    reinterpret_cast<pointer_t>( desired )
                )   ;
#           else
                return std_ops::cas( pDest, expected, desired, ORDER::order, membar_relaxed::order )    ;
#           endif
            }

            static inline T * vcas( T * volatile * pDest, T * expected, T * desired )
            {
                CDS_STATIC_ASSERT( sizeof(T *) == sizeof(atomic_type) )  ;
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                return reinterpret_cast<T *>( platform::vcasptr<ORDER>(
                    reinterpret_cast< pointer_t volatile *>( pDest ),
                    reinterpret_cast<pointer_t>( expected ),
                    reinterpret_cast<pointer_t>( desired )
                ))   ;
#           else
                return std_ops::vcas( pDest, expected, desired, ORDER::order, membar_relaxed::order )    ;
#           endif
            }

            // Optional atomic primitives (can be emulated by CAS)

            static inline T * xchg( T * volatile * pDest, T * val )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
#               ifdef CDS_xchgptr_defined
                    // Processor has native xchgptr implementation
                    CDS_STATIC_ASSERT( sizeof(T *) == sizeof(atomic_type) )  ;
                    return reinterpret_cast<T *>( platform::xchgptr<ORDER>(
                        reinterpret_cast< pointer_t volatile *>( pDest ),
                        reinterpret_cast<T *>( val )
                    )) ;
#               else
                    // CAS-based emulation
                    operand_type cur ;
                    do {
                        cur = rebind_order<membar_relaxed>::other::load( pDest )  ;
                    } while ( !cas( pDest, cur, val ))    ;
                    return cur  ;
#               endif
#           else
                return std_ops::xchg( pDest, val, ORDER::order )    ;
#           endif
            }
        } ;

        //64bit atomic operations
        template <typename ORDER, typename T>
        struct tmpl_ops<ORDER, T, sizeof(atomic64_t)>
        {
            typedef integral_operations<T, sizeof(atomic64_t)>   std_ops    ;
            typedef typename std_ops::operand_type      operand_type        ;
            typedef typename std_ops::atomic_type       atomic_type         ;
            //typedef typename std_ops::atomic_unaligned  atomic_unaligned    ;

            static const unsigned int operand_size = std_ops::operand_size  ;

            template <typename OTHER_ORDER>
            struct rebind_order {
                typedef tmpl_ops<OTHER_ORDER, T, sizeof(T)> other   ;
            };

            // Mandatory atomic primitives

            static inline T load( T volatile const * pAddr )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                assert( cds::details::is_aligned<8>( pAddr )) ;
                atomic_type v = platform::load64<ORDER>( reinterpret_cast< atomic_type volatile const *>( pAddr ) )    ;
                return *(( T *) &v) ;
#           else
                return std_ops::load( pAddr, ORDER::order )    ;
#           endif
            }

            static inline T& load( T& dest, T volatile const * pAddr )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                assert( cds::details::is_aligned<8>( pAddr )) ;
                *((atomic_type *) &dest) = platform::load64<ORDER>( reinterpret_cast< atomic_type volatile const *>( pAddr ) )  ;
                return dest ;
#           else
                return std_ops::load( dest, pAddr, ORDER::order )    ;
#           endif
            }

            static inline const T& store( T volatile * pAddr, const T& val )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                assert( cds::details::is_aligned<8>( pAddr )) ;
                platform::store64<ORDER>( reinterpret_cast< atomic_type volatile *>( pAddr ), atomic_cast<atomic_type>( val ) )    ;
#           else
                return std_ops::store( pAddr, val, ORDER::order )    ;
#           endif
                return val ;
            }

            static inline bool cas( T volatile * pDest, const T& expected, const T& desired )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                assert( cds::details::is_aligned<8>( pDest )) ;
                return platform::cas64<ORDER>(
                    reinterpret_cast< atomic_type volatile *>( pDest ),
                    atomic_cast<atomic_type>( expected ),
                    atomic_cast<atomic_type>( desired )
                )   ;
#           else
                return std_ops::cas( pDest, expected, desired, ORDER::order, membar_relaxed::order )    ;
#           endif
            }

            static inline T vcas( T volatile * pDest, const T& expected, const T& desired )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                assert( cds::details::is_aligned<8>( pDest )) ;
                return (T) platform::vcas64<ORDER>(
                    reinterpret_cast< atomic_type volatile *>( pDest ),
                    atomic_cast<atomic_type>( expected ),
                    atomic_cast<atomic_type>( desired )
                )   ;
#           else
                return std_ops::vcas( pAddr, expected, desired, ORDER::order, membar_relaxed::order )    ;
#           endif
            }

            // Optional atomic primitives (can be emulated by CAS)

            static inline T xchg( T volatile * pDest, const T& val )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
#               ifdef CDS_xchg64_defined
                    // Processor has native xchg implementation
                    CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                    assert( cds::details::is_aligned<8>( pDest )) ;
                    return (T) platform::xchg64<ORDER>( reinterpret_cast< atomic_type volatile *>( pDest ), atomic_cast<atomic_type>( val ) ) ;
#               else
                    // CAS-based emulation
                    operand_type cur ;
                    do {
                        cur = rebind_order<membar_relaxed>::other::load( pDest )  ;
                    } while ( !cas( pDest, cur, val ))    ;
                    return cur  ;
#               endif
#           else
                return std_ops::xchg( pDest, val, ORDER::order )    ;
#           endif
            }

            static inline T xadd( T volatile * pDest, const T& val )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
#               ifdef CDS_xadd64_defined
                    // Processor has native xadd implementation
                    return (T) platform::xadd64<ORDER>( reinterpret_cast< atomic_type volatile *>( pDest ), atomic_cast<atomic_type>( val ) ) ;
#               else
                    // CAS-based emulation
                    operand_type cur ;
                    do {
                        cur = rebind_order<membar_relaxed>::other::load( pDest )  ;
                    } while ( !cas( pDest, cur, cur + val ))    ;
                    return cur  ;
#               endif
#           else
                return std_ops::xadd( pDest, val, ORDER::order )   ;
#           endif
            }

            static inline T inc( T volatile * pDest )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
#               ifdef CDS_inc64_defined
                    // Processor has native inc implementation
                    CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                    return (T) platform::inc64<ORDER>( reinterpret_cast< atomic_type volatile *>( pDest ) ) ;
#               else
                    return xadd( pDest, 1 )  ;
#               endif
#           else
                return std_ops::inc( pDest, ORDER::order )   ;
#           endif
            }

            static inline T dec( T volatile * pDest )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
#               ifdef CDS_dec64_defined
                    // Processor has native dec implementation
                    CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                    return (T) platform::dec64<ORDER>( reinterpret_cast< atomic_type volatile *>( pDest ) ) ;
#               else
                    return xadd( pDest, T(0-1) )  ;
#               endif
#           else
                return std_ops::dec( pDest, ORDER::order )   ;
#           endif
            }

            static inline T bitwise_and( T volatile * pDest, const T& val )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
#               ifdef CDS_and64_defined
                    // Processor has native xand implementation
                    CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                    return (T) platform::and64<ORDER>( reinterpret_cast< atomic_type volatile *>( pDest ), atomic_cast<atomic_type>( val ) ) ;
#               else
                    // CAS-based emulation
                    operand_type cur ;
                    do {
                        cur = rebind_order<membar_relaxed>::other::load( pDest )  ;
                    } while ( !cas( pDest, cur, cur & val ))    ;
                    return cur  ;
#               endif
#           else
                return std_ops::bitwise_and( pDest, val, ORDER::order )   ;
#           endif
            }

            static inline T bitwise_or( T volatile * pDest, const T& val )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
#               ifdef CDS_or64_defined
                    // Processor has native bitwise_or implementation
                    CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                    return (T) platform::or64<ORDER>( reinterpret_cast< atomic_type volatile *>( pDest ), atomic_cast<atomic_type>( val ) ) ;
#               else
                    // CAS-based emulation
                    operand_type cur ;
                    do {
                        cur = rebind_order<membar_relaxed>::other::load( pDest )  ;
                    } while ( !cas( pDest, cur, cur | val ))    ;
                    return cur  ;
#               endif
#           else
                return std_ops::bitwise_or( pDest, val, ORDER::order )   ;
#           endif
            }

            static inline T bitwise_xor( T volatile * pDest, const T& val )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
#               ifdef CDS_xor64_defined
                    // Processor has native bitwise_or implementation
                    CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                    return (T) platform::xor64<ORDER>( reinterpret_cast< atomic_type volatile *>( pDest ), atomic_cast<atomic_type>( val ) ) ;
#               else
                    // CAS-based emulation
                    operand_type cur ;
                    do {
                        cur = rebind_order<membar_relaxed>::other::load( pDest )  ;
                    } while ( !cas( pDest, cur, cur ^ val ))    ;
                    return cur  ;
#               endif
#           else
                return std_ops::bitwise_xor( pDest, val, ORDER::order )   ;
#           endif
            }
        };

#ifdef CDS_128bit_atomic_defined
        // 128bit atomic operations for user-defined aligned types

        template <typename ORDER, typename T>
        struct tmpl_ops<ORDER, T, sizeof(atomic128_t)>
        {
            typedef operations<T, sizeof(atomic128_t)>   std_ops   ;
            typedef typename std_ops::operand_type      operand_type    ;
            typedef typename std_ops::atomic_type       atomic_type     ;

            static const unsigned int operand_size = std_ops::operand_size  ;

            template <typename OTHER_ORDER>
            struct rebind_order {
                typedef tmpl_ops<OTHER_ORDER, T, sizeof(T)> other   ;
            };

            // Mandatory atomic primitives

            static inline T load( T volatile const * pAddr )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                assert( cds::details::is_aligned<16>( pAddr )) ;
                atomic_type v = platform::load128<ORDER>( reinterpret_cast< atomic_type volatile const *>( pAddr ) )    ;
                return *(( T *) &v) ;
#           else
                return std_ops::load( pAddr, ORDER::order )    ;
#           endif
            }

            static inline T& load( T& dest, T volatile const * pAddr )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                assert( cds::details::is_aligned<16>( pAddr )) ;
                *((atomic_type *) &dest) = platform::load128<ORDER>( reinterpret_cast< atomic_type volatile const *>( pAddr ) )    ;
                return dest ;
#           else
                return std_ops::load( dest, pAddr, ORDER::order )    ;
#           endif
            }

            static inline void store( T volatile * pAddr, const T& val )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                assert( cds::details::is_aligned<16>( pAddr )) ;
                platform::store128<ORDER>( reinterpret_cast< atomic_type volatile *>( pAddr ), atomic_cast<atomic_type>( val ) )    ;
#           else
                return std_ops::store( pAddr, val, ORDER::order )    ;
#           endif
            }

            static inline bool cas( T volatile * pDest, const T& expected, const T& desired )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                assert( cds::details::is_aligned<16>( pDest )) ;
                return platform::cas128<ORDER>(
                    reinterpret_cast< atomic_type volatile *>( pDest ),
                    atomic_cast<atomic_type>( expected ),
                    atomic_cast<atomic_type>( desired )
                )   ;
#           else
                return std_ops::cas( pDest, expected, desired, ORDER::order, membar_relaxed::order )    ;
#           endif
            }

            static inline T vcas( T volatile * pDest, const T& expected, const T& desired )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
                CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                assert( cds::details::is_aligned<16>( pDest )) ;
                return (T) platform::vcas128<ORDER>(
                    reinterpret_cast< atomic_type volatile *>( pDest ),
                    atomic_cast<atomic_type>( expected ),
                    atomic_cast<atomic_type>( desired )
                )   ;
#           else
                return std_ops::vcas( pDest, expected, desired, ORDER::order, membar_relaxed::order )    ;
#           endif
            }

            // Optional atomic primitives (can be emulated by CAS)

            static inline T xchg( T volatile * pDest, const T& val )
            {
#           ifdef CDS_ATOMIC_TEMPLATE_DEFINED
#               ifdef CDS_xchg128_defined
                    // Processor has native xchg implementation
                    CDS_STATIC_ASSERT( sizeof(T) == sizeof(atomic_type) )  ;
                    assert( cds::details::is_aligned<16>( pDest )) ;
                    return (T) platform::xchg128<ORDER>( reinterpret_cast< atomic_type volatile *>( pDest ), atomic_cast<atomic_type>( val ) ) ;
#               else
                    // CAS-based emulation
                    operand_type cur ;
                    do {
                        cur = rebind_order<membar_relaxed>::other::load( pDest )  ;
                    } while ( !cas( pDest, cur, val ))    ;
                    return cur  ;
#               endif
#           else
                return std_ops::xchg( pDest, val, ORDER::order )    ;
#           endif
            }
        };
#endif  // #ifdef CDS_128bit_atomic_defined

    }   // namespace details
    //@endcond

    /// Memory fence of type \p ORDER
    template <typename ORDER>
    static inline void fence()
    {
#   ifdef CDS_ATOMIC_TEMPLATE_DEFINED
        platform::fence<ORDER>()    ;
#   else
        platform::fence( ORDER::order )    ;
#   endif
    }

    /// Atomic load value of address \p p with explicit memory order
    template <typename ORDER, typename T>
    static inline T load( T volatile const * p )
    {
        return details::tmpl_ops<ORDER,T,sizeof(T)>::load( p )    ;
    }

    /// Atomic load value of address \p p with explicit memory order
    template <typename ORDER, typename T>
    static inline T& load( T& dest, T volatile const * p )
    {
        return details::tmpl_ops<ORDER,T,sizeof(T)>::load( dest, p )    ;
    }

    /// Atomic store a value \p val to memory address \p p with explicit memory order
    template <typename ORDER, typename T>
    static inline void store( T volatile * p, const T val )
    {
        details::tmpl_ops<ORDER,T,sizeof(T)>::store( p, val )    ;
    }

    /// Atomic exchange
    template <typename ORDER, typename T>
    static inline T exchange( T volatile * p, const T val )
    {
        return details::tmpl_ops<ORDER,T,sizeof(T)>::xchg( p, val )    ;
    }

    /// Atomic compare and swap
    template <typename SUCCESS_ORDER, typename T>
    static inline bool cas( T volatile * p, const T expected, const T desired )
    {
        return details::tmpl_ops<SUCCESS_ORDER,T,sizeof(T)>::cas( p, expected, desired )    ;
    }

    /// Atomic compare and swap, returns current value
    template <typename SUCCESS_ORDER, typename T>
    static inline T vcas( T volatile * p, const T expected, const T desired )
    {
        return details::tmpl_ops<SUCCESS_ORDER,T,sizeof(T)>::vcas( p, expected, desired )    ;
    }

    /// Atomic fetch and add
    template <typename ORDER, typename T>
    static inline T xadd( T volatile * p, const T val )
    {
        return details::tmpl_ops<ORDER,T,sizeof(T)>::xadd( p, val )    ;
    }

    /// Atomic post-increment
    template <typename ORDER, typename T>
    static inline T inc( T volatile * p )
    {
        return details::tmpl_ops<ORDER,T,sizeof(T)>::inc( p )    ;
    }

    /// Atomic post-decrement
    template <typename ORDER, typename T>
    static inline T dec( T volatile * p )
    {
        return details::tmpl_ops<ORDER,T,sizeof(T)>::dec( p )    ;
    }

    /// Atomic bitwise and
    template <typename ORDER, typename T>
    static inline T bitwise_and( T volatile * p, const T val )
    {
        return details::tmpl_ops<ORDER,T,sizeof(T)>::bitwise_and( p, val )    ;
    }

    /// Atomic bitwise or
    template <typename ORDER, typename T>
    static inline T bitwise_or( T volatile * p, const T val )
    {
        return details::tmpl_ops<ORDER,T,sizeof(T)>::bitwise_or( p, val )    ;
    }

    /// Atomic bitwise xor
    template <typename ORDER, typename T>
    static inline T bitwise_xor( T volatile * p, const T val )
    {
        return details::tmpl_ops<ORDER,T,sizeof(T)>::bitwise_xor( p, val )    ;
    }

}}  // namespace cds::atomics

#endif  // #ifndef __CDS_ATOMIC_ATOMIC_TEMPLATE_H
