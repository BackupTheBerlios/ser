/*
 * $Id: modparam.h,v 1.2 2002/09/19 11:51:26 jku Exp $
 *
 */

#ifndef modparam_h
#define modparam_h

#include "sr_module.h"

int set_mod_param(char* _mod, char* _name, modparam_t _type, void* _val);

#endif
