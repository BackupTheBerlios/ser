/*
 * $Id: case_rout.h,v 1.1 2002/07/25 12:28:26 janakj Exp $
 *
 * Route header field parser macros
 */

#ifndef CASE_ROUT_H
#define CASE_ROUT_H


#define Rout_CASE                   \
     p += 4;                        \
     switch(*p) {                   \
     case 'e':                      \
     case 'E':                      \
	     hdr->type = HDR_ROUTE; \
	     p++;                   \
	     goto dc_end;           \
                                    \
     default:                       \
	     goto other;            \
     }


#endif

