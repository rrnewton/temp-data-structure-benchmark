/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_OPT_MAKE_OPTIONS_VAR_H
#define __CDS_OPT_MAKE_OPTIONS_VAR_H

#ifndef __CDS_OPT_OPTIONS_H
#   error <cds/opt/options.h> must be included instead of <cds/opt/make_options_var.h>
#endif 

//@cond
namespace cds { namespace opt {

    namespace details {
        template <typename OPTION_LIST, typename OPTION>
        struct do_pack
        {
            // Use "pack" member template to pack options
            typedef typename OPTION::template pack<OPTION_LIST> type;
        };

        template <typename ...T> class typelist ;

        template <typename TYPELIST> struct typelist_head ;
        template <typename HEAD, typename ...TAIL>
        struct typelist_head< typelist<HEAD, TAIL...> > {
            typedef HEAD type   ;
        };
        template <typename HEAD>
        struct typelist_head< typelist<HEAD> > {
            typedef HEAD type   ;
        };

        template <typename TYPELIST> struct typelist_tail ;
        template <typename HEAD, typename ...TAIL>
        struct typelist_tail< typelist<HEAD, TAIL...> > {
            typedef typelist<TAIL...> type   ;
        };
        template <typename HEAD>
        struct typelist_tail< typelist<HEAD> > {
            typedef typelist<> type   ;
        };

        template <typename OPTION_LIST, typename TYPELIST>
        struct make_options_impl {
            typedef typename make_options_impl<
                typename do_pack<
                    OPTION_LIST, 
                    typename typelist_head< TYPELIST >::type
                >::type,
                typename typelist_tail<TYPELIST>::type
            >::type type ;
        };

        template <typename OPTION_LIST>
        struct make_options_impl<OPTION_LIST, typelist<> > {
            typedef OPTION_LIST type ;
        };
    }   // namespace details

    template <typename OPTION_LIST, typename... OPTIONS>
    struct make_options {
        typedef typename details::make_options_impl< OPTION_LIST, details::typelist<OPTIONS...> >::type type  ;
    };

}}  // namespace cds::opt
//@endcond

#endif // #ifndef __CDS_OPT_MAKE_OPTIONS_STD_H
