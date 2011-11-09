/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_CONCEPT_H
#define __CDS_CONCEPT_H

#include <boost/iterator.hpp>

namespace cds {
    /// Concepts
    namespace concept {

        /// Empty type
        /**
            This typeis useful for generic programming, especially for member declaration with type choice
            by \p boost::mpl::if_ template.
        */
        struct empty_type {};

        /// Bounded container
        /**
            If a container has upper limit of item contained then it should be based on bounded_container class.
            Example of those containers: cyclic queue (cds::queue::TZCyclicQueue)
        */
        struct bounded_container {};

        /// Iterator may be called in multi-threaded environment.
        /**
            This concept means that the iterator "owns" the value pointed by it.
            While a node of concurrent data structure is pointed by the concurrent iterator
            the node cannot be physically destroyed.
            However, the node may be excluded from the data structure or marked as deleted.
        */
        struct concurrent_iterator_tag {};

        /// Iterator cannot be called in multi-threaded environment.
        /**
            This concept means that the node of concurrent data structure may be destroyed
            by other thread while non-concurrent iterator is pointed to this node. Iterators of this type of concurrency
            should be used in non-concurrent environment only and/or for debugging purposes.
        */
        struct nonconcurrent_iterator_tag {};

        /// Base class for non-concurrent forward iterator of lock-free container implementation. \p T is type of data stored in container
        template <typename T>
        struct non_concurrent_iterator: public boost::iterator< std::forward_iterator_tag, T >
        {
            //@cond
            typedef    nonconcurrent_iterator_tag        concurrency_type    ;
            //@endcond
        };

        /// Base class for concurrent iterator of lock-free container implementation. \p T is type of data stored in container
        template <typename T>
        struct concurrent_iterator: public boost::iterator< std::forward_iterator_tag, T >
        {
            //@cond
            typedef    concurrent_iterator_tag        concurrency_type    ;
            //@endcond
        };


    }    // namespace concept
}    // namespace cds

#endif    // __CDS_CONCEPT_H
