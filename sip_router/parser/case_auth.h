/*
 * $Id: case_auth.h,v 1.2 2002/08/12 12:04:50 janakj Exp $
 *
 * Authoriazation header field parser macros
 */

#ifndef CASE_AUTH_H
#define CASE_AUTH_H


#define AUTH_ATIO_CASE                                 \
        if (val == _atio_) {                           \
	        p += 4;                                \
		switch(*p) {                           \
		case 'n':                              \
		case 'N':                              \
		        hdr->type = HDR_AUTHORIZATION; \
			p++;                           \
			goto dc_end;                   \
                                                       \
		default: goto other;                   \
		}                                      \
	}
	             

#define AUTH_ORIZ_CASE            \
        if (val == _oriz_) {      \
                p += 4;           \
	        val = READ(p);    \
	        AUTH_ATIO_CASE;   \
          			  \
                val = unify(val); \
	        AUTH_ATIO_CASE;   \
                                  \
                goto other;       \
        }


#define Auth_CASE      \
     p += 4;           \
     val = READ(p);    \
     AUTH_ORIZ_CASE;   \
                       \
     val = unify(val); \
     AUTH_ORIZ_CASE;   \
                       \
     goto other;


#endif /* CASE_AUTH_H */
