/* 
 * $Id: parser_f.h,v 1.1 2001/09/03 21:27:11 andrei Exp $
 */

#ifndef parser_f_h
#define parser_f_h

char* eat_line(char* buffer, unsigned int len);
char* eat_space(char* buffer, unsigned int len);
char* eat_token(char* buffer, unsigned int len);
char* eat_token2(char* buffer, unsigned int len, char delim);
int is_empty(char* buffer, unsigned int len);

#endif
 
