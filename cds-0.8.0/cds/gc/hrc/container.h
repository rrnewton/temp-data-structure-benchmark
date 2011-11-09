/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_HRC_CONTAINER_H
#define __CDS_GC_HRC_CONTAINER_H

/*
    File: gc/hrc/container.h

    Base classes for HRC containers

    Editions:
        2008.03.30    cds    Created
*/

#include <cds/gc/hrc/hrc.h>
#include <cds/threading/model.h>
#include <cds/details/allocator.h>

namespace cds { namespace gc {
    namespace hrc {

        /// Base class for all HRC-based containers
        class Container
        {
        public:
            /// Returns Gidenstam's GC for current thread
            static ThreadGC&    getGC()
            {
                return cds::threading::getGC<hrc_gc>() ;
            }
        };

        /** Templated base class for HRC nodes

            The class defines method \ref destroy for node type \p T
            \par Template parameters:
                \li \p T Node type. Usually it is derived class
                \li \p ALLOCATOR Memory allocator class

            \par Typical use:
            \code
            template <typename T, class ALLOCATOR = CDS_DEFAULT_ALLOCATOR >
            class MyNode: public cds::gc::hrc::ContainerNodeT< MyNode, ALLOCATOR >
            {
                ...    // Declarations specific to MyNode
            protected: // Implementation of ContainerNodeT pure methods
                virtual void    cleanUp( ThreadGC * pGC )    ;
                virtual void    terminate( ThreadGC * pGC, bool bConcurrent ) ;
            } ;
            \endcode
        */
        template <typename T, class ALLOCATOR = CDS_DEFAULT_ALLOCATOR >
        class ContainerNodeT: public ContainerNode
        {
        protected:
            typedef cds::details::Allocator<T, ALLOCATOR >    node_allocator    ;    ///< Node allocator
        protected:
            /// Deletes this node
            virtual void destroy()
            {
                node_allocator alloc ;
                alloc.Delete( static_cast<T *>( this ) )    ;
            }
        };

    }    // namespace hrc
} /* namespace gc */ }    /* namespace cds */

#endif // #ifndef __CDS_GC_HRC_CONTAINER_H
