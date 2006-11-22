#!/bin/sh
#
# $Id: ser_mysql.sh,v 1.6 2006/11/22 13:57:51 janakj Exp $
#
# Script for adding and dropping ser MySQL tables
#
# Copyright (C) 2006 iptelorg GmbH
#

#################################################################
# configuration variables
#################################################################
DEFAULT_DBHOST="localhost"   # Default hostname of the database server
DEFAULT_SQLUSER="root"       # Database username with admin privileges
DEFAULT_DBNAME="ser"         # Default name of SER database
DEFAULT_ROUSER="serro"       # Default read-only username to SER database
DEFAULT_ROPASS="47serro11"   # Default password of read-only user
DEFAULT_RWUSER="ser"         # Default username of read-write user
DEFAULT_RWPASS="heslo"       # Default password of read-write user

DEFAULT_MYSQL="mysql"
DEFAULT_MYSQLDUMP="mysqldump"

DEFAULT_CREATE_SCRIPT="my_create.sql"
DEFAULT_DATA_SCRIPT="my_data.sql"
DEFAULT_DROP_SCRIPT="my_drop.sql"

CMD="$MYSQL -f -h$DBHOST -u$SQLUSER"
DEFAULT_DUMP_OPTS="-c -a -e --add-locks --all"

usage() {
cat <<EOF
NAME
  $COMMAND - SER MySQL database administration tool

SYNOPSIS
  $COMMAND [options] create
  $COMMAND [options] drop
  $COMMAND [options] backup [filename] 
  $COMMAND [options] restore [filename]
  $COMMAND [options] update-data

DESCRIPTION
  This tool is a simple shell wrapper over mysql client utility that can
  be used to create, drop, or backup SER database stored on a MySQL server.
  See section COMMANDS for brief overview of supported actions.

  The SQL definition of tables within SER database is stored in a separate
  file which can be usualy found in /usr/lib/ser/my_create.sql (depending
  on installation). You can use that file to create SER database manually
  if you cannot or do not want to use this shell wrapper.

  This tool requires mysql client utility to create or drop SER database.
  Furthemore backup and restore commands require mysqldump. Both tools
  can be found in mysql-client package.

COMMANDS
  create
    Create a new SER database from scratch. The database must not exist.
    This command creates the database, the default name of the database
    is '${DEFAULT_DBNAME}' (the default name can be changed using a command line
    parameter, see below). Furthemore the script will load table definition
    from the external SQL file and create users with access to the newly
    created database. You can use command line options to change the
    default database name, usernames and passwords. Note that you need to
    change SER and SERWeb configuration if you change database name or
    usernames because SER and SERWeb are pre-configured to use the default
    names.

  drop
    This command can be used to delete SER database and corresponding
    database users. WARNING: This command will delete all data in the
    database and this action cannot be undone afterwards. Make sure that
    you have backups if you want to keep the data from the database.
    The command also deletes the database users by default. You can change
    that behavior using -k command line options, see below.

  backup <filename>
    Backup the contents of SER database. If you specify a filename then the
    contents of the database will be saved in that file, otherwise the tool
    will dumps the contents on the standard output. By default the backup
    SQL data contains CREATE TABLE statements that will drop and recreate
    database tables being loaded. This ensures that the tables are empty
    and have correct structure. You can change this behavior using -t command 
    line option.

  restore <filename>
    Load the contents of SER database from a file (if you specify one) or
    from the standard input. Make sure that the database exists before you
    load the data. Make sure that the database is empty if you have backups 
    without create table statements (i.e. created with -t command line option) 
    and that the tables are empty.

  update-data
    Update initial data in database. This command delete vendor-controled
    rows from databaze and replace them with new ones.
    
    
OPTIONS
  -h, --help
      Display this help text.

  -n NAME, --name=NAME
      Database name of SER database.
      (Default value is '$DEFAULT_DBNAME')

  -r USERNAME, --ro-username=USERNAME
      Username of user with read-only permissions to SER database.
      (Default value is '$DEFAULT_ROUSER')

  -w USERNAME, --rw-username=USERNAME
      Username of user with read-write permissions to SER database.
      (Default value is '$DEFAULT_RWUSER')

  -p PASSWORD, --ro-password=PASSWORD
      Password of user with read-only permissions to SER database.
      (Default value is '$DEFAULT_ROPASS')

  -P PASSWORD, --rw-password=PASSWORD
      Password of user with read-write permissions to SER database.
      (Default value is '$DEFAULT_RWPASS')

  -t, --tables
      Do not write CREATE TABLE statements that recreate tables when
      restoring data from backup.      

  -s HOST, --server=HOST
      Hostname or IP address of database server.
      (Default value is '$DEFAULT_DBHOST')

  -u USERNAME, --username=USERNAME
      Username of database administrator.
      (Default value is '$DEFAULT_SQLUSER')

  -q[PASSWORD], --sql-password[=PASSWORD]
      Database administrator password. If you specify this option without
      value then the script will assume that no password for database
      administrator is needed and will not ask for it.
      (No default value)

  -k, --keep-users
      Do not delete database users when removing the database. This
      is useful if you have multiple databases and use the same users
      to access them.

  -v, --verbose
      Enable verbose mode. This option can be given multiple times
      to produce more and more output.
        
ENVIRONMENT VARIABLES
  MYSQL     Path to mysql command (Currently ${MYSQL})
  MYSQLDUMP Path to mysqldump command (Currently ${MYSQLDUMP})

AUTHOR
  Written by Jan Janak <jan@iptel.org>

COPYRIGHT
  Copyright (C) 2006 iptelorg GmbH
  This is free software. You may redistribute copies of it under the
  termp of the GNU General Public License. There is NO WARRANTY, to the
  extent permitted by law.

FILES
  $CREATE_SCRIPT
  $DATA_SCRIPT
  $DROP_SCRIPT
    
REPORTING BUGS
  Report bugs to <ser-bugs@iptel.org>             
EOF
} #usage


