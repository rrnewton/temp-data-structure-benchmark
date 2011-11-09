/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0+
*/


#ifndef __CDS_BAG_SBAG_H
#define __CDS_BAG_SBAG_H

/*
    Sundell et. al. lock-free concurrent bag

    Editions:
        2011.10.20    Guilherme Fernandes [stallone3 at gmail dot com]    Created
*/

#include <cds/details/allocator.h>

namespace cds {
    namespace bag {

        /// Sundell et. al. lock-free concurrent bag
        /*
            Declaration of Sundell et. al. lock-free concurrent bag algorithm.
            \par Template parameters
                \li \p GC safe memory reclamation schema (garbage collector)
                \li \p T type of item stored in bag
                \li \p TRAITS class traits. The default is traits
                \li \p ALLOCATOR node allocator. The default is \ref CDS_DEFAULT_ALLOCATOR

            There are specialization for each appropriate reclamation schema \p GC.
        */
        template < typename GC,
            typename T,
            int NR_THREADS,
            class ALLOCATOR = CDS_DEFAULT_ALLOCATOR
        >
        class SBag    ;


    }    // namespace bag
}    // namespace cds

#endif // #ifndef __CDS_BAG_SBAG_H
