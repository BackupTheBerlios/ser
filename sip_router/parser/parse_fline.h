/*
 * $Id: parse_fline.h,v 1.3 2002/08/12 12:15:49 janakj Exp $
 */

#ifndef PARSE_FLINE_H
#define PARSE_FLINE_H

#include "../str.h"

/* Message is request */
#define SIP_REQUEST 1

/* Message is reply */
#define SIP_REPLY   2

/* Invalid message */
#define SIP_INVALID 0

#define SIP_VERSION "SIP/2.0"
#define SIP_VERSION_LEN 7

#define CANCEL "CANCEL"
#define ACK    "ACK"
#define INVITE "INVITE"

#define INVITE_LEN 6
#define CANCEL_LEN 6
#define ACK_LEN 3
#define BYE_LEN 3


struct msg_start {
	int type;                         /* Type of the Message - Request/Response */
	union {
		struct {
			str method;       /* Method string */
			str uri;          /* Request URI */
			str version;      /* SIP version */
			int method_value;
		} request;
		struct {
			str version;      /* SIP version */
			str status;       /* Reply status */
			str reason;       /* Reply reason phrase */
			unsigned int /* statusclass,*/ statuscode;
		} reply;
	}u;
};


char* parse_first_line(char* buffer, unsigned int len, struct msg_start * fl);

char* parse_fline(char* buffer, char* end, struct msg_start* fl);


#endif /* PARSE_FLINE_H */
