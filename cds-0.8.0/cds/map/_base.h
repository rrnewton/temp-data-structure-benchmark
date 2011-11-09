/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_MAP__BASE_H
#define __CDS_MAP__BASE_H

/*
    Filename: map/_base.h

    Description:
        Base declarations for maps


    Editions:
        2008.10.06    Maxim.Khiszinsky    Created
*/

#include <cds/details/defs.h>

#include <cds/details/void_selector.h>
#include <cds/details/comparator.h>
#include <cds/atomic.h>

#include <boost/functional/hash.hpp>

/**\page map_common_interface Map common interface

    The implementations of map are divided into two group. The first group is
    GC-based maps that supports item erasing. We calls that list as deletable map.
    The second group is based on cds::gc::no_gc that means the map is persistent
    and it does not support item erasing.
    The interface for persistent and deletable map is slightly different.

    Preconditions for template parameters:
        \li \p VALUE should be default-constructible

    Any map implementation in CDS library supports the following interface:

    \par Stable interface

\member{insert}
\code
    bool insert (const KEY &key, const VALUE &val)
\endcode
        Insert new item (\a key, \a val) into the map
        \param key a key
        \param val a value
    \return \c true if insertion succeeded, \c false otherwise (i.e. the \a key already in the map)

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
        Ensures that the \p key exists in map, changes data of existing item to \p val.
        \param key the key
        \param val the value 
        \param func the functor making change of item's value
    \details The operation performs inserting or changing data with lock-free manner.

        If \p key exists in the map then the function changes the value of the item pointed by \p key to \p val.
        Changing is performed by calling the functor \p func with signature:
        \code
        struct ensure_functor {
            void operator()( VALUE& itemValue, const T& val, bool bNew ) ;
        };
        \endcode
        where parameters are:
            \li \p itemValue is the reference to the map's item pointed by key \p key
            \li \p val is the value passed to \p ensure member function
            \li \p bNew equals \p true if a new item has been added for \p key, and 
                \p false if \p key is found.
        The functor \p func must guarantee that during changing the value \p itemValue no other changes 
        or reads could be made on this map item by concurrent threads. 

        If \p key is not in the map then \p ensure add it like \p insert.

    \return \c std::pair<bool, bool> where \c first is \c true if operation is successfull,
        \c second is \c true if new item has been added or \c false if the item with \a key already in the map.

\member{emplace}
\code
    template <typename T, typename FUNC>
    bool emplace( const KEY& key, const T& val, FUNC func )
\endcode
    Emplaces the value of key \p key with new value \p val
    \param key a key to find
    \param val new value
    \param func functor to change the item's value of key \p key
    \details The operation changes the value (or a part of its) of key \p key to new \p val.
        The functor \p func has the purpose like \p func argument of \ref ensure member function. 
        The signature of \p func is
        \code
        struct emplace_functor {
            void operator()( VALUE& itemValue, const T& val ) ;
        };
        \endcode
        The first argument \p itemValue of the functor \p func is the reference
        to the map's item pointed by \p key. The second argument \p val is the value passed
        to \p emplace member function. \p func must guarantee that during changing
        key's value no other changes could be made on this map's item by concurrent thread.
        The map only guarantees that the item found by key cannot be deleted while \p func worked.

        If \p key is not found in the map, then \p func is not called.

    \return The function returns \p true if the \p key exists in the map, \p false otherwise

\member{erase}
\code
    bool erase (const KEY &key)
\endcode
        Erases a key \a key from the map
        \param key the key for deleting
        \return \c true if key \a key found and deleted from the map, \c false if the \a key is not found in the map
        \note The \c erase method is applicable for non-persistent implementation only. Persistent
        implementations (based on cds::gc::no_gc) don't support \p erase.

\member{erase}
\code
    template <typename T, typename FUNC>
    bool erase( const key_type& key, T& dest, FUNC func ) ;
\endcode
    This function is designed to remove a list item. 
        \param key the key for deleting
        \param dest  argument for \p func
        \param func the functor called before deleting item found.
    Before removing the item found by \p key, the functor \p func is called. The functor has the following signature:
        \code
        struct erase_functor {
            void operator()( T& dest, VALUE& itemValue ) ;
        };
        \endcode
        where \p itemValue is the item to erase. The functor may change \p itemValue; for example, 
        it may produce a preliminary cleaning of \p itemValue, or it may save some data from \p itemValue
        to \p dest.
    \note The \c erase method is applicable for non-persistent implementation only. Persistent
        implementations (based on cds::gc::no_gc) don't support \p erase.


\member{find}
\code
    bool find (const KEY &key)
\endcode
        Find a key \c key in the map
        \param key a key searching
        \return \c true if \a key found, \c false otherwise

\member{find}
\code
    template <typename T, typename FUNC>
    bool find (const KEY &key, T &data, FUNC func)
\endcode
        Find key in the map. If \p key is found returns its data in \a data parameter.
        \param key a key
        \param val a value found
        \param func the user-defined functor to return item's value
    \return \c true if \a key found, \c false otherwise
    \details If \p key found the functor \p func is called with parameters:
        \code
        struct find_functor {
            void operator ()( T& data, const VALUE& itemValue )
        };
        \endcode
        where \p itemValue is the item found by \p key. The functor copies the item's value 
        \p itemValue or its part to \p data. The map guarantees only that the item found 
        cannot be deleted while \p func works. The functor \p func should take into account 
        that concurrent threads may change the item value.

\member{empty}
\code
    bool empty () const
\endcode
        Checks if the map is empty.
        \return \c true if the map is empty, \c false otherwise

\member{clear}
\code
    void clear ()
\endcode
        Clears the map. For many map implementation this function is not thread safe.

\member{size}
\code
    size_t size() const
\endcode
        Returns item count.
        Note that \c size()==0 is not equal to \c empty()==true because of lock-free nature of
        the algorithms implemented in CDS library. The value returned is approximate estimation of map's item count.
        To check whether the map is empty you should use \c empty() method.
*/
namespace cds {

