/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_REFCOUNTER_H
#define __CDS_REFCOUNTER_H

/*
    Refernce counting primitives

    Editions:
*/

#include <cds/atomic.h>

namespace cds {

    /// Simple reference counter
    /**
        Thread-safe reference counter build on atomic primitives.
        @p T is one of atomic types
    */
    template <typename T>
    class ref_counter
    {
        atomic<T>   m_nRefCount    ;        ///< The reference counter

    public:
        typedef T   ref_counter_type  ; ///< The reference counter type

    public:
        /// Construct reference counter with specified value @p initValue
        ref_counter( T initValue = 0 )
            : m_nRefCount(initValue)
        {
            CDS_STATIC_ASSERT( sizeof(T) == sizeof( atomic_t ) )    ;
        }

        /// Get current value of reference counter. The value is loaded with membar_relaxed constraint
        T    getCounter() const
        {
            return m_nRefCount.template load<membar_relaxed>() ;
        }

        /// Get current value of reference counter. \p ORDER is memory order constraint for loading
        template <typename ORDER>
        T   value() const
        {
            return m_nRefCount.template load<ORDER>()    ;
        }

        /// Current value of reference counter. The value is loaded with membar_relaxed constraint
        operator T() const       
        { 
            return getCounter(); 
        }

        /// Atomic increment
        void    inc()
        {
            m_nRefCount.template inc<membar_acquire>() ;
        }

        /// Atomic decrement. Return @a true if reference counter is 0.
        bool    dec()
        {
            return m_nRefCount.template dec<membar_release>() == 1 ;
        }

        /// Atomic increment
        void operator ++()
        {
            inc()    ;
        }

        /// Atomic decrement
        bool operator--()
        {
            return dec()    ;
        }
    };

    /// Signed reference counter
    typedef ref_counter<atomic_t>           signed_ref_counter      ;

    /// Unsigned reference counter
    typedef ref_counter<unsigned_atomic_t>  unsigned_ref_counter    ;


//@cond none
    /* RCObject
        Объект, время жизни которого определяется счетчиком ссылок на него.
        Служит базовым классом для всех классов с управлением временем жизни на основе счетчика ссылок.
        Объекты, производные от RCObject, должны создаваться ТОЛЬКО динамически (newed).

        Данный класс до конца не разработан. Реализация не совсем отвечает потребностям
    */
    class RCObject
    {
        unsigned_ref_counter    m_RefCount    ;

        RCObject( const RCObject& obj )    ;    // копирование запрещено
    public:
        RCObject()
        {}

        RCObject( bool bLocked )
        {
            if ( bLocked )
                acquire()    ;
        }

        virtual ~RCObject()
        {}

    public:    // Operations
        // Увеличение счетчика ссылок на объект
        void    acquire()        { m_RefCount.inc(); }

        // Уменьшение счетчика ссылок на объект. Возвращает false, если счетчик ссылок уменьшается до 0 и объект деаллокируется.
        bool    release()
        {
            bool bRet ;
            if ( !( bRet = m_RefCount.dec()) )
                delete this ;
            return bRet    ;
        }
    };

    /* RCptr<T>
        Указатель на объект типа T, производного от RCObject
        Класс сам управляет счетчиком ссылок: конструктор увеличивает счетчик (вызов T::acquire),
        деструктор уменьшает (вызов T::release).
    */
    template <class T>
    class RCptr {
        T *        m_ptr    ;

        void    release()
        {
            if ( m_ptr ) {
                m_ptr->release()    ;
                m_ptr = NULL        ;
            }
        }

        void    assign( T * p )
        {
            if ( p )
                p->acquire()    ;
            m_ptr = p    ;
        }

    public:
        RCptr()
            : m_ptr( NULL )
        {}

        // Увеличивает счетчик ссылок (вызов acquire)
        RCptr( T * p )
            { assign ( p )    ; }

        // Увеличивает счетчик ссылок, если bAcquire == true
        RCptr( T * p, bool bAcquire )
        {
            if ( bAcquire )
                assign( p )    ;
            else
                m_ptr = p    ;
        }

        // Уменьшает счетчик ссылок (вызов release)
        ~RCptr()
            { release() ;    }

    public:
        operator const T * () const            { return m_ptr; }
        operator T *()                        { return m_ptr; }

        const T *    operator ->() const        { return m_ptr; }
        T *            operator ->()            { return m_ptr; }
        T **        operator &()
        {
            assert( m_ptr == NULL ) ;
            return &m_ptr   ;
        }

        T *            operator =( T * p )
        {
            release()    ;
            assign( p )    ;
            return p    ;
        }

        bool        operator !() const      { return isNull(); }

    public:
        bool        isNull() const            { return m_ptr == NULL; }

        // Отменяет владение указателем на объект.
        // Возвращает указатель, которым владеет RCptr, а сам присваивает внутреннему указателю NULL.
        // Возвращаемый указатель следует освободить вызовом release() по окончании использования.
        T *            freeOwnership()
        {
            T * p = m_ptr    ;
            m_ptr = NULL    ;
            return p        ;
        }

        // Устанавливает уже acquired-значение указателя p
        void setAquired( T * p )
        {
            release()   ;
            m_ptr = p   ;
        }
    };

//@endcond
} // namespace cds

#endif    // #ifndef __CDS_REFCOUNTER_H
