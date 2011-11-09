/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ATOMIC_ATOMIC_H
#define __CDS_ATOMIC_ATOMIC_H

#include <cds/atomic/atomic_int.h>

namespace cds {

    /// Cross-platform atomic primitives
    namespace atomics {

        // Atomic functions

        /// Memory fence of type \p order
        static inline void fence(memory_order order)
        {
            platform::fence( order )    ;
        }

        /// Atomic load value of address \p p with explicit memory order
        template <typename T>
        static inline T load( T volatile const * p, memory_order order )
        {
            return details::operations<T,sizeof(T)>::load( p, order )    ;
        }

        /// Atomic load value of address \p p with explicit memory order
        template <typename T>
        static inline T& load( T& dest, T volatile const * p, memory_order order )
        {
            return details::operations<T,sizeof(T)>::load( dest, p, order )    ;
        }

        /// Atomic store a value \p val to memory address \p p with explicit memory order
        template <typename T>
        static inline void store( T volatile * p, const T val, memory_order order )
        {
            details::operations<T,sizeof(T)>::store( p, val, order )    ;
        }

        /// Atomic exchange
        template <typename T>
        static inline T exchange( T volatile * p, const T val, memory_order order )
        {
            return details::operations<T,sizeof(T)>::xchg( p, val, order )    ;
        }

        /// Atomic compare and swap
        template <typename T>
        static inline bool cas( T volatile * p, const T expected, const T desired, memory_order success_order, memory_order failure_order )
        {
            return details::operations<T,sizeof(T)>::cas( p, expected, desired, success_order, failure_order )    ;
        }

        /// Atomic compare and swap, returns current value
        template <typename T>
        static inline T vcas( T volatile * p, const T expected, const T desired, memory_order success_order, memory_order failure_order )
        {
            return details::operations<T,sizeof(T)>::vcas( p, expected, desired, success_order, failure_order )    ;
        }

        /// Atomic fetch and add
        template <typename T>
        static inline T xadd( T volatile * p, const T val, memory_order order )
        {
            return details::integral_operations_selector<T,sizeof(T)>::op_impl::xadd( p, val, order )    ;
        }

        /// Atomic post-increment
        template <typename T>
        static inline T inc( T volatile * p, memory_order order )
        {
            return details::integral_operations_selector<T,sizeof(T)>::op_impl::inc( p, order )    ;
        }

        /// Atomic post-decrement
        template <typename T>
        static inline T dec( T volatile * p, memory_order order )
        {
            return details::integral_operations_selector<T,sizeof(T)>::op_impl::dec( p, order )    ;
        }

        /// Atomic bitwise and
        template <typename T>
        static inline T bitwise_and( T volatile * p, const T val, memory_order order )
        {
            return details::integral_operations_selector<T,sizeof(T)>::op_impl::bitwise_and( p, val, order )    ;
        }

        /// Atomic bitwise or
        template <typename T>
        static inline T bitwise_or( T volatile * p, const T val, memory_order order )
        {
            return details::integral_operations_selector<T,sizeof(T)>::op_impl::bitwise_or( p, val, order )    ;
        }

        /// Atomic bitwise xor
        template <typename T>
        static inline T bitwise_xor( T volatile * p, const T val, memory_order order )
        {
            return details::integral_operations_selector<T,sizeof(T)>::op_impl::bitwise_xor( p, val, order )    ;
        }

    }   // namespace atomics

    /// Atomic type
    template <typename T>
    class atomic
    {
        typedef typename atomics::details::atomic_type_selector<T>::type  native_atomic_type ;   ///< Appropriate atomic type for type T
        //@cond
        typedef atomics::details::operations<T, sizeof(native_atomic_type) >     operations  ;
        //@endcond

        T volatile m_val            ;   ///< Atomic value
    public:
        typedef T   value_type      ;   ///< Type that pointer pointed to

        /// Default ctor. No assigment is performed
        atomic() {}

        /// Assignment ctor
        explicit atomic( T v )
            : m_val(v)
        {}

        /// Read value
        T load( memory_order order ) const volatile
        {
            return operations::load( &m_val, order )  ;
        }

        /// Read pointer value, metaprogramming version
        template <typename ORDER>
        T  load() volatile const
        {
            return atomics::details::tmpl_ops<ORDER, T, sizeof(T)>::load( &m_val )  ;
        }

        /// Store value
        void store( const T v, memory_order order ) volatile
        {
            operations::store( &m_val, v, order )  ;
        }

