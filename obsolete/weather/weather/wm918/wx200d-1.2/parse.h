/*
 * @(#)$Id: parse.h,v 1.1 2002/09/23 19:12:51 bogdan Rel $
 *
 * Copyright (C) 1998 - 2001 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 */

/* byte converting macros for improved readability and efficiency */
#define LO(byte)	(byte & 0x0f)
#define HI(byte)	((byte & 0xf0) >> 4)
#define MHI(byte)	((byte & 0x0f) << 4)
#define NUM(byte)	(10 * HI(byte) + LO(byte))
#define BIT(byte, bit)	((byte & (1 << bit)) >> bit)
#define ERROR(mem, byte, value)	if (!((mem.err) = (byte == 0xee))) \
				    (mem.val) = (value);

/* raw int to base float unit conversions */
#define TENTHS(val)	(val * 0.1)
#define F2C(val)	((val - 32.0) / 1.8)
#define IN2MM(val)	(val * 25.4)
#define MPH2MPS(val)	(val * 0.44704)