#
# Read password from user
#
prompt_pw()
{
    export PW

    if [ ! -z $DONT_ASK ] ; then
	unset PW
	return 0
    elif [ -z "$PW" ] ; then
	savetty=`stty -g`
	printf "Enter password for MySQL user ${SQLUSER} (Hit enter for no password): "
	stty -echo
	read PW
	stty $savetty
	echo
    fi
    if [ -z "$PW" ]; then
	unset PW
    else
	PW="-p$PW"
    fi
}

#
# Execute an SQL command
#
sql_query()
{
    if [ $# -gt 1 ] ; then
	DB=$1
	shift
	$CMD "$PW" $MYSQL_OPTS "$DB" -e "$@"
    else
	$CMD "$PW" $MYSQL_OPTS "$@"
    fi
}

# Drop SER database
drop_db()
{
    # Drop the database if it exists
    sql_query "" "DROP DATABASE IF EXISTS ${DBNAME}"

    # Revoke permissions to both RW and RO users
    sql_query "" "REVOKE ALL PRIVILEGES ON ${DBNAME}.* FROM '${RWUSER}'@'%'"
    sql_query "" "REVOKE ALL PRIVILEGES ON ${DBNAME}.* FROM '${RWUSER}'@'localhost'"

    sql_query "" "REVOKE ALL PRIVILEGES ON ${DBNAME}.* FROM '${ROUSER}'@'%'"
    sql_query "" "REVOKE ALL PRIVILEGES ON ${DBNAME}.* FROM '${ROUSER}'@'localhost'"

    if [ ! -z "$KEEP_USERS" ] ; then
	    # Works only with MySQL 4.1.1 and higher
		#sql_query "" "DROP USER '${RWUSER}'@'%'"
		#sql_query "" "DROP USER '${RWUSER}'@'localhost'"
		#sql_query "" "DROP USER '${ROUSER}'@'%'"
		#sql_query "" "DROP USER '${ROUSER}'@'localhost'"
		
		# Works with older MySQL versions
		sql_query "" "DELETE FROM mysql.user WHERE User='${RWUSER}' and Host='%'"
		sql_query "" "DELETE FROM mysql.user WHERE User='${RWUSER}' and Host='localhost'"
		sql_query "" "DELETE FROM mysql.user WHERE User='${ROUSER}' and Host='%'"
		sql_query "" "DELETE FROM mysql.user WHERE User='${ROUSER}' and Host='localhost'"
    fi

    sql_query "" "FLUSH PRIVILEGES"
} # drop_db


# Create SER database
create_db ()
{
    # Create the database
    sql_query "" "CREATE DATABASE IF NOT EXISTS ${DBNAME}"

    # Add read/write access to RWUSER
    sql_query "" "GRANT ALL ON ${DBNAME}.* TO '${RWUSER}'@'%' IDENTIFIED BY '${RWPASS}'"
    sql_query "" "GRANT ALL ON ${DBNAME}.* TO '${RWUSER}'@'localhost' IDENTIFIED BY '${RWPASS}'"
    
    # Add read-only access to ROUSER
    sql_query "" "GRANT ALL ON ${DBNAME}.* TO '${ROUSER}'@'%' IDENTIFIED BY '${ROPASS}'"
    sql_query "" "GRANT ALL ON ${DBNAME}.* TO '${ROUSER}'@'localhost' IDENTIFIED BY '${ROPASS}'"
    
    # Activate changes
    sql_query "" "FLUSH PRIVILEGES"

    # Load table definitions
    sql_query $DBNAME < $CREATE_SCRIPT

    # Load initial data
    sql_query $DBNAME < $DATA_SCRIPT
} # create_db


# Update initial data
update_db_data ()
{
    sql_query $DBNAME < $DATA_SCRIPT
} # update_db_data


# Main program
COMMAND=`basename $0`

if [ -z "$DBNAME" ] ; then DBNAME=$DEFAULT_DBNAME; fi;
if [ -z "$ROUSER" ] ; then ROUSER=$DEFAULT_ROUSER; fi;
if [ -z "$RWUSER" ] ; then RWUSER=$DEFAULT_RWUSER; fi;
if [ -z "$ROPASS" ] ; then ROPASS=$DEFAULT_ROPASS; fi;
if [ -z "$RWPASS" ] ; then RWPASS=$DEFAULT_RWPASS; fi;
if [ -z "$DBHOST" ] ; then DBHOST=$DEFAULT_DBHOST; fi;
if [ -z "$SQLUSER" ] ; then SQLUSER=$DEFAULT_SQLUSER; fi;
if [ -z "$MYSQL" ] ; then MYSQL=$DEFAULT_MYSQL; fi
if [ -z "$MYSQLDUMP" ] ; then MYSQLDUMP=$DEFAULT_MYSQLDUMP; fi
if [ -z "$DUMP_OPTS" ] ; then DUMP_OPTS=$DEFAULT_DUMP_OPTS; fi 
if [ -z "$CREATE_SCRIPT" ] ; then CREATE_SCRIPT=`dirname $0`"/"$DEFAULT_CREATE_SCRIPT; fi
if [ -z "$DATA_SCRIPT" ] ; then DATA_SCRIPT=`dirname $0`"/"$DEFAULT_DATA_SCRIPT; fi
if [ -z "$DROP_SCRIPT" ] ; then DROP_SCRIPT=`dirname $0`"/"$DEFAULT_DROP_SCRIPT; fi

TEMP=`getopt -o hn:r:w:p:P:ts:u:vkq:: --long help,name:,ro-username:,rw-username:,\
ro-password:,rw-password:,tables,server:,username:,verbose,keep-users,sql-password:: -n $COMMAND -- "$@"`
if [ $? != 0 ] ; then exit 1; fi
eval set -- "$TEMP"

while true ; do
    case "$1" in
	-h|--help)         usage; exit 0 ;;
	-n|--name)         DBNAME=$2; shift 2 ;;
	-r|--ro-username)  ROUSER=$2; shift 2 ;;
	-w|--rw-username)  RWUSER=$2; shift 2 ;;
	-p|--ro-password)  ROPASS=$2; shift 2 ;;
	-P|--rw-password)  RWPASS=$2; shift 2 ;;
	-t|--tables)       DUMP_OPTS="$DUMP_OPTS -t "; shift ;;
	-s|--server)       DBHOST=$2; shift 2 ;;
	-u|--username)     SQLUSER=$2; shift 2 ;;
        -v|--verbose)      MYSQL_OPTS="$MYSQL_OPTS -v "; shift ;;
	-k|--keep-users)   KEEP_USERS=1; shift ;;
        -q|--sql-password)
	    case "$2" in
		"") DONT_ASK=1; shift 2 ;;
		*)  PW=$2; shift 2 ;;
	    esac 
	    ;;
	--)               shift; break ;;
	*)                echo "Internal error"; exit 1 ;;
    esac
