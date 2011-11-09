/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_DETAILS_MARKPTR_H
#define __CDS_DETAILS_MARKPTR_H

/*
    Filename: markptr.h
    Created 2007.05.16 by Maxim.Khiszinsky

    Description:
        marked_ptr<T> - it is a pointer that lowest bits is used for bit flag storing

    Editions:
        2007.05.16  Maxim.Khiszinsky    Created
*/

#include <cds/atomic.h>

namespace cds {
    namespace details {

    //@cond
        template <int MASK, typename T>
        static inline int isMarkedBit( T * p )
        {
            return reinterpret_cast<uptr_atomic_t>( p ) & MASK  ;
        }

        template <int MASK, typename T>
        static inline T * markBit( T * p )
        {
            return reinterpret_cast< T *>( reinterpret_cast<uptr_atomic_t>( p ) | MASK );
        }

        template <int MASK, typename T>
        static inline T * unmarkBit( T * p )
        {
            return reinterpret_cast< T *>( reinterpret_cast<uptr_atomic_t>( p ) & ~MASK );
        }

        template <int MASK, typename T>
        static inline T * setMarkBit( T * p, bool bMark )
        {
            return bMark ? markBit<MASK>( p ) : unmarkBit<MASK>( p );
        }

        template <int MASK, typename T>
        static inline uptr_atomic_t getMarkBits( T * p )
        {
            return reinterpret_cast<uptr_atomic_t>( p ) & MASK ;
        }

        template <int MASK, typename T>
        static inline T * setMarkBits( T * p, uptr_atomic_t nBits )
        {
            assert( ( nBits & ~MASK ) == 0 )    ;
            return reinterpret_cast< T *>( (reinterpret_cast<uptr_atomic_t>( p ) & ~MASK) | nBits  ) ;
        }

        template <typename T>
        static inline int isMarkedBit( T * p )
        {
            return isMarkedBit<1>( p );
        }

        template <typename T>
        static inline int isMarkedBit( T * p, uptr_atomic_t nMask )
        {
            return (int) (reinterpret_cast<uptr_atomic_t>( p ) & nMask)    ;
        }

        template <typename T>
        static inline T * markBit( T * p )
        {
            return markBit<1>( p );
        }
        template <typename T>
        static inline T * markBit( T * p, uptr_atomic_t nMask )
        {
            return reinterpret_cast< T *>( reinterpret_cast<uptr_atomic_t>( p ) | nMask );
        }

        template <typename T>
        static inline T * unmarkBit( T * p )
        {
            return unmarkBit<1>( p );
        }
        template <typename T>
        static inline T * unmarkBit( T * p, uptr_atomic_t nMask )
        {
            return reinterpret_cast< T *>( reinterpret_cast<uptr_atomic_t>( p ) & ~nMask );
        }

        template <typename T>
        static inline T * setMarkBit( T * p, bool bMark )
        {
            return setMarkBit<1>( p, bMark );
        }

        template <typename T>
        static inline uptr_atomic_t getMarkBits( T * p )
        {
            return getMarkBits<1>( p );
        }
    //@endcond

        /// Generic marked pointer
        /**
            Template parameters:
                \par \p T data type
                \par \p BITS - bit mask. These bits is used as bit-field in \p T*.

            For 32-bit platforms, the addresses are aligned by 4-byte boundary. Therefore, the lowest 2 bit of the pointer is zero
            and may be used as bit-field.

            For 64-bit platforms, the addresses have 8-byte alignment and the lowest 3 bits of the pointer may be used as
            bit-field.

            Many lock-free algorithms use this lowest unused bits as a bit-field for storing internal flags
            and operating with the pointer and its flags atomically.
        */
        template <typename T, int BITS>
        class marked_ptr
        {
            T *             m_ptr    ;   ///< Marked pointer value together with the flag.

        public:
            typedef T       value_type      ;   ///< type of value the class points to
            typedef T *     pointer_type    ;   ///< type of pointer

        public:
            /// Constructs null marked pointer. The flag is cleared.
            CDS_CONSTEXPR marked_ptr() : m_ptr( NULL ) {}

            /// Constructs marked pointer with \p ptr value. The least bit of \p ptr is the flag.
            explicit marked_ptr( T * ptr ) : m_ptr( ptr ) {}

            /// Constructs marked pointer with \p ptr value and \p bMark flag.
            marked_ptr( T * ptr, int nMask ) : m_ptr( details::setMarkBits<BITS>( ptr, nMask ))
            {}

            /// Copy constructor
            marked_ptr( const marked_ptr<T,BITS>& src ): m_ptr( src.m_ptr )
            {}

