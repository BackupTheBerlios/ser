/*
 * $Id: case_cseq.h,v 1.1 2002/07/25 12:28:26 janakj Exp $
 *
 * CSeq header field parser macros
 */

#ifndef CASE_CSEQ_H
#define CASE_CSEQ_H


#define CSeq_CASE          \
     hdr->type = HDR_CSEQ; \
     p += 4;               \
     goto dc_end


#endif