done

if [ $# -eq 0 ]; then
    usage
    exit 1
fi

# Make sure we can execute mysql command
TEMP=`which $MYSQL`
if [ $? != 0 ] ; then
    echo "Could not find mysql client utility"
    echo "Set MYSQL environment variable properly (see -h for more info)"
    exit 1
fi

# Make sure we can execute mysqldump command
TEMP=`which $MYSQLDUMP`
if [ $? != 0 ] ; then
    echo "Could not find mysqldump utility"
    echo "Set MYSQLDUMP environment variable properly (see -h for more info)"
    exit 1
fi


CMD="$MYSQL -h$DBHOST -u$SQLUSER"
DUMP_CMD="${MYSQLDUMP} -h$DBHOST -u$SQLUSER $DUMP_OPTS"

case $1 in
    create) # Create SER database and users
	prompt_pw
	create_db
	exit $?
	;;
    
    drop) # Drop SER database and users
	prompt_pw
	drop_db
	exit $?
	;;

    update-data) # Update initial data
	prompt_pw
	update_db_data
	exit $?
	;;
    
    backup) # backup SER database
	shift
	if [ $# -eq 1 ] ; then
	    prompt_pw
	    $DUMP_CMD "$PW" $MYSQL_OPTS ${DBNAME} > $1
	elif [ $# -eq 0 ] ; then
	    prompt_pw
	    $DUMP_CMD "$PW" $MYSQL_OPTS ${DBNAME}
	else
	    usage
	    exit 1
	fi
	exit $?
	;;

    restore) # restore SER database
	shift
	if [ $# -eq 1 ]; then
	    prompt_pw
	    sql_query $DBNAME < $1
	elif [ $# -eq 0 ] ; then
	    prompt_pw
	    cat | sql_query $DBNAME
	else
	    usage
	    exit 1
	fi
	exit $?
	;;
        
    *)
	echo "Unknown command '$1'"
	usage
	exit 1;
	;;
esac