            /// Returns the pointer stored in marked pointer
            T *        ptr()                { return details::unmarkBit<BITS>( m_ptr ) ;            }
            /// Returns the pointer stored in marked pointer
            T *        ptr() const            { return details::unmarkBit<BITS>( m_ptr ) ;            }
            /// Returns the raw value of marked pointer (pointer and the flag)
            T *        all()                { return m_ptr;                                    }
            /// Returns the raw value of marked pointer (pointer and the flag)
            T *        all() const    { return m_ptr;                                            }
            //@{
            /// Returns reference to the raw value of marked pointer
            T * volatile &          ref()                 { return m_ptr;                }
            T * volatile const &    ref() const           { return m_ptr;                }
            T * volatile &          ref() volatile        { return m_ptr;                }
            T * volatile const &    ref() volatile const  { return m_ptr;                }
            //@}

            /// Checks if the mark of pointer is set byp nMask
            int    isMarked( int nMask ) const    { return details::isMarkedBit( m_ptr, nMask ) != 0 ;    }
            /// Sets mark of pointer by \p nMask
            T *        mark( int nMask )
            {
                assert( (nMask & ~BITS) == 0 )  ;
                return m_ptr = details::markBit( m_ptr, nMask );
            }
            /// Clears mark of pointer by \p nMask
            T *        unmark( int nMask )
            {
                assert( (nMask & ~BITS) == 0 )  ;
                return m_ptr = details::unmarkBit( m_ptr, nMask );
            }
            /// Checks if the pointer is null. The mark bit is not take into account.
            bool    isNull() const        { return ptr() == NULL ;                        }
            /// Analogue of \ref isNull
            bool    isEmpty() const        { return isNull();                              }

            /// Returns the mark flags extracted from pointer
            uptr_atomic_t bits()
            {
                return details::getMarkBits<BITS>( m_ptr ) ;
            }

            /// Sets marked pointer and its mark bits
            void    set( T * p, uptr_atomic_t nMask )
            {
                m_ptr = details::setMarkBits<BITS>( p, nMask );
            }

            /// Assigns the \p ptr value to marked pointer. The flag sets according to last bit of \p ptr
            T *        operator = ( T * ptr )                    { return m_ptr = ptr ;                                }

            /// Copy assignment
            marked_ptr<T,BITS>& operator = ( const marked_ptr<T,BITS>& src ) { m_ptr = src.m_ptr; return *this;    }

            /// Returns the pointer with flag cleared. Analogue of \ref ptr() call
            T * operator ->()                                { return ptr();                                        }

            /// Returns the const pointer with flag cleared. Analogue of \ref ptr() call
            const T * operator ->() const                    { return ptr();                                        }

            //@{
            /// Loads the marked pointer atomically
            marked_ptr<T, BITS> load(memory_order order) const
            {
                return  marked_ptr<T, BITS>( atomics::load( &m_ptr, order ))   ;
            }
            marked_ptr<T, BITS> load(memory_order order) const volatile
            {
                return  marked_ptr<T, BITS>( atomics::load( &m_ptr, order ))   ;
            }
            template <typename ORDER>
            marked_ptr<T, BITS> load() const
            {
                return  marked_ptr<T, BITS>( atomics::load<ORDER>( &m_ptr ))   ;
            }
            template <typename ORDER>
            marked_ptr<T, BITS> load() const volatile
            {
                return  marked_ptr<T, BITS>( atomics::load<ORDER>( &m_ptr ))   ;
            }
            //@}

            //@{
            /// Stores marked pointer atomically
            void store( T * p, memory_order order )
            {
                atomics::store( &m_ptr, p, order )  ;
            }
            void store( T * p, memory_order order ) volatile
            {
                atomics::store( &m_ptr, p, order )  ;
            }
            void store( T * p, int nMask, memory_order order )
            {
                atomics::store( &m_ptr, details::setMarkBits<BITS>( p, nMask ), order )  ;
            }
            void store( marked_ptr<T,BITS> p, memory_order order )
            {
                atomics::store( &m_ptr, p.all(), order )  ;
            }
            void store( T * p, int nMask, memory_order order ) volatile
            {
                atomics::store( &m_ptr, details::setMarkBits<BITS>( p, nMask ), order )  ;
            }
            void store( marked_ptr<T,BITS> p, memory_order order ) volatile
            {
                atomics::store( &m_ptr, p.all(), order )  ;
            }

            template <typename ORDER>
            void store( T * p )
            {
                atomics::store<ORDER>( &m_ptr, p )  ;
            }
            template <typename ORDER>
            void store( T * p ) volatile
            {
                atomics::store<ORDER>( &m_ptr, p )  ;
            }
            template <typename ORDER>
            void store( T * p, int nMask )
            {
                atomics::store<ORDER>( &m_ptr, details::setMarkBits<BITS>( p, nMask ) )  ;
            }
            template <typename ORDER>
            void store( marked_ptr<T,BITS> p )
            {
                atomics::store<ORDER>( &m_ptr, p.all() )  ;
            }
            template <typename ORDER>
            void store( T * p, int nMask ) volatile
            {
                atomics::store<ORDER>( &m_ptr, details::setMarkBits<BITS>( p, nMask ) )  ;
            }
            template <typename ORDER>
            void store( marked_ptr<T,BITS> p ) volatile
            {
                atomics::store<ORDER>( &m_ptr, p.all() )  ;
            }
            //@}

