/*
 * @(#)$Id: client.c,v 1.1 2002/09/23 19:12:51 bogdan Rel $
 *
 * Copyright (C) 1998 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file opens and closes client connections to a wx200d server.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "wx200.h"
#include "config.h"

#define BUFLEN 32		/* max length of hostname:port */
char wx200host[BUFLEN];
int wx200port = PORT;
char wx200error[80];

/* connect to a wx200d server, return the socket or -1 if error */
int
wx200open(char *hostport)
{
  char *c;
  int s;
  struct sockaddr_in server;
  struct in_addr address;
  struct hostent *host;

  if (hostport != NULL && *hostport != '\0' && *hostport != ':')
    strncpy(wx200host, hostport, BUFLEN);
  else if ((c = getenv("WX200D")) != NULL && *c != '\0' && *c != ':')
    strncpy(wx200host, c, BUFLEN);
  else
    strncpy(wx200host, HOST, BUFLEN);
  if ((c = strchr(wx200host, ':')) != NULL)
    *c = '\0';
  if (hostport != NULL && (c = strrchr(hostport, ':')) != NULL)
    wx200port = strtol(++c, NULL, 0);
  else if ((c = getenv("WX200D")) != NULL && (c = strrchr(c, ':')) != NULL)
    wx200port = strtol(++c, NULL, 0);
  else
    wx200port = PORT;

  if (strspn(wx200host, "0123456789.") == strlen(wx200host)) {
    inet_aton(wx200host, &address);
  } else {
    if ((host = gethostbyname(wx200host)) == NULL) {
      sprintf(wx200error, "can't resolve %s: %d",
	      wx200host, h_errno);
      return(-1);
    }
    address.s_addr = ((struct in_addr *)(host->h_addr_list[0]))->s_addr;
    /*     fprintf(stderr, "%s=%s=%s\n", wx200host, */
    /* 	    host->h_name, inet_ntoa(address)); */
  }
  server.sin_addr=address;
  server.sin_port=htons(wx200port);
  server.sin_family=AF_INET;

  if ((s = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) == -1) {
    sprintf(wx200error, "socket: %s", strerror(errno));
    return(-2);
  }
  {
    /* To allow quick (re) start of the deamon - without getting a
     * 'port/address in use' when a client was bound during
     * killoff. <dirkx@covalent.net> / April 2001 */
    int i = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) == -1) {
      sprintf(wx200error, "setsockopt: %s",  strerror(errno));
      return(-2);
    }
  }
  if (connect(s, (struct sockaddr *)&server,
	      sizeof(struct sockaddr_in)) == -1) {
    sprintf(wx200error, "connect %s:%d: %s",
	    wx200host, wx200port, strerror(errno));
    return(-3);
  }
  return(s);
}

/* close the given wx200 socket connection  */
int
wx200close(int socket)
{
  shutdown(socket, 2);
  return(close(socket));
}
