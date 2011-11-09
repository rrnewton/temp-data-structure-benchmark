/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ATOMIC_ATOMIC_INT_H
#define __CDS_ATOMIC_ATOMIC_INT_H

#include <cds/atomic/atomic_template.h>

//@cond none
namespace cds { namespace atomics { namespace details {

    /// Atomic type for integers
    template <typename T>
    class atomic_int
    {
    public:
        typedef typename atomic_type_selector<T>::type              atomic_type ;   ///< Appropriate atomic type for type T
        typedef typename atomic_type_selector<T>::unaligned_type    unaligned_atomic_type ; /// unaligned atomic type for T
        typedef typename atomic_type_selector<T>::aligned_value_type aligned_value_type ;   ///< Aligned type T
        typedef typename atomic_type_selector<T>::value_type        value_type ;            ///< == T

    private:
        typedef typename integral_operations_selector<T, sizeof(atomic_type) >::op_impl     operations  ;

        template <typename ORDER>
        struct tmpl_operations: public tmpl_ops<ORDER, value_type, sizeof(value_type)>
        {};

        aligned_value_type volatile m_val   ;

    public:
        CDS_CONSTEXPR atomic_int()
            : m_val(0)  
        {}
        explicit atomic_int(T val) { m_val = val ; }

        /// Load value
        T  load( memory_order order ) const volatile
        {
            return operations::load( &m_val, order )  ;
        }

        /// Load value
        template <typename ORDER>
        T  load() const volatile
        {
            return tmpl_operations<ORDER>::load( &m_val )  ;
        }

        /// Store value
        T store( T val, memory_order order ) volatile
        {
            return operations::store( &m_val, val, order )  ;
        }

        /// Store value
        template <typename ORDER>
        T store( T val ) volatile
        {
            return tmpl_operations<ORDER>::store( &m_val, val )  ;
        }

        /// CAS
        bool cas( T expected, T desired, memory_order success_order, memory_order failure_order ) volatile
        {
            return operations::cas( &m_val, expected, desired, success_order, failure_order ) ;
        }

        /// CAS
        template <typename SUCCESS_ORDER>
        bool cas( T expected, T desired ) volatile
        {
            return tmpl_operations<SUCCESS_ORDER>::cas( &m_val, expected, desired ) ;
        }

        /// Valued CAS
        T vcas( T expected, T desired, memory_order success_order, memory_order failure_order ) volatile
        {
            return operations::vcas( &m_val, expected, desired, success_order, failure_order ) ;
        }

        /// Valued CAS
        template <typename SUCCESS_ORDER>
        T vcas( T expected, T desired ) volatile
        {
            return tmpl_operations<SUCCESS_ORDER>::vcas( &m_val, expected, desired ) ;
        }

        /// exchange
        T xchg( T val, memory_order order ) volatile
        {
            return operations::xchg( &m_val, val, order ) ;
        }

        /// exchange
        template <typename ORDER>
        T xchg( T val ) volatile
        {
            return tmpl_operations<ORDER>::xchg( &m_val, val ) ;
        }

        /// Atomic add
        T xadd( T val, memory_order order ) volatile
        {
            return operations::xadd( &m_val, val, order )  ;
        }

        /// Atomic add
        template <typename ORDER>
        T xadd( T val ) volatile
        {
            return tmpl_operations<ORDER>::xadd( &m_val, val )  ;
        }

        /// Atomic increment
        T inc( memory_order order ) volatile
        {
            return operations::inc( &m_val, order ) ;
        }

        /// Atomic increment
        template <typename ORDER>
        T inc() volatile
        {
            return tmpl_operations<ORDER>::inc( &m_val ) ;
        }

        /// Atomic decrement
        T dec( memory_order order ) volatile
        {
            return operations::dec( &m_val, order ) ;
        }

        /// Atomic decrement
        template <typename ORDER>
        T dec() volatile
        {
            return tmpl_operations<ORDER>::dec( &m_val ) ;
        }
    };

}}} // namespace cds::atomics::details
//@endcond

#endif // #ifndef __CDS_ATOMIC_ATOMIC_INT_H
