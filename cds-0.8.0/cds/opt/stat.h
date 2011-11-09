/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_OPT_STAT_H
#define __CDS_OPT_STAT_H

#include <cds/details/defs.h>

namespace cds { namespace opt {

    /// [type-option] Generic option setter for statisitcs
    /**
        This option sets a type to gather statistics. 
        The option is generic - no predefined type(s) is provided.
        The particular \p TYPE of statistics depends on internal structure of
        the object or class.
    */
    template <typename TYPE>
    struct stat {
        //@cond
        template <typename BASE> struct pack: public BASE
        {
            typedef TYPE stat ;
        };
        //@endcond
    };

}}  // namespace cds::opt

#endif  // #ifndef __CDS_OPT_STAT_H
