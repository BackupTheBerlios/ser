/* 
 * $Id: md5utils.h,v 1.2 2002/09/19 11:51:26 jku Exp $
 *
 */

#ifndef _MD5UTILS_H
#define _MD5UTILS_H

#include "str.h"

#define MD5_LEN	32

void MDStringArray (char *dst, str src[], int size);

#endif
