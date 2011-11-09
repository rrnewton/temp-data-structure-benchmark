/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2011

    Version 0.8.0
*/


#ifndef __CDS_GC_HZP_DETAILS_HP_INLINE_H
#define __CDS_GC_HZP_DETAILS_HP_INLINE_H

namespace cds {
    namespace gc{ namespace hzp { namespace details {

        /************************************************************************/
        /* INLINES                                                              */
        /************************************************************************/
        inline retired_vector::retired_vector( const cds::gc::hzp::GarbageCollector& HzpMgr )
            : m_arr( HzpMgr.getMaxRetiredPtrCount() ),
            m_nSize(0)
        {}

        inline HPRec::HPRec( const cds::gc::hzp::GarbageCollector& HzpMgr )
            : m_hzp( HzpMgr.getHazardPointerCount() ),
            m_arrRetired( HzpMgr )
        {}

    } } }    // namespace gc::hzp::details
}    // namespace cds


#endif // #ifndef __CDS_GC_HZP_DETAILS_HP_INLINE_H