            //@{
            /// CAS operation on the marked pointer
            bool cas( T * pExpected, T * pDesired, memory_order success_order, memory_order failure_order)
            {
                return atomics::cas( &m_ptr, pExpected, pDesired, success_order, failure_order )   ;
            }
            bool cas( const marked_ptr<T,BITS>& pExpected, const marked_ptr<T,BITS>& pDesired, memory_order success_order, memory_order failure_order)
            {
                return atomics::cas( &m_ptr, pExpected.all(), pDesired.all(), success_order, failure_order )   ;
            }
            bool cas( T * pExpected, T * pDesired, memory_order success_order, memory_order failure_order) volatile
            {
                return atomics::cas( &m_ptr, pExpected, pDesired, success_order, failure_order )   ;
            }
            bool cas( const marked_ptr<T,BITS>& pExpected, const marked_ptr<T,BITS>& pDesired, memory_order success_order, memory_order failure_order) volatile
            {
                return atomics::cas( &m_ptr, pExpected.all(), pDesired.all(), success_order, failure_order )   ;
            }
            template <typename SUCCESS_ORDER>
            bool cas( T * pExpected, T * pDesired)
            {
                return atomics::cas<SUCCESS_ORDER>( &m_ptr, pExpected, pDesired )   ;
            }
            template <typename SUCCESS_ORDER>
            bool cas( const marked_ptr<T,BITS>& pExpected, const marked_ptr<T,BITS>& pDesired)
            {
                return atomics::cas<SUCCESS_ORDER>( &m_ptr, pExpected.all(), pDesired.all())   ;
            }
            template <typename SUCCESS_ORDER>
            bool cas( T * pExpected, T * pDesired ) volatile
            {
                return atomics::cas<SUCCESS_ORDER>( &m_ptr, pExpected, pDesired )   ;
            }
            template <typename SUCCESS_ORDER>
            bool cas( const marked_ptr<T,BITS>& pExpected, const marked_ptr<T,BITS>& pDesired ) volatile
            {
                return atomics::cas<SUCCESS_ORDER>( &m_ptr, pExpected.all(), pDesired.all() )   ;
            }
            //@}

            //@{
            /// VCAS operation on the marked pointer
            marked_ptr<T, BITS> vcas( T * pExpected, T * pDesired, memory_order success_order, memory_order failure_order)
            {
                return marked_ptr<T, BITS>( atomics::vcas( &m_ptr, pExpected, pDesired, success_order, failure_order ))    ;
            }
            marked_ptr<T, BITS> vcas( const marked_ptr<T,BITS>& pExpected, const marked_ptr<T,BITS>& pDesired, memory_order success_order, memory_order failure_order)
            {
                return marked_ptr<T,BITS>( atomics::vcas( &m_ptr, pExpected.all(), pDesired.all(), success_order, failure_order ))    ;
            }
            marked_ptr<T,BITS> vcas( T * pExpected, T * pDesired, memory_order success_order, memory_order failure_order) volatile
            {
                return marked_ptr<T,BITS>( atomics::vcas( &m_ptr, pExpected, pDesired, success_order, failure_order ))    ;
            }
            marked_ptr<T,BITS> vcas( const marked_ptr<T,BITS>& pExpected, const marked_ptr<T,BITS>& pDesired, memory_order success_order, memory_order failure_order) volatile
            {
                return marked_ptr<T,BITS>( atomics::vcas( &m_ptr, pExpected.all(), pDesired.all(), success_order, failure_order ))    ;
            }

            template <typename SUCCESS_ORDER>
            marked_ptr<T,BITS> vcas( T * pExpected, T * pDesired )
            {
                return marked_ptr<T,BITS>( atomics::vcas<SUCCESS_ORDER>( &m_ptr, pExpected, pDesired ))    ;
            }
            template <typename SUCCESS_ORDER>
            marked_ptr<T,BITS> vcas( const marked_ptr<T,BITS>& pExpected, const marked_ptr<T,BITS>& pDesired)
            {
                return marked_ptr<T,BITS>( atomics::vcas<SUCCESS_ORDER>( &m_ptr, pExpected.all(), pDesired.all() ))    ;
            }
            template <typename SUCCESS_ORDER>
            marked_ptr<T,BITS> vcas( T * pExpected, T * pDesired ) volatile
            {
                return marked_ptr<T,BITS>( atomics::vcas<SUCCESS_ORDER>( &m_ptr, pExpected, pDesired ))    ;
            }
            template <typename SUCCESS_ORDER>
            marked_ptr<T,BITS> vcas( const marked_ptr<T,BITS>& pExpected, const marked_ptr<T,BITS>& pDesired ) volatile
            {
                return marked_ptr<T,BITS>( atomics::vcas<SUCCESS_ORDER>( &m_ptr, pExpected.all(), pDesired.all() ))    ;
            }
            //@}

