/*
 * $Id: case_from.h,v 1.1 2002/07/25 12:28:26 janakj Exp $
 *
 * From header field parser macros
 */

#ifndef CASE_FROM_H
#define CASE_FROM_H


#define From_CASE             \
        hdr->type = HDR_FROM; \
        p += 4;               \
        goto dc_end


#endif
