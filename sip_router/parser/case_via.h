/*
 * $Id: case_via.h,v 1.2 2002/08/12 12:04:50 janakj Exp $
 *
 * Via header field parser macros
 */

#ifndef CASE_VIA_H
#define CASE_VIA_H


#define Via1_CASE            \
        hdr->type = HDR_VIA; \
        hdr->name.len = 3;   \
        *(p + 3) = '\0';     \
        return (p + 4)        


#define Via2_CASE            \
        hdr->type = HDR_VIA; \
        p += 4;              \
        goto dc_end


#endif /* CASE_VIA_H */
