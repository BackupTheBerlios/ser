/* $Id: ctl_defaults.h,v 1.2 2006/09/19 16:13:28 andrei Exp $
 */

#ifndef __ctl_defaults_h
#define __ctl_defaults_h
/*listen by default on: */
#define DEFAULT_CTL_SOCKET  "unixs:/tmp/ser_ctl"
/* port used by default for tcp/udp if no port is explicitely specified */
#define DEFAULT_CTL_PORT 2049

#define PROC_CTL -32

#endif
