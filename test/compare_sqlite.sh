#!/bin/bash

# Author:  Lukas Toggenburger
# Website: https://github.com/ltog/osmi-addresses

description="This tool compares the contents of two sqlite/spatialite files and prints their difference."

# set names of temporary files
export tmpfile1=/tmp/${0}_deleteme1.tmp
export tmpfile2=/tmp/${0}_deleteme2.tmp

# make sure exactly two arguments are given
if [ $# -ne 2 ]; then
	echo "Usage: $0 file1.sqlite file2.sqlite"
	echo "Description: $description"
	exit 1
fi

# define a function to call a difftool (implemented this way so 'diff' can be used when 'colordiff' ist not installed)
difftool() {
	if hash colordiff 2>/dev/null; then # from http://stackoverflow.com/a/677212
		colordiff "$@"
	else
		diff "$@"
	fi
}

# delete the temporary files
cleanup() {
	rm $tmpfile1 $tmpfile2
}

handle_signals() {
	echo "Signal handling function was called. Going to clean up..."
	cleanup
	exit 1
}

trap handle_signals SIGINT SIGTERM

# make note of the tables names of the two files (can't use ".tables" since its output has two columns)
tables1=$(sqlite3 $1 '.schema' | grep "CREATE TABLE '" | sed -e "s/^CREATE TABLE '\([^']*\)'.*$/\1/" | sort)
tables2=$(sqlite3 $2 '.schema' | grep "CREATE TABLE '" | sed -e "s/^CREATE TABLE '\([^']*\)'.*$/\1/" | sort)

# make sure the table names are the same
if [ "$tables1" != "$tables2" ]
then
	difference=$(echo -e "${tables1}\n${tables2}" | sort | uniq -u) # store the table names that occurred only once
	echo "ERROR: Tables are not the same. Look out for table(s):"
	echo "$difference"
	exit 2
fi

# compare the contents of the tables
while read -r table; do
	echo "Checking table $table"

	# read column info
	sqlite3 $1 "PRAGMA table_info($table)" > $tmpfile1
	sqlite3 $2 "PRAGMA table_info($table)" > $tmpfile2

	# read rows
	sqlite3 $1 "SELECT * FROM $table" >> $tmpfile1
	sqlite3 $2 "SELECT * FROM $table" >> $tmpfile2

	# print diff
	difftool $tmpfile1 $tmpfile2

done <<< "$tables1"

cleanup

