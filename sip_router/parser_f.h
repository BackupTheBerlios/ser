/* 
 * $Id: parser_f.h,v 1.4 2001/11/23 16:38:51 jku Exp $
 */

#ifndef parser_f_h
#define parser_f_h

char* eat_line(char* buffer, unsigned int len);

/* macro now
int is_empty(char* buffer, unsigned int len);
*/

/* MACROEATER no more optional */
/* #ifdef MACROEATER */

/* turn the most frequently called functions into macros */


#define eat_space_end(buffer,pend)                                       \
  ( {   char *p;                                                 	\
	char *pe=(pend);						\
        for(p=(buffer);(p<pe)&& (*p==' ' || *p=='\t') ;p++);		\
        p;                                                              \
  } )

#define eat_token_end(buffer,pend)					\
  ( { char *p       ;							\
      char *pe=(pend);						\
      for (p=(buffer);(p<pe)&&					\
                        (*p!=' ')&&(*p!='\t')&&(*p!='\n')&&(*p!='\r');	\
                p++);							\
      p;								\
  } )

#define eat_token2_end(buffer,pend,delim)					\
  ( { char *p       ;							\
      char *pe=(pend);						\
      for (p=(buffer);(p<pe)&&					\
                        (*p!=(delim))&&(*p!='\n')&&(*p!='\r');		\
                p++);							\
      p;								\
  } )

#define is_empty_end(buffer, pend )					\
  ( { char *p;								\
      char *pe=(pend);						\
      p=eat_space_end( buffer, pe );					\
      ((p<pend ) && (*p=='\r' || *p=='\n')) ? 1 : 0;			\
  } )


/*
#else


char* eat_space(char* buffer, unsigned int len);
char* eat_token(char* buffer, unsigned int len);
char* eat_token2(char* buffer, unsigned int len, char delim);

#endif
*/
/* EoMACROEATER */

#endif
