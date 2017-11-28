#!/bin/sh
#--
# $Id: autogen.sh,v 1.1 2004/07/17 01:26:54 nadim Exp $
#--

EXIT=no

if test "$1" = "clean"; then
    echo "Removing auto-generated files..."
    rm -rf configure config.log config.status \
           Makefile config.h
    EXIT="yes"
fi

if test "$EXIT" = "yes"; then
    exit
fi

echo "Running autoconf..."
autoconf configure.in > configure && chmod +x configure
rm -rf autom4te.cache

echo "You can now run \"./configure\" and then \"make\"."
