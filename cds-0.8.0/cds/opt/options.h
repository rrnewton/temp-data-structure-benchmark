/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_OPT_OPTIONS_H
#define __CDS_OPT_OPTIONS_H

/*
    Framework to define template options

    Editions:
        2011.01.23 khizmax  Created
*/

#include <cds/details/defs.h>

namespace cds { 

/// Framework to define template options
/**
    There are two kind of options:
    \li \p type-option - option that determines a data type. The template argument \p TYPE of the option is a type.
    \li \p value-option - option that determines a value. The template argument \p VALUE of the option is a value.
*/
namespace opt {

    /// Predefined options value (generally, for the options that determine the data types)
    namespace v {}

    /// Type indicates that no option is being used and that the default options should be used
    struct none
    {
        //@cond
        template <class BASE> struct pack: public BASE
        {};
        //@endcond
    };

    /// Option setter specifies a tag
    /**
        Suppose, you have a struct
        \code
        struct Feature 
        {  .... };
        \endcode
        and you want that your class \p X would be derived from several \p Feature:
        \code
            class X: public Feature, public Feature
            { .... };
        \endcode
        
        How can you distinguish one \p Feature from another?
        You may use a tag option:
        \code
            template <typename TAG>
            struct Feature 
            { .... };

            class tag_a;
            class tag_b;
            class X: public Feature< tag_a >, public Feature< tag_b >
            { .... };
        \endcode
        Now you can distinguish one \p Feature from another:
        \code
            X x ;
            Feature<tag_a>& fa = static_cast< Feature<tag_a> >( x ) ;
            Feature<tag_b>& fb = static_cast< Feature<tag_b> >( x ) ;
        \endcode

        \p tag option setter allows you to do things like this for an option-centric approach:
        \code
        template <typename ...OPTIONS>
        struct Feature 
        { .... };

        class tag_a;
        class tag_b;
        class X: public Feature< tag<tag_a> >, public Feature< tag<tag_b> >
        { .... };
        \endcode

        This option setter is widely used in cds::intrusive containers to distinguish 
        between different intrusive part of container's node.

        An incomplete type can serve as a \p TAG.
    */
    template <typename TAG>
    struct tag {
        //@cond
        template<class BASE> struct pack: public BASE 
        {
            typedef TAG tag ;
        };
        //@endcond
    };

    /// Option setter specifies lock class [type-option]
    /**
        Specification of the \p TYPE class is:
        \code
        struct Lock {
            void lock() ;
            void unlock() ;
        };
        \endcode
    */
    template <typename TYPE>
    struct lock_type {
        //@cond
        template<class BASE> struct pack: public BASE 
        {
            typedef TYPE lock_type ;
        };
        //@endcond
    };

    /// Back-off strategy option setter
    /**
        Back-off strategy used in some algorithm.
        See cds::backoff namespace for back-off explanation and supported interface.
    */
    template <typename TYPE>
    struct back_off {
        //@cond
        template <class BASE> struct pack: public BASE
        {
            typedef TYPE back_off   ;
        };
        //@endcond
    };

    /// Garbage collecting schema option setter
    /**
        Possible values of \p GC template parameter are:
        - cds::gc::hzp::GC - Hazard Pointer garbage collector
        - cds::gc::hrc::GC - Gidenstam's garbage collector
        - cds::gc::ptb::GC - Pass-the-Buck garbage collector
        - cds::gc::none::GC - No garbage collector
    */
    template <typename GC>
    struct gc {
        //@cond
        template <class BASE> struct pack: public BASE
        {
            typedef GC gc   ;
        };
        //@endcond
    };

}}  // namespace cds::opt

#ifdef CDS_COMPILER_SUPPORTS_VARIADIC_TEMPLATE
#   include <cds/opt/make_options_var.h>
#else
#   include <cds/opt/make_options_std.h>
#endif

#endif  // #ifndef __CDS_OPT_OPTIONS_H