        /// Store value, metaprogramming version
        template <typename ORDER>
        void store( const T v ) volatile
        {
            atomics::details::tmpl_ops<ORDER, T, sizeof(T)>::store( &m_val, v )  ;
        }

        /// CAS
        bool cas( const T expected, const T desired, memory_order success_order, memory_order failure_order ) volatile
        {
            return operations::cas( &m_val, expected, desired, success_order, failure_order ) ;
        }

        /// CAS, metaprogramming version
        template <typename SUCCESS_ORDER>
        bool cas( const T expected, const T desired ) volatile
        {
            return atomics::details::tmpl_ops<SUCCESS_ORDER, T, sizeof(T)>::cas( &m_val, expected, desired ) ;
        }

        /// Valued CAS
        T vcas( const T expected, const T desired, memory_order success_order, memory_order failure_order ) volatile
        {
            return operations::vcas( &m_val, expected, desired, success_order, failure_order ) ;
        }

        /// Valued CAS, metaprogramming version
        template <typename SUCCESS_ORDER>
        T vcas( const T expected, const T desired ) volatile
        {
            return atomics::details::tmpl_ops<SUCCESS_ORDER, T, sizeof(T)>::vcas( &m_val, expected, desired ) ;
        }

        /// Value exchange
        T xchg( const T v, memory_order order ) volatile
        {
            return operations::xchg( &m_val, v, order ) ;
        }

        /// Value exchange, metaprogramming version
        template <typename ORDER>
        T xchg( const T v ) volatile
        {
            return atomics::details::tmpl_ops<ORDER, T, sizeof(T)>::xchg( &m_val, v ) ;
        }
    };

    /// Atomic pointer
    template<typename T>
    class atomic<T *>
    {
        T * volatile m_ptr   ;  ///< Atomic pointer

        typedef atomics::details::operations<T *, sizeof(pointer_t)>   operations   ;   ///< Internal cross-platform implementation
    public:
        typedef T   value_type      ;   ///< Type that pointer pointed to
        typedef T * pointer_type    ;   ///< Pointer type

        /// Default ctor assigns NULL to atomic pointer
        CDS_CONSTEXPR atomic()
            : m_ptr(NULL)
        {}

        /// The ctor assigns \p to atomic pointer
        explicit atomic( T * p )
            : m_ptr(p)
        {}

        /// Read pointer value
        T * load( memory_order order ) volatile const
        {
            return operations::load( &m_ptr, order )  ;
        }

        /// Read pointer value
        template <typename ORDER>
        T * load() volatile const
        {
            return atomics::details::tmpl_ops<ORDER, T*, sizeof(pointer_t)>::load( &m_ptr )  ;
        }

        /// Store pointer value
        T * store( T * p, memory_order order ) volatile
        {
            return operations::store( &m_ptr, p, order )  ;
        }

        /// Store pointer value
        template <typename ORDER>
        T * store( T * p ) volatile
        {
            return atomics::details::tmpl_ops<ORDER, T*, sizeof(pointer_t)>::store( &m_ptr, p )  ;
        }

        /// CAS
        bool cas( T * expected, T * desired, memory_order success_order, memory_order failure_order ) volatile
        {
            return operations::cas( &m_ptr, expected, desired, success_order, failure_order ) ;
        }

        /// CAS
        template <typename SUCCESS_ORDER>
        bool cas( T * expected, T * desired ) volatile
        {
            return atomics::details::tmpl_ops<SUCCESS_ORDER, T*, sizeof(pointer_t)>::cas( &m_ptr, expected, desired ) ;
        }

        /// Valued CAS
        T * vcas( T * expected, T * desired, memory_order success_order, memory_order failure_order ) volatile
        {
            return operations::vcas( &m_ptr, expected, desired, success_order, failure_order ) ;
        }

        /// Valued CAS
        template <typename SUCCESS_ORDER>
        T * vcas( T * expected, T * desired ) volatile
        {
            return atomics::details::tmpl_ops<SUCCESS_ORDER, T*, sizeof(pointer_t)>::vcas( &m_ptr, expected, desired ) ;
        }

        /// Pointer exchange
        T * xchg( T * p, memory_order order ) volatile
        {
            return operations::xchg( &m_ptr, p, order ) ;
        }