            //@{
            /// Atomic exchange marked pointer
            marked_ptr<T,BITS> xchg( T * pVal, memory_order order )
            {
                return marked_ptr<T,BITS>( atomics::exchange( &m_ptr, pVal, order ))    ;
            }
            marked_ptr<T,BITS> xchg( T * pVal, int nMask, memory_order order )
            {
                return marked_ptr<T,BITS>( atomics::exchange( &m_ptr, details::setMarkBits<BITS>( pVal, nMask ), order ))    ;
            }
            marked_ptr<T,BITS> xchg( const marked_ptr<T,BITS> pVal, memory_order order )
            {
                return marked_ptr<T,BITS>( atomics::exchange( &m_ptr, pVal.all(), order ))    ;
            }
            marked_ptr<T,BITS> xchg( T * pVal, memory_order order ) volatile
            {
                return marked_ptr<T,BITS>( atomics::exchange( &m_ptr, pVal, order ))    ;
            }
            marked_ptr<T,BITS> xchg( T * pVal, int nMask, memory_order order ) volatile
            {
                return marked_ptr<T,BITS>( atomics::exchange( &m_ptr, details::setMarkBits<BITS>( pVal, nMask ), order ))    ;
            }
            marked_ptr<T,BITS> xchg( const marked_ptr<T,BITS> pVal, memory_order order ) volatile
            {
                return marked_ptr<T,BITS>( atomics::exchange( &m_ptr, pVal.all(), order ))    ;
            }

            template <typename ORDER>
            marked_ptr<T,BITS> xchg( T * pVal )
            {
                return marked_ptr<T,BITS>( atomics::exchange<ORDER>( &m_ptr, pVal ))    ;
            }
            template <typename ORDER>
            marked_ptr<T,BITS> xchg( T * pVal, int nMask )
            {
                return marked_ptr<T,BITS>( atomics::exchange<ORDER>( &m_ptr, details::setMarkBit<BITS>( pVal, nMask ) ))    ;
            }
            template <typename ORDER>
            marked_ptr<T,BITS> xchg( const marked_ptr<T,BITS> pVal )
            {
                return marked_ptr<T,BITS>( atomics::exchange<ORDER>( &m_ptr, pVal.all() ))    ;
            }
            template <typename ORDER>
            marked_ptr<T,BITS> xchg( T * pVal ) volatile
            {
                return marked_ptr<T,BITS>( atomics::exchange<ORDER>( &m_ptr, pVal ))    ;
            }
            template <typename ORDER>
            marked_ptr<T,BITS> xchg( T * pVal, int nMask ) volatile
            {
                return marked_ptr<T,BITS>( atomics::exchange<ORDER>( &m_ptr, details::setMarkBits<BITS>( pVal, nMask ) ))    ;
            }
            template <typename ORDER>
            marked_ptr<T,BITS> xchg( const marked_ptr<T,BITS> pVal ) volatile
            {
                return marked_ptr<T,BITS>( atomics::exchange<ORDER>( &m_ptr, pVal.all() ))    ;
            }
            //@}

            //@{
            /// Compare marked pointers. It is compared a pointer and its mark bits
            friend inline bool operator ==( const marked_ptr& p1, const marked_ptr& p2 )   { return p1.m_ptr == p2.m_ptr;  }
            friend inline bool operator ==( const marked_ptr& p1, const T * p2 )           { return p1.m_ptr == p2;        }
            friend inline bool operator ==( const T * p1, const marked_ptr& p2 )           { return p1       == p2.m_ptr;  }

            friend inline bool operator !=( const marked_ptr& p1, const marked_ptr& p2 )   { return !(p1 == p2);    }
            friend inline bool operator !=( const marked_ptr& p1, const T * p2 )           { return !(p1 == p2);    }
            friend inline bool operator !=( const T * p1, const marked_ptr& p2 )           { return !(p1 == p2);    }

            //@}
        } ;

        /// Marked pointer
        /**
            This specialization uses the lowest bit of an address as a flag.
            Usually, this flag is marked a node as logically deleted.
        */
        template <typename T>
        class marked_ptr< T, 1 >
        {
            T * volatile    m_ptr    ;   ///< Marked pointer value together with the flag.

        public:
            typedef T       value_type      ;   ///< type of value the class points to
            typedef T *     pointer_type    ;   ///< type of pointer

        public:
            /// Constructs null marked pointer. The flag is cleared.
            marked_ptr() : m_ptr( NULL ) {}

            /// Constructs marked pointer with \p ptr value. The least bit of \p ptr is the flag.
            explicit marked_ptr( T * ptr ) : m_ptr( ptr ) {}

