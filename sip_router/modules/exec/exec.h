/*
 *
 * $Id: exec.h,v 1.1 2002/08/16 13:22:46 jku Exp $
 *
 */

int exec_str(struct sip_msg *msg, char *cmd, char *param);
int exec_msg(struct sip_msg *msg, char *cmd );
