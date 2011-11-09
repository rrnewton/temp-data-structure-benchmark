/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_OPT_MAKE_OPTIONS_STD_H
#define __CDS_OPT_MAKE_OPTIONS_STD_H

#ifndef __CDS_OPT_OPTIONS_H
#   error <cds/opt/options.h> must be included instead of <cds/opt/make_options_std.h>
#endif 

//@cond
namespace cds { namespace opt {

    template<typename OPTION_LIST, typename OPTION>
    struct do_pack
    {
        // Use "pack" member template to pack options
        typedef typename OPTION::template pack<OPTION_LIST> type;
    };

    template <
        typename DEFAULT_OPTIONS
        ,typename O1 = none
        ,typename O2 = none
        ,typename O3 = none
        ,typename O4 = none
        ,typename O5 = none
        ,typename O6 = none
        ,typename O7 = none
        ,typename O8 = none
        ,typename O9 = none
        ,typename O10 = none
        ,typename O11 = none
        ,typename O12 = none
        ,typename O13 = none
        ,typename O14 = none
        ,typename O15 = none
        ,typename O16 = none
    >
    struct make_options {
        typedef 
            typename do_pack<
                typename do_pack<
                    typename do_pack<
                        typename do_pack<
                            typename do_pack<
                                typename do_pack<
                                    typename do_pack<
                                        typename do_pack<
                                            typename do_pack<
                                                typename do_pack<
                                                    typename do_pack<
                                                        typename do_pack<
                                                            typename do_pack<
                                                                typename do_pack<
                                                                    typename do_pack< 
                                                                        typename do_pack< 
                                                                            DEFAULT_OPTIONS
                                                                            ,O16
                                                                        >::type
                                                                        ,O15
                                                                    >::type
                                                                    ,O14
                                                                >::type
                                                                ,O13
                                                            >::type
                                                            ,O12
                                                        >::type
                                                        ,O11
                                                    >::type
                                                    ,O10
                                                >::type
                                                ,O9
                                            >::type
                                            ,O8
                                        >::type
                                        ,O7
                                    >::type
                                    ,O6
                                >::type
                                ,O5
                            >::type
                            ,O4
                        >::type
                        ,O3
                    >::type
                    ,O2
                >::type
                ,O1
            >::type
        type ;
    };

}}  // namespace cds::opt
//@endcond

#endif // #ifndef __CDS_OPT_MAKE_OPTIONS_STD_H
