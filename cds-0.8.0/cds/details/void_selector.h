/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_DETAILS_VOID_SELECTOR_H
#define __CDS_DETAILS_VOID_SELECTOR_H

#include <boost/type_traits/is_void.hpp>
#include <boost/mpl/if.hpp>

namespace cds {
    namespace details {

        /// Void type selector
        /**
            Returns in typedef \p type result of
            \code
            if ( T == [possibly cv-qualified ] void ) type := TVOID
            else type := TNOVOID
            \endcode

            The \p TNOVOID is optional, default is \p TNOVOID == T
        */
        template <typename T, typename TVOID, typename TNOVOID = T>
        struct void_selector {
            typedef typename boost::mpl::if_< boost::is_void<T>, TVOID, TNOVOID >::type type ;    ///< Result type
        };

    }    // namespace details
} // namespace cds

#endif // #ifndef __CDS_DETAILS_VOID_SELECTOR_H