            /// Constructs marked pointer with \p ptr value and \p bMark flag.
            marked_ptr( T * ptr, bool bMark ) : m_ptr( details::setMarkBit( ptr, bMark )) {}

            /// Copy constructor
            marked_ptr( const marked_ptr<T,1>& src ): m_ptr( src.m_ptr ) {}

            /// Returns the pointer stored in marked pointer
            T *        ptr()                { return details::unmarkBit( m_ptr ) ;            }
            /// Returns the pointer stored in marked pointer
            T *        ptr() const            { return details::unmarkBit( m_ptr ) ;            }
            /// Returns the raw value of marked pointer (pointer and the flag)
            T *        all()                { return m_ptr;                                    }
            /// Returns the raw value of marked pointer (pointer and the flag)
            T *        all() const    { return m_ptr;                                            }
            //@{
            /// Returns reference to the raw value of marked pointer
            T * volatile &          ref()                 { return m_ptr;                }
            T * volatile const &    ref() const           { return m_ptr;                }
            T * volatile &          ref() volatile        { return m_ptr;                }
            T * volatile const &    ref() volatile const  { return m_ptr;                }
            //@}

            /// Checks if the mark of pointer is set.
            bool    isMarked() const    { return details::isMarkedBit( m_ptr ) != 0 ;    }
            /// Sets mark of pointer
            T *        mark()                { return m_ptr = details::markBit( m_ptr );        }
            /// Clears mark of pointer
            T *        unmark()            { return m_ptr = details::unmarkBit( m_ptr );    }
            /// Checks if the pointer is null. The mark bit is not take into account.
            bool    isNull() const        { return ptr() == NULL ;                        }
            /// Analogue of \ref isNull
            bool    isEmpty() const        { return isNull();                              }

            /// Returns the mark flag extracted from pointer
            uptr_atomic_t bits()        { return details::getMarkBits<1>( m_ptr ) ; }

            /// Sets marked pointer to \p and its mark to \p nMask
            void    set( T * p, uptr_atomic_t nMask ) { m_ptr = details::setMarkBits<1>( p, nMask ); }

            /// Assigns the \p ptr value to marked pointer. The flag sets according to last bit of \p ptr
            T *        operator = ( T * ptr )                    { return m_ptr = ptr ;                                }

            /// Copy assignment
            marked_ptr<T,1>& operator = ( const marked_ptr<T,1>& src ) { m_ptr = src.m_ptr; return *this;        }

            /// Returns the pointer with flag cleared. Analogue of \ref ptr() call
            T * operator ->()                                { return ptr();                                        }

            /// Returns the const pointer with flag cleared. Analogue of \ref ptr() call
            const T * operator ->() const                    { return ptr();                                        }

            //@{
            /// Loads the marked pointer atomically
            marked_ptr<T, 1> load(memory_order order) const
            {
                return  marked_ptr<T, 1>( atomics::load( &m_ptr, order ))   ;
            }
            marked_ptr<T, 1> load(memory_order order) const volatile
            {
                return  marked_ptr<T, 1>( atomics::load( &m_ptr, order ))   ;
            }
            template <typename ORDER>
            marked_ptr<T, 1> load() const
            {
                return  marked_ptr<T, 1>( atomics::load<ORDER>( &m_ptr ))   ;
            }
            template <typename ORDER>
            marked_ptr<T, 1> load() const volatile
            {
                return  marked_ptr<T, 1>( atomics::load<ORDER>( &m_ptr ))   ;
            }
            //@}

            //@{
            /// Stores marked pointer atomically
            void store( T * p, memory_order order )
            {
                atomics::store( &m_ptr, p, order )  ;
            }
            void store( T * p, memory_order order ) volatile
            {
                atomics::store( &m_ptr, p, order )  ;
            }
            void store( T * p, bool bMark, memory_order order )
            {
                atomics::store( &m_ptr, details::setMarkBit( p, bMark ), order )  ;
            }
            void store( marked_ptr<T,1> p, memory_order order )
            {
                atomics::store( &m_ptr, p.all(), order )  ;
            }
            void store( T * p, bool bMark, memory_order order ) volatile
            {
                atomics::store( &m_ptr, details::setMarkBit( p, bMark ), order )  ;
            }
            void store( marked_ptr<T,1> p, memory_order order ) volatile
            {
                atomics::store( &m_ptr, p.all(), order )  ;
            }

            template <typename ORDER>
            void store( T * p )
            {
                atomics::store<ORDER>( &m_ptr, p )  ;
            }
            template <typename ORDER>
            void store( T * p ) volatile
            {
                atomics::store<ORDER>( &m_ptr, p )  ;
            }
            template <typename ORDER>
            void store( T * p, bool bMark )
            {
                atomics::store<ORDER>( &m_ptr, details::setMarkBit( p, bMark ) )  ;
            }
            template <typename ORDER>
            void store( marked_ptr<T,1> p )
            {
                atomics::store<ORDER>( &m_ptr, p.all() )  ;
            }
            template <typename ORDER>
            void store( T * p, bool bMark ) volatile
            {
                atomics::store<ORDER>( &m_ptr, details::setMarkBit( p, bMark ) )  ;
            }
            template <typename ORDER>
            void store( marked_ptr<T,1> p ) volatile
            {
                atomics::store<ORDER>( &m_ptr, p.all() )  ;
            }
            //@}

