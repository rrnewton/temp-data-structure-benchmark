/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __UNIT_STACK_TYPES_H
#define __UNIT_STACK_TYPES_H

#include <cds/stack/stack_tagged.h>
#include <cds/stack/stack_hzp.h>

namespace stack {
    template <typename T>
    struct Types {
        typedef cds::stack::Stack< cds::gc::hzp_gc, T >         Stack_HP        ;
#ifdef CDS_DWORD_CAS_SUPPORTED
        typedef cds::stack::Stack< cds::gc::tagged_gc, T >      Stack_Tagged    ;
#endif

        struct Yield_Traits: public cds::stack::Traits {
            typedef cds::backoff::yield     backoff_strategy    ;
        };
        typedef cds::stack::Stack< cds::gc::hzp_gc, T, Yield_Traits >         Stack_HP_yield        ;
#ifdef CDS_DWORD_CAS_SUPPORTED
        typedef cds::stack::Stack< cds::gc::tagged_gc, T, Yield_Traits >      Stack_Tagged_yield    ;
#endif

        struct Pause_Traits: public cds::stack::Traits {
            typedef cds::backoff::pause     backoff_strategy    ;
        };
        typedef cds::stack::Stack< cds::gc::hzp_gc, T, Pause_Traits >         Stack_HP_pause        ;
#ifdef CDS_DWORD_CAS_SUPPORTED
        typedef cds::stack::Stack< cds::gc::tagged_gc, T, Pause_Traits >      Stack_Tagged_pause    ;
#endif

    };
} // namespace stack

#endif // #ifndef __UNIT_STACK_TYPES_H
