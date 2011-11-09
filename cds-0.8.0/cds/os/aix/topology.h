/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_OS_AIX_TOPOLOGY_H
#define __CDS_OS_AIX_TOPOLOGY_H

#include <cds/os/details/fake_topology.h>

#include <unistd.h>

#ifdef _AIX53
#   include <sys/processor.h>
#endif

namespace cds { namespace OS { 
    ///  IBM AIX specific wrappers
    namespace Aix {

        /// System topology
        /**
            The implementation assumes that the processor IDs are in numerical order 
            from 0 to N - 1, where N - count of processor in the system
        */
        struct topology: public OS::details::fake_topology
        {
        private:
            typedef OS::details::fake_topology  base_class  ;
        public:
            /// Logical processor count for the system
            static unsigned int processor_count()   
            {
                long nCount = ::sysconf(_SC_NPROCESSORS_ONLN) ;
                if ( nCount == -1 )
                    return  1   ;
                return (unsigned int) nCount    ;
            }

            /// Get current processor number
            /**
                Caveat: AIX has no "get current processor number" system call.
                The function emulates "current processor number" using thread-specific data.
            */
            static unsigned int current_processor()
            {
#           if defined(_AIX53) && _AIX53 == 1
                // AIX 5.3 has undocumented function mycpu() in sys/processor.h
                return ::mycpu()    ;
#           else
                return base_class::current_processor() ;
#           endif
            }

            /// Synonym for \ref current_processor
            static unsigned int native_current_processor()
            {
                return current_processor()  ;
            }

            //@cond
            static void init() 
            {}
            static void fini() 
            {}
            //@endcond
        };
    }   // namespace Aix

    using Aix::topology   ;
}}  // namespace cds::OS

#endif  // #ifndef __CDS_OS_AIX_TOPOLOGY_H
