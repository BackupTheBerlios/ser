TARGET		= rlm_gsm
SRCS		= rlm_gsm.c
HEADERS		= 
RLM_CFLAGS	= -I/usr/include/mysql
RLM_LIBS	= -L/usr/lib/mysql -lmysqlclient

include ../rules.mak

$(STATIC_OBJS): $(HEADERS)

$(DYNAMIC_OBJS): $(HEADERS)
