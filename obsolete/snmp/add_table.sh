#!/bin/bash

# $Id $
#
# SNMP Module
# Script to generate code for a table

if [ -f ./mib2c.sipdataset.conf ]; then
	mib2c -c ./mib2c.sipdataset.conf $1
	echo ""
	echo "!!!!!!! IMPORTANT !!!!!!"
	echo "= Chk output files for XXX and fix as needed"
	echo "= Add header file to snmp_mod.h"
	echo "= Add a call in init_ser_mibs (agent.c) to init_branch(fileName)"
	echo "= Add a call in sipCommon_handler_init (snmp_handler.c) to init_table()"
	echo "!!!!!!!!!!!!!!!!!!!!!!!!"
else
	echo "Couldn't find template file!!!"
fi