    /// Map implementations
    /**
        Any implementation of the map in \p CDS library is derived from class map_tag. It means that
            \code
                boost::type_traits::is_base_of< cds::map::map_tag, L>::value
            \endcode
        is \p true iff L is an implementation of the map.

        The common interface of the map implementation see \ref map_common_interface.
    */
    namespace map {

        /// Key traits
        template <class KEY>
        struct key_traits {
            typedef KEY                                     key_type        ;    ///< key type
            typedef cds::details::Comparator< KEY >         key_comparator  ;    ///< Key comparator type (binary predicate)

            typedef boost::hash< KEY >                      hash_functor    ;    ///< Hasher (unary functor)
            typedef typename hash_functor::result_type      hash_type       ;    ///< Type of hash value
            typedef cds::details::Comparator< hash_type >   hash_comparator ;    ///< Comparator for hash values (binary functor)
        }    ;

        /// Value traits
        template <class T>
        struct value_traits {
            typedef T                                value_type    ;    ///< value type
        };

        /// Pair (key, value) traits
        template <class KEY, class VALUE>
        struct pair_traits {
            typedef map::key_traits<KEY>        key_traits      ;    ///< Key traits type
            typedef map::value_traits<VALUE>    value_traits    ;    ///< Value traits type
        };

        /// Empty hash map statistics
        struct empty_statistics {};

        /// Generic map traits
        /**
            Many typedefs in this struct is \p void. The map implementation selects appropriate
            default type if corresponding MapTraits typedef is \p void. cds::details::void_selector is useful for this purpose.
        */
        struct type_traits {
            typedef void                bucket_type    ;    ///< type of bucket. Usually, it is one of ordered list implementation. Default - MichaelList<cds::gc::hzp_gc>
            typedef void                item_counter_type;    ///< item counter. If void then no item counter is used (EmptyItemCouner)
            typedef empty_statistics    statistics    ;    ///< Internal statistics implementation
        };

        /// This empty class is like a marker for the map implementations.
        /**
            In CDS library, for any map implementation L the value of
            \code
                boost::type_traits::is_base_of< cds::map::map_tag, L>::value
            \endcode
            is \p true.
        */
        struct map_tag {};

        /// Base of any map implementation in the library
        /**
            \par Template parameters
                \li \p KEY        type of key stored in list
                \li \p VALUE    type of value stored in list
                \li \p PAIR_TRAITS (key, value) pair traits. The default is cds::map::pair_traits <\p KEY, \p VALUE>
                \li \p TRAITS    map traits. The default is cds::map::type_traits. Different map implementation requires different
                                traits. The cds::map::type_traits class provides generic implementation of map traits.
        */
        template <
            typename KEY,
            typename VALUE,
            typename PAIR_TRAITS = pair_traits< KEY, VALUE >,
            typename TRAITS = type_traits
        >
        struct map_base: public map_tag {
            typedef KEY        key_type    ;    ///< key type
            typedef VALUE    value_type    ;    ///< value type

            typedef typename cds::details::void_selector<
                PAIR_TRAITS,
                map::pair_traits< KEY, VALUE >
            >::type                            pair_traits;    ///< Pair traits

            typedef typename cds::details::void_selector<
                TRAITS,
                map::type_traits
            >::type                            type_traits    ;        ///< Map traits

            /// item counter type. Default is cds::atomics::empty_item_counter if TRAITS::item_counter_type type is void
            typedef typename cds::details::void_selector<
                typename type_traits::item_counter_type,
                atomics::empty_item_counter
            >::type                            item_counter_type ;

            typedef typename pair_traits::key_traits        key_traits        ;    ///< key traits
            typedef typename pair_traits::value_traits        value_traits    ;    ///< value traits type

            typedef typename key_traits::hash_functor        hash_functor    ;    ///< hash function
            typedef typename key_traits::hash_type            hash_type        ;    ///< hash function result type
            typedef typename key_traits::hash_comparator    hash_comparator    ;    ///< hash comparator
        };

    }    // namespace map
}    // namespace cds

#endif  // #ifndef __CDS_MAP__BASE_H
