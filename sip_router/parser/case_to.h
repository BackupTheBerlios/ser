/*
 * $Id: case_to.h,v 1.2 2002/08/12 12:04:50 janakj Exp $
 *
 * To header field parser macros
 */

#ifndef CASE_TO_H
#define CASE_TO_H


#define To12_CASE           \
        hdr->type = HDR_TO; \
        hdr->name.len = 2;  \
        *(p + 2) = '\0';    \
        return (p + 4);


#endif /* CASE_TO_H */
