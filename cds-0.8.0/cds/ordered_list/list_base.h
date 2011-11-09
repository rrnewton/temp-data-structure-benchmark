/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_ORDERED_LIST_LIST_BASE_H
#define __CDS_ORDERED_LIST_LIST_BASE_H

#include <cds/details/defs.h>
#include <cds/user_setup/allocator.h>   // CDS_DEFAULT_ALLOCATOR

#include <utility>                      // std::pair
#include <cds/details/void_selector.h>
#include <cds/backoff_strategy.h>
#include <cds/details/allocator.h>        // for derived classes
#include <cds/ordered_list/details/trivial_functor.h>

/** \page ordered_list_common_interface Ordered list common interface

    The implementations of ordered list are divided into two group. The first group is
    GC-based list that supports item erasing. We calls that list as deletable list.
    The second group is based on cds::gc::no_gc that means the list is persistent
    and it does not support item erasing.
    The interface for persistent and deletable list is slightly different.

    Preconditions for template parameters:
        \li \p VALUE should be default-constructible

    Any ordered list implementation in CDS library supports the following interface:

    \par Stable interface

\member{insert}
\code
    bool insert (const KEY &key, const VALUE &val)
\endcode
        Insert new item (\a key, \a val) into the list.
        \param key a key
        \param val a value
        \return \c true if insertion succeeded, \c false otherwise (i.e. the \a key already in the list)

\member{insert}
\code
    VALUE * insert (const KEY &key)
\endcode
        [Persistent list only] Inserts new item with \a key into the list.
        \param key a key
        \return the pointer to the value of new item. If the key already exists in the list returns NULL.
        \details Recall that persistent list has concurrent nature. The pointer returned points
        to the value of list's item that may be changed concurrently by other thread. You should use
        some proper synchronization on per-value basis or atomic operations when value changing.

\member{insert}
\code
    template <typename T, typename FUNC>
    bool insert (const KEY &key, const T &val, FUNC func )
\endcode
        Insert new item (\a key, \a val) into the map
        \param key is a key
        \param val is a value
        \param func is uset-defined functor to initialize new item's value.

    \details The signature of functor \p func is:
    \code
    struct insert_functor {
        void operator()( VALUE& itemValue, const T& val ) ;
    };
    \endcode
    where
        \li \p itemValue is value of item inserted
        \li \p val is the value passed to \p insert member function
    The functor \p func must guarantee that during changing the value \p itemValue no other changes 
    or reads could be made on this map item by concurrent threads. 

    \return \c true if insertion succeeded, \c false otherwise (i.e. the \a key already in the map)

\member{ensure}
\code
    template <typename T, typename FUNC>
    std::pair< bool, bool > ensure (const KEY &key, const T &val, FUNC func)
\endcode
        Ensures that key \a key exists in list, changes data of existing item to \a val.
        \param key a key to find
        \param val a value
        \param func the functor making change of item's value
        \details The operation performs inserting or changing data with lock-free manner.

            If \p key exists in the list then changes the value of the item pointed by \p key to \p val.
            Changing is performed by calling user-defined function \p func with signature:
            \code
            void func( VALUE& itemValue, const VALUE& val, bool bNew ) ;
            \endcode
            where parameters are:
                \li \p itemValue is the reference to the map's item pointed by key \p key
                \li \p val is the value passed to \p ensure member function
                \li \p bNew equals \p true if a new item has been added for \p key, and 
                \p false if \p key is found.

            User-defined functor \p func must guarantee that during changing
            key's value no any other changes or reads could be made on this list item by concurrent threads.

            If \p key is not in list then add it.
        \return \c std::pair<bool, bool> where \c first is \c true if operation is successfull,
            \c second is \c true if new item has been added or \c false if the item with \a key already in list.

\member{emplace}
\code
    template <typename T, typename FUNC>
    bool emplace( const KEY& key, const T& val, FUNC func )
\endcode
    Emplaces the vale of key \p key with new value \p val
    \param key a key to find
    \param val new value
    \param func function to change the value of key \p key
    \details The operation changes the value (or a part of the value) of key \p key to new one \p val.
        The user-defined functor \p func has the purpose like \p func argument of \ref ensure
        member function. The signature of \p func is
        \code
        void func( VALUE& itemValue, const T& val ) ;
        \endcode
        The first argument \p itemValue of user-defined function \p func is the reference
        to the list's item pointed by key \p key. The second argument \p val is the value passed
        to \p emplace member function. User-defined function \p func must guarantee that during changing
        key's value no any other changes could be made on this list's item by a concurrent thread.
        The list guarantees only that the item found by key cannot be deleted while \p func works

    \return The function returns \p true if the key \p key exists in the list, \p false otherwise

\member{erase}
\code
    bool erase (const KEY &key)
\endcode
        [Deletable list only] Erase a key \a key from the list
        \param key a key for deleting
        \return \c true if key \a key is found and deleted from the list, \c false if the \a key is not found in the list
        \note The \c erase method is applicable for non-persistent implementation only

\member{find}
\code
    bool find (const KEY &key)
\endcode
        Find a key \c key in the list
        \param key a key finding for
        \return \c true if \a key found, \c false otherwise

\member{find}
\code
    template <typename T, typename FUNC>
    bool find (const KEY &key, T &data, FUNC func)
\endcode
        Find key in the list. If key is found returns its data in \a data parameter.
        \param key a key
        \param val a value found
        \param func the user-defined function to copy item's value
    \return \c true if \a key found, \c false otherwise
    \details If \p key found the function calls user-defined function \p func with parameters:
        \code
        void func( T& data, const VALUE& itemValue )
        \endcode
        where \p itemValue is the item found by \p key. The user-defined function
        copies the item's value \p itemValue or its part to \p data. The list guarantees only
        that the item found cannot be deleted while \p func call. The user-defined function \p func
        should take into account that concurrent threads may change the item value and
        performs some synchronization during copying.

\member{get}
\code
    VALUE * get( const KEY& key )
\endcode
        [Persistent list only] Find key in the list
        \return the pointer to the value found. If the key is not found returns NULL.
        \details Remember, that other thread may change item's value concurrently. You should use
        some proper synchronization or atomic operations on per-value basis when value changing.

\member{empty}
\code
    bool empty () const
\endcode
        Check if the list is empty.
        \return \c true if the list is empty, \c false otherwise

\member{clear}
\code
    void clear ()
\endcode
        Clears the list. For many list implementation (and for all of persistent list) this function is not thread safe.

*/

