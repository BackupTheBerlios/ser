/*
 * $Id: case_allo.h,v 1.2 2002/08/12 12:04:50 janakj Exp $
 *
 * Allow header field parser macros
 */

#ifndef CASE_ALLO_H
#define CASE_ALLO_H


#define Allo_CASE                     \
    p += 4;                           \
    if ((*p == 'w') || (*p == 'W')) { \
            hdr->type = HDR_ALLOW;    \
            p++;                      \
	    goto dc_end;              \
    }                                 \
    goto other;


#endif /* CASE_ALLO_H */
