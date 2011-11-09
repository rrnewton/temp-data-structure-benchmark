/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#include <cds/os/topology.h>

#if CDS_OS_TYPE == CDS_OS_LINUX

#include <unistd.h>
#include <fstream>

namespace cds { namespace OS { namespace Linux {

    unsigned int topology::s_nProcessorCount = 0   ;

    void topology::init() 
    {
         long n = ::sysconf( _SC_NPROCESSORS_ONLN )   ;
         if ( n > 0 )
            s_nProcessorCount = n ;
         else {
            try {
                std::ifstream cpuinfo("/proc/cpuinfo");
                std::string line;

                unsigned int nProcCount = 0 ;
                while ( !cpuinfo.eof() ) {
                    std::getline(cpuinfo,line);
                    if (!line.size())
                        continue;
                    if ( line.find( "processor" ) !=0 )
                        continue;
                    ++nProcCount    ;
                }
                s_nProcessorCount = nProcCount  ;
            }
            catch ( std::exception& ex ) {
                s_nProcessorCount = 1   ;
            }
         }
    }

    void topology::fini()
    {}
}}} // namespace cds::OS::Linux

#endif  // #if CDS_OS_TYPE == CDS_OS_LINUX
