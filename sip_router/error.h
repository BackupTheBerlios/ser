/*
 * $Id: error.h,v 1.3 2002/05/31 01:59:06 jku Exp $
 */

#ifndef error_h
#define error_h

#define E_UNSPEC      -1
#define E_OUT_OF_MEM  -2
#define E_BAD_RE      -3
/* #define E_BAD_ADDRESS -4 */
#define E_BUG         -5
#define E_CFG         -6
#define E_NO_SOCKET		-7

#define E_SEND		  -477
/* unresolveable next-hop address */
#define E_BAD_ADDRESS -478
/* unparseable URI */
#define E_BAD_URI 	  -479
/* misformated request */
#define E_BAD_REQ	  -400

#define MAX_REASON_LEN	128

/* processing status of the last command */
extern int ser_error;
extern int prev_ser_error;

int err2reason_phrase( int ser_error, int *sip_error, 
                char *phrase, int etl, char *signature );


#endif