            //@{
            /// CAS operation on the marked pointer
            bool cas( T * pExpected, T * pDesired, memory_order success_order, memory_order failure_order)
            {
                return atomics::cas( &m_ptr, pExpected, pDesired, success_order, failure_order )   ;
            }
            bool cas( const marked_ptr<T,1>& pExpected, const marked_ptr<T,1>& pDesired, memory_order success_order, memory_order failure_order)
            {
                return atomics::cas( &m_ptr, pExpected.all(), pDesired.all(), success_order, failure_order )   ;
            }
            bool cas( T * pExpected, T * pDesired, memory_order success_order, memory_order failure_order) volatile
            {
                return atomics::cas( &m_ptr, pExpected, pDesired, success_order, failure_order )   ;
            }
            bool cas( const marked_ptr<T,1>& pExpected, const marked_ptr<T,1>& pDesired, memory_order success_order, memory_order failure_order) volatile
            {
                return atomics::cas( &m_ptr, pExpected.all(), pDesired.all(), success_order, failure_order )   ;
            }
            template <typename SUCCESS_ORDER>
            bool cas( T * pExpected, T * pDesired)
            {
                return atomics::cas<SUCCESS_ORDER>( &m_ptr, pExpected, pDesired )   ;
            }
            template <typename SUCCESS_ORDER>
            bool cas( const marked_ptr<T,1>& pExpected, const marked_ptr<T,1>& pDesired)
            {
                return atomics::cas<SUCCESS_ORDER>( &m_ptr, pExpected.all(), pDesired.all())   ;
            }
            template <typename SUCCESS_ORDER>
            bool cas( T * pExpected, T * pDesired ) volatile
            {
                return atomics::cas<SUCCESS_ORDER>( &m_ptr, pExpected, pDesired )   ;
            }
            template <typename SUCCESS_ORDER>
            bool cas( const marked_ptr<T,1>& pExpected, const marked_ptr<T,1>& pDesired ) volatile
            {
                return atomics::cas<SUCCESS_ORDER>( &m_ptr, pExpected.all(), pDesired.all() )   ;
            }
            //@}

            //@{
            /// VCAS operation on the marked pointer
            marked_ptr<T, 1> vcas( T * pExpected, T * pDesired, memory_order success_order, memory_order failure_order)
            {
                return marked_ptr<T, 1>( atomics::vcas( &m_ptr, pExpected, pDesired, success_order, failure_order ))    ;
            }
            marked_ptr<T, 1> vcas( const marked_ptr<T, 1>& pExpected, const marked_ptr<T, 1>& pDesired, memory_order success_order, memory_order failure_order)
            {
                return marked_ptr<T, 1>( atomics::vcas( &m_ptr, pExpected.all(), pDesired.all(), success_order, failure_order ))    ;
            }
            marked_ptr<T, 1> vcas( T * pExpected, T * pDesired, memory_order success_order, memory_order failure_order) volatile
            {
                return marked_ptr<T, 1>( atomics::vcas( &m_ptr, pExpected, pDesired, success_order, failure_order ))    ;
            }
            marked_ptr<T, 1> vcas( const marked_ptr<T, 1>& pExpected, const marked_ptr<T, 1>& pDesired, memory_order success_order, memory_order failure_order) volatile
            {
                return marked_ptr<T, 1>( atomics::vcas( &m_ptr, pExpected.all(), pDesired.all(), success_order, failure_order ))    ;
            }

            template <typename SUCCESS_ORDER>
            marked_ptr<T, 1> vcas( T * pExpected, T * pDesired )
            {
                return marked_ptr<T, 1>( atomics::vcas<SUCCESS_ORDER>( &m_ptr, pExpected, pDesired ))    ;
            }
            template <typename SUCCESS_ORDER>
            marked_ptr<T, 1> vcas( const marked_ptr<T, 1>& pExpected, const marked_ptr<T, 1>& pDesired)
            {
                return marked_ptr<T, 1>( atomics::vcas<SUCCESS_ORDER>( &m_ptr, pExpected.all(), pDesired.all() ))    ;
            }
            template <typename SUCCESS_ORDER>
            marked_ptr<T, 1> vcas( T * pExpected, T * pDesired ) volatile
            {
                return marked_ptr<T, 1>( atomics::vcas<SUCCESS_ORDER>( &m_ptr, pExpected, pDesired ))    ;
            }
            template <typename SUCCESS_ORDER>
            marked_ptr<T, 1> vcas( const marked_ptr<T, 1>& pExpected, const marked_ptr<T, 1>& pDesired ) volatile
            {
                return marked_ptr<T, 1>( atomics::vcas<SUCCESS_ORDER>( &m_ptr, pExpected.all(), pDesired.all() ))    ;
            }
            //@}