        /// Pointer exchange
        template <typename ORDER>
        T * xchg( T * p ) volatile
        {
            return atomics::details::tmpl_ops<ORDER, T*, sizeof(pointer_t)>::xchg( &m_ptr, p ) ;
        }

        /// Returns pointer to atomic pointer
        T * volatile *  operator &()
        {
            return &m_ptr   ;
        }
    };

#define _CDS_DEFINE_ATOMIC_INT( _type )  \
    template <> \
    class atomic<_type>: public atomics::details::atomic_int<_type>  \
    {   \
        typedef atomics::details::atomic_int<_type> base_class ;  \
    public: \
        atomic() {}    \
        explicit atomic( _type v ) \
            : base_class( v )   \
        {}  \
    };

    _CDS_DEFINE_ATOMIC_INT(int)
    _CDS_DEFINE_ATOMIC_INT(unsigned int)
    _CDS_DEFINE_ATOMIC_INT(long)
    _CDS_DEFINE_ATOMIC_INT(unsigned long)
    _CDS_DEFINE_ATOMIC_INT(long long)
    _CDS_DEFINE_ATOMIC_INT(unsigned long long)


#ifdef CDS_atomic_bool_defined
    /// Atomic bool
    template <>
    class atomic<bool>: public atomics::platform::atomic_bool
    {
        typedef atomics::platform::atomic_bool   base_class  ;
    public:
        atomic()
            : base_class()
        {}

        explicit atomic( bool bVal )
            : base_class( bVal )
        {}
    }   ;
#else
    //@cond
    // Emulate atomic bool through atomic32_t
    template <>
    class atomic<bool>
    {
        atomic<atomic32_t>  m_bool  ;
    public:
        atomic()
            : m_bool( 0 )
        {}

        explicit atomic( bool bVal )
            : m_bool( bVal ? 1 : 0 )
        {}

        bool    load( memory_order order ) const volatile
        {
            return m_bool.load( order ) != 0    ;
        }

        template <typename ORDER>
        bool    load() const volatile
        {
            return m_bool.load<ORDER>() != 0    ;
        }

        void store( bool v, memory_order order ) volatile
        {
            m_bool.store( v, order )    ;
        }

        template <typename ORDER>
        void store( bool v ) volatile
        {
            m_bool.store<ORDER>( v )    ;
        }

        bool exchange(bool val, memory_order order ) volatile
        {
            return m_bool.xchg( val, order ) != 0  ;
        }

        template <typename ORDER>
        bool exchange(bool val ) volatile
        {
            return m_bool.xchg<ORDER>( val ) != 0  ;
        }

        bool cas( bool bExpected, bool bDesired, memory_order success_order, memory_order failure_order ) volatile
        {
            return m_bool.cas( bExpected, bDesired, success_order, failure_order )  ;
        }

        template <typename SUCCESS_ORDER>
        bool cas( bool bExpected, bool bDesired ) volatile
        {
            return m_bool.cas<SUCCESS_ORDER>( bExpected, bDesired )  ;
        }

        bool vcas( bool bExpected, bool bDesired, memory_order success_order, memory_order failure_order ) volatile
        {
            return m_bool.vcas( bExpected, bDesired, success_order, failure_order ) != 0    ;
        }

        template <typename SUCCESS_ORDER>
        bool vcas( bool bExpected, bool bDesired ) volatile
        {
            return m_bool.vcas<SUCCESS_ORDER>( bExpected, bDesired ) != 0    ;
        }
    };
    //@endcond
#endif

    namespace atomics {

        /// Atomic item counter
        /**
            This class is simplified interface around \ref atomic<size_t>.
            The class supports getting of current value of the counter and increment/decrement its value.
            \par Template parameters:
            \li INC_ORDER - default memory order for increment/decrement operations
            \li READ_ORDER - default memory order for \ref value() method
        */
        template < typename INC_ORDER = membar_relaxed, typename READ_ORDER = membar_relaxed >
        class item_counter
        {
            typedef atomic<size_t>  atomic_type ;   ///< atomic type used
            atomic<size_t>          m_Counter   ;   ///< Atomic item counter
        public:
            typedef typename atomic_type::unaligned_atomic_type counter_type    ;   ///< Integral item counter type (size_t)

        public:
            item_counter()
                : m_Counter(0)
            {}

            /// Returns current value of the counter
            counter_type    value(memory_order order = READ_ORDER::order) const
            {
                return m_Counter.load( order ) ;
            }

            /// Same as \ref value()
            operator counter_type() const
            {
                return value()  ;
            }

