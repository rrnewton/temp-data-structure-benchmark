/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_TAGGED_GC_CONTAINER_NODE_H
#define __CDS_GC_TAGGED_GC_CONTAINER_NODE_H

/*
    Editions:
        2011.02.19  Maxim.Khiszinsky    Moved from cds/gc/tagged/gc.h to separated header
*/

#include <cds/gc/tagged/tagged_type.h>
#include <cds/details/allocator.h>      // call_dtor

#ifdef CDS_DWORD_CAS_SUPPORTED
namespace cds { namespace gc { namespace tagged {

    /// Base for tagged container node
    /**
        All container node types based on \ref tagged_gc technique must be 
        derived from this class.

        \par Container node requirements:
            \li The member function \p destroy_data should be provided. 
                This function clears old content of node's data; for example, it
                may call destructor:
            \code
                void destroy_data()
                { 
                    m_data.T::~T(); 
                }
            \endcode

            \li The member function \p construct_data should be provided.
                The function acts as constructor for the nodes popped from free-list;
                for example, it may call in-place new:
            \code
                void construct_data()               
                { 
                    new ( &m_data ) T; 
                }
                void construct_data( const T& src )  
                { 
                    new ( &m_data ) T( src ); 
                }
            \endcode
    */
    struct CDS_TAGGED_ALIGN_ATTRIBUTE ContainerNode 
    {
        typedef tagged_type< ContainerNode * >  tagged_free_ptr ;   ///< free-list next pointer type

        tagged_free_ptr     m_pNextFree ;   ///< Next item in the free-list

        /*
        /// Get pointer to node data
        T&        data()                    { return static_cast<NODE *>(this)->m_data; }

        //@cond
        void destroyData()                  { cds::details::call_dtor( &( data()) ) ; }
        void constructData()                { new ( &data() ) T; }
        void constructData(const T& src )   { new ( &data() ) T( src ); }
        //@endcond
        */
    } ;

}}} // namespace cds::gc::tagged
#endif  // #ifdef CDS_DWORD_CAS_SUPPORTED

#endif // #ifndef __CDS_GC_TAGGED_GC_CONTAINER_NODE_H