            //@{
            /// Atomic exchange marked pointer
            marked_ptr<T, 1> xchg( T * pVal, memory_order order )
            {
                return marked_ptr<T, 1>( atomics::exchange( &m_ptr, pVal, order ))    ;
            }
            marked_ptr<T, 1> xchg( T * pVal, bool bMark, memory_order order )
            {
                return marked_ptr<T, 1>( atomics::exchange( &m_ptr, details::setMarkBit( pVal, bMark ), order ))    ;
            }
            marked_ptr<T, 1> xchg( const marked_ptr<T, 1> pVal, memory_order order )
            {
                return marked_ptr<T, 1>( atomics::exchange( &m_ptr, pVal.all(), order ))    ;
            }
            marked_ptr<T, 1> xchg( T * pVal, memory_order order ) volatile
            {
                return marked_ptr<T, 1>( atomics::exchange( &m_ptr, pVal, order ))    ;
            }
            marked_ptr<T, 1> xchg( T * pVal, bool bMark, memory_order order ) volatile
            {
                return marked_ptr<T, 1>( atomics::exchange( &m_ptr, details::setMarkBit( pVal, bMark ), order ))    ;
            }
            marked_ptr<T, 1> xchg( const marked_ptr<T, 1> pVal, memory_order order ) volatile
            {
                return marked_ptr<T, 1>( atomics::exchange( &m_ptr, pVal.all(), order ))    ;
            }

            template <typename ORDER>
            marked_ptr<T, 1> xchg( T * pVal )
            {
                return marked_ptr<T, 1>( atomics::exchange<ORDER>( &m_ptr, pVal ))    ;
            }
            template <typename ORDER>
            marked_ptr<T, 1> xchg( T * pVal, bool bMark )
            {
                return marked_ptr<T, 1>( atomics::exchange<ORDER>( &m_ptr, details::setMarkBit( pVal, bMark ) ))    ;
            }
            template <typename ORDER>
            marked_ptr<T, 1> xchg( const marked_ptr<T, 1> pVal )
            {
                return marked_ptr<T, 1>( atomics::exchange<ORDER>( &m_ptr, pVal.all() ))    ;
            }
            template <typename ORDER>
            marked_ptr<T, 1> xchg( T * pVal ) volatile
            {
                return marked_ptr<T, 1>( atomics::exchange<ORDER>( &m_ptr, pVal ))    ;
            }
            template <typename ORDER>
            marked_ptr<T, 1> xchg( T * pVal, bool bMark ) volatile
            {
                return marked_ptr<T, 1>( atomics::exchange<ORDER>( &m_ptr, details::setMarkBit( pVal, bMark ) ))    ;
            }
            template <typename ORDER>
            marked_ptr<T, 1> xchg( const marked_ptr<T, 1> pVal ) volatile
            {
                return marked_ptr<T, 1>( atomics::exchange<ORDER>( &m_ptr, pVal.all() ))    ;
            }
            //@}

            //@{
            /// Compare marked pointers
            friend inline bool operator ==( const marked_ptr& p1, const marked_ptr& p2 )   { return p1.m_ptr == p2.m_ptr;  }
            friend inline bool operator ==( const marked_ptr& p1, const T * p2 )           { return p1.m_ptr == p2;        }
            friend inline bool operator ==( const T * p1, const marked_ptr& p2 )           { return p1       == p2.m_ptr;  }

            friend inline bool operator !=( const marked_ptr& p1, const marked_ptr& p2 )   { return !(p1 == p2);    }
            friend inline bool operator !=( const marked_ptr& p1, const T * p2 )           { return !(p1 == p2);    }
            friend inline bool operator !=( const T * p1, const marked_ptr& p2 )           { return !(p1 == p2);    }

            //@}
        };

    }    // namespace details

    //@cond none
    namespace atomics {
        // Atomic functions for marked_ptr

        /// Atomic load
        template <typename ORDER, typename T, int BITS>
        static inline cds::details::marked_ptr<T,BITS> load( cds::details::marked_ptr<T,BITS> volatile const * p )
        {
            return cds::details::marked_ptr<T,BITS>( atomics::load<ORDER>( &( p->ref() ) )) ;
        }
        template <typename T, int BITS>
        static inline cds::details::marked_ptr<T,BITS> load( cds::details::marked_ptr<T,BITS> volatile const * p, memory_order order )
        {
            return p->load( order )    ;
        }

