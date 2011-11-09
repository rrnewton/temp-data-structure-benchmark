/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_TAGGED_GC_IMPL_H
#define __CDS_GC_TAGGED_GC_IMPL_H

/*
    Editions:
        2011.02.19  Maxim.Khiszinsky    tagged_type and ContainerNode is moved to separate headers
        2010.02.22  Maxim.Khiszinsky    FreeList implementation is moved to separate header (details/free_list.h)
        2008.10.01  Maxim.Khiszinsky    Refactoring
        2007.03.03  Maxim.Khiszinsky    Created
*/

#include <cds/gc/tagged_gc.h>
#include <cds/gc/tagged/container_node.h>
#include <cds/gc/tagged/details/free_list.h>

namespace cds { namespace gc {

    /// Tagged pointer reclamation schema to solve ABA-problem
    /**
        Tagged pointer reclamation schema was founded by IBM for resolving ABA problem
        of CAS-based algorithms.
        Each CASable pointer in this schema consists of two part: the pointer and the tag.
        The CAS operation atomically increments the tag any time when CAS is succeeded.
        In practice, it is means that the goal platform must support double-word CAS primitive.
        Tagged pointer is sufficiently efficient technique; unfortunately, not all of modern CPU
        architecture supports double-word CAS primitive; currently, only x86 and amd64 have
        full support of double-word CAS in 32-bit and 64-bit mode.

        The macro \p CDS_DWORD_CAS_SUPPORTED is defined for the architecture that supports double-word
        CAS.

    \par Implementation notes
        It seems the tagged GC is not safe for complex data structure.

        Consider the map based on tagged GC schema. Let's item's key is std::string 
        (i.e. dynamically allocated buffer).

        Thread F is seeking key K in the map. Suppose, it is preempted at the item with key K.
        Thread D deletes key K from the map. According to tagged GC, the item with key K places
        to the free-list. Notes, when a item is moved to the free-list the item's internal data
        (the string in our case) is not destroyed.

        At the same time, the thread X inserts the key K in the map. It allocates new item from 
        GC's free-list. Since the free-list is stack-based the item popped is the item that has 
        just been deleted by the thread D. When an item is popped from the free-list, the destructor 
        of internal data of the item must be called to clean it.
        Suppose, when item's destructor is processing, the thread F resumes and compares its current item 
        with the key K. Oops! The current item for thread F is the item that is being destroyed at 
        this moment and item's key is K.  

        This case is one of typical problem of lock-free programming. For complex data structures,
        before an item can be reused we must be sure that no thread can access to it. The tagged
        GC saves us from ABA-problem but it does not solve "early reusing" problem.

        For simple data structures tagged GC is safe.
    */
    namespace tagged {

    } // namespace tagged
}} // namespace cds::gc

#endif // #ifndef __CDS_GC_TAGGED_GC_IMPL_H