            /// Returns underlying atomic interface
            atomic_type&  getAtomic()
            {
                return m_Counter    ;
            }

            /// Returns underlying atomic interface (const)
            const atomic_type&  getAtomic() const
            {
                return m_Counter    ;
            }

            /// Increments the counter. Semantics: postincrement
            size_t inc(memory_order order = INC_ORDER::order )
            {
                return m_Counter.inc( order )   ;
            }

            /// Decrements the counter. Semantics: postdecrement
            size_t dec(memory_order order = INC_ORDER::order )
            {
                return m_Counter.dec( order ) ;
            }

            /// Preincrement
            size_t operator ++()
            {
                return inc() + 1    ;
            }
            /// Postincrement
            size_t operator ++(int)
            {
                return inc()    ;
            }

            /// Predecrement
            size_t operator --()
            {
                return dec() - 1    ;
            }
            /// Postdecrement
            size_t operator --(int)
            {
                return dec()    ;
            }

            /// Resets count to 0
            void reset(memory_order order = INC_ORDER::order)
            {
                m_Counter.store( 0, order ) ;
            }
        };

        /// Empty item counter
        /**
            This class may be used instead of \ref item_counter when you do not need full \ref item_counter interface.
            All methods of the class is empty and returns 0.

            The object of this class should not be used in data structure that behavior significantly depends on item counting
            (for example, in many hash map implementation).
        */
        class empty_item_counter {
        public:
            typedef item_counter<>::counter_type counter_type    ;  ///< Counter type
        public:
            /// Returns 0
            counter_type    value(memory_order /*order*/ = membar_relaxed::order) const
            {
                return 0    ;
            }

            /// Same as \ref value(), always returns 0.
            operator counter_type() const
            {
                return value()  ;
            }

            /// Dummy increment. Always returns 0
            size_t inc(memory_order /*order*/ = membar_relaxed::order )
            {
                return 0    ;
            }

            /// Dummy increment. Always returns 0
            size_t dec(memory_order /*order*/ = membar_relaxed::order )
            {
                return 0    ;
            }

            /// Dummy pre-increment. Always returns 0
            size_t operator ++()
            {
                return 0    ;
            }
            /// Dummy post-increment. Always returns 0
            size_t operator ++(int)
            {
                return 0    ;
            }

            /// Dummy pre-decrement. Always returns 0
            size_t operator --()
            {
                return 0    ;
            }
            /// Dummy post-decrement. Always returns 0
            size_t operator --(int)
            {
                return 0    ;
            }

            /// Dummy function
            void reset(memory_order = membar_relaxed::order)
            {}
        };

        /// Atomic event counter.
        /**
            This class uses weak memory ordering (memory_order_relaxed) and may be used for statistical purposes
        */
        class event_counter: private atomic<size_t>
        {
            typedef atomic<size_t>  base_class  ;   ///< Base class
        public:
            typedef size_t      value_type  ;       ///< Type of counter

        public:
            event_counter()
                : base_class(0)
            {}

            /// Assign operator
            size_t operator =( size_t n )
            {
                base_class::store( n, membar_relaxed::order )    ;
                return n;
            }

            /// Addition
            size_t operator +=( size_t n )
            {
                return base_class::xadd( n, membar_relaxed::order )  ;
            }

            /// Substraction
            size_t operator -=( size_t n )
            {
                return base_class::xadd( 0-n, membar_relaxed::order )  ;
            }

            /// Get current value of the counter
            operator size_t () const
            {
                return get()    ;
            }

            /// Preincrement
            size_t operator ++()
            {
                return base_class::inc( membar_relaxed::order ) + 1 ;
            }
            /// Postincrement
            size_t operator ++(int)
            {
                return base_class::inc( membar_relaxed::order ) ;
            }

            /// Predecrement
            size_t operator --()
            {
                return base_class::dec( membar_relaxed::order ) - 1 ;
            }
            /// Postdecrement
            size_t operator --(int)
            {
                return base_class::dec( membar_relaxed::order ) ;
            }

            /// Get current value of the counter
            size_t get() const
            {
                return base_class::load( membar_relaxed::order ) ;
            }
        };
    }   // namespace atomics

#ifdef CDS_128bit_atomic_defined
    using atomics::details::atomic128_t  ;
#endif

}   // namespace cds

#endif  // #ifndef __CDS_ATOMIC_ATOMIC_H