        /// Atomic store a value \p val to memory address \p p with explicit memory order
        template <typename ORDER, typename T, int BITS>
        static inline void store( cds::details::marked_ptr<T,BITS> volatile * p, cds::details::marked_ptr<T,BITS> val )
        {
            atomics::store<ORDER>( &(p->ref()), val.all() )   ;
        }
        template <typename ORDER, typename T, int BITS>
        static inline void store( cds::details::marked_ptr<T,BITS> volatile * p, T * val )
        {
            atomics::store<ORDER>( &(p->ref()), val )   ;
        }
        template <typename T, int BITS>
        static inline void store( cds::details::marked_ptr<T,BITS> volatile * p, cds::details::marked_ptr<T,BITS> val, memory_order order )
        {
            p->store( val, order )    ;
        }
        template <typename T, int BITS>
        static inline void store( cds::details::marked_ptr<T,BITS> volatile * p, T * val, memory_order order )
        {
            p->store( val, order )    ;
        }

        /// Atomic exchange
        template <typename ORDER, typename T, int BITS>
        static inline cds::details::marked_ptr<T,BITS> exchange(
            cds::details::marked_ptr<T,BITS> volatile * p,
            cds::details::marked_ptr<T,BITS> val )
        {
            return atomics::exchange<ORDER>( &(p->ref()), val.all() )    ;
        }
        template <typename ORDER, typename T, int BITS>
        static inline cds::details::marked_ptr<T,BITS> exchange( cds::details::marked_ptr<T,BITS> volatile * p, T * val )
        {
            return atomics::exchange<ORDER>( &(p->ref()), val )    ;
        }
        template <typename T, int BITS>
        static inline cds::details::marked_ptr<T,BITS> exchange(
            cds::details::marked_ptr<T,BITS> volatile * p,
            cds::details::marked_ptr<T,BITS> val,
            memory_order order )
        {
            return p->xchg( val, order )    ;
        }
        template <typename T, int BITS>
        static inline cds::details::marked_ptr<T,BITS> exchange(
            cds::details::marked_ptr<T,BITS> volatile * p,
            T * val,
            memory_order order )
        {
            return p->xchg( val, order )    ;
        }

        /// Atomic compare and swap
        template <typename ORDER, typename T, int BITS>
        static inline bool cas(
            cds::details::marked_ptr<T,BITS> volatile * p,
            cds::details::marked_ptr<T,BITS> expected,
            cds::details::marked_ptr<T,BITS> desired
            )
        {
            return atomics::cas<ORDER>( &(p->ref()), expected, desired )    ;
        }
        template <typename ORDER, typename T, int BITS>
        static inline bool cas( cds::details::marked_ptr<T,BITS> volatile * p, T * expected, T * desired )
        {
            return atomics::cas<ORDER>( &(p->ref()), expected, desired )    ;
        }
        template <typename T, int BITS>
        static inline bool cas(
            cds::details::marked_ptr<T,BITS> volatile * p,
            cds::details::marked_ptr<T,BITS> expected,
            cds::details::marked_ptr<T,BITS> desired,
            memory_order success_order,
            memory_order failure_order )
        {
            return p->cas( expected, desired, success_order, failure_order )    ;
        }
        template <typename T, int BITS>
        static inline bool cas(
            cds::details::marked_ptr<T,BITS> volatile * p,
            T * expected,
            T * desired,
            memory_order success_order,
            memory_order failure_order )
        {
            return p->cas( expected, desired, success_order, failure_order )    ;
        }

        /// Atomic compare and swap
        template <typename ORDER, typename T, int BITS>
        static inline cds::details::marked_ptr<T,BITS> vcas(
            cds::details::marked_ptr<T,BITS> volatile * p,
            cds::details::marked_ptr<T,BITS> expected,
            cds::details::marked_ptr<T,BITS> desired
            )
        {
            return atomics::vcas<ORDER>( &(p->ref()), expected, desired )    ;
        }
        template <typename ORDER, typename T, int BITS>
        static inline cds::details::marked_ptr<T,BITS> vcas( cds::details::marked_ptr<T,BITS> volatile * p, T * expected, T * desired )
        {
            return atomics::vcas<ORDER>( &(p->ref()), expected, desired )    ;
        }
        template <typename T, int BITS>
        static inline cds::details::marked_ptr<T,BITS> vcas(
            cds::details::marked_ptr<T,BITS> volatile * p,
            cds::details::marked_ptr<T,BITS> expected,
            cds::details::marked_ptr<T,BITS> desired,
            memory_order success_order,
            memory_order failure_order )
        {
            return p->vcas( expected, desired, success_order, failure_order )    ;
        }
        template <typename T, int BITS>
        static inline cds::details::marked_ptr<T,BITS> vcas(
            cds::details::marked_ptr<T,BITS> volatile * p,
            T * expected,
            T * desired,
            memory_order success_order,
            memory_order failure_order )
        {
            return p->vcas( expected, desired, success_order, failure_order )    ;
        }
    }   // namespace atomics
    //@endcond

}    // namespace cds

#endif // #ifndef __CDS_DETAILS_MARKPTR_H