namespace cds {
    /// Ordered list implementations
    /**
        Any implementation of ordered list in \p CDS library is derived from class ordered_list_tag. It means that
            \code
                boost::type_traits::is_base_of< cds::ordered_list::ordered_list_tag, L>::value
            \endcode
        is \p true iff L is an implementation of ordered list

        Usually, the ordered list is used as a container to resolve collisions in various hash map implementations. Keys in ordered list
        are stored in sorted order.

        The common interface of ordered list implementation see \ref ordered_list_common_interface.
    */
    namespace ordered_list {

        /// Internal statistics of ordered list implementation
        struct Statistics {};

        /// Empty statistics of ordered list implementation
        struct empty_statistics {};

        /// Ordered list default type traits class.
        /**
            This class defines all helper types to support different ordered list implementations.
            You should derive yourself traits implementation from this structure.
        */
        struct type_traits
        {
            typedef cds::backoff::empty        backoff_strategy    ;    ///< Back-off strategy
            typedef void                    free_list_type      ;    ///< Free-list implementation (for tagged_gc derived classes only)
            typedef void                    lock_type           ;    ///< Lock implementation (for fine-grained lock lists only, like LazyList)
            typedef empty_statistics        statistics            ;    ///< Internal statistics implementation

            /// Node deletion function
            /**
                This functor is used internally for some implementations of ordered lists. If a list may store
                the nodes of different types then for those list the node deletion function must be provided.

                If it is \p void then use default GC-related node deletion function
            */
            typedef void                    node_deleter_functor   ;
        };

        /// Ordered list type traits selector.
        /**
            If \p TRAITS template parameter is \p void then \ref cds::ordered_list::type_traits is used
        */
        template <typename TRAITS>
        struct type_traits_selector: public TRAITS   {};

        //@cond
        template <>
        struct type_traits_selector<void>: public type_traits {};
        //@endcond

        /// This empty class is like a marker for all ordered list implementations.
        /**
            For any ordered list implementation L the value of
            \code
                boost::type_traits::is_base_of< cds::ordered_list::ordered_list_tag, L>::value
            \endcode
            is \p true.
        */
        struct ordered_list_tag {};

        /// Base of ordered list classes
        /**
            \par Template parameters
                \li \p KEY        type of key stored in list
                \li \p VALUE    type of value stored in list
                \li \p TRAITS    list traits. If \p TRAITS == void then  default implementation cds::ordered_list::type_traits is selected.

            All ordered list is derived from this class. In turn, this class derived from ordered_list_tag class.
        */
        template <typename KEY, typename VALUE, typename TRAITS>
        struct OrderedList: public ordered_list_tag
        {
            typedef        KEY        key_type    ;    ///< Key type
            typedef        VALUE    value_type    ;    ///< Data type

            /// List type traits.
            typedef     type_traits_selector<TRAITS>    type_traits ;

            typedef typename cds::details::void_selector<
                typename type_traits::statistics,
                empty_statistics
            >::type     statistics ; ///< Internal statistics implementation

            /// Get reference to internal statistics
            const statistics& getStatistics() const { return m_Statistics; }

        protected:
            //@cond
            statistics            m_Statistics ;
            //@endcond
        };

        /// Split-ordered list adapters
        /**
            This namespace contains implementation details adapting different ordered list for split-ordered list requirements.
        */
        namespace split_list {

            template <
                typename TAG,
                typename REGULAR_KEY,
                typename DUMMY_KEY,
                typename VALUE,
                typename PAIR_TRAITS = cds::map::pair_traits< REGULAR_KEY, VALUE >,
                typename TRAITS = ordered_list::type_traits,
                class ALLOCATOR = CDS_DEFAULT_ALLOCATOR
            >
            struct impl_selector    ;

        }   // namespace split_list

    }    // namespace ordered_list
}    // namespace cds

#endif // #ifndef __CDS_ORDERED_LIST_LIST_BASE_H

