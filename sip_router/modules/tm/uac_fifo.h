/*
 * UAC FIFO interface
 *
 * $Id: uac_fifo.h,v 1.2 2004/02/11 03:38:49 jiri Exp $
 *
 */

#ifndef UAC_FIFO_H
#define UAC_FIFO_H

#include <stdio.h>


/*
 * FIFO function for sending messages
 */
int fifo_uac(FILE *stream, char *response_file);


#endif /* UAC_FIFO_H */
