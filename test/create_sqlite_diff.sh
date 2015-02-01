#!/bin/bash

# Author:  Lukas Toggenburger
# Website: https://github.com/ltog/osmi-addresses

description="This tool reads two spatialite files and creates a third containing the difference (deleted and newly added rows) of the tables of those two files."

# make sure exactly 3 arguments are given
if [ $# -ne 3 ]; then
	echo "Usage: $0 old.sqlite new.sqlite diff.sqlite"
	echo "Description: $description"
	exit 1
fi

# make sure the input file DO exist and the output file DOESN'T exist
if [ ! -f $1 ]; then
	echo "ERROR: $1 is not a file."
	exit 1
fi

if [ ! -f $2 ]; then
	echo "ERROR: $2 is not a file."
	exit 2
fi

if [ -e $3 ]; then
	echo "ERROR: $3 exists."
	exit 3
fi

# make note of the tables names of the two files (can't use ".tables" since its output has two columns)
tables1=$(sqlite3 $1 '.schema' | grep "CREATE TABLE '" | sed -e "s/^CREATE TABLE '\([^']*\)'.*$/\1/" | sort)
tables2=$(sqlite3 $2 '.schema' | grep "CREATE TABLE '" | sed -e "s/^CREATE TABLE '\([^']*\)'.*$/\1/" | sort)

# make sure the table names are the same
if [ "$tables1" != "$tables2" ]
then
	difference=$(echo -e "${tables1}\n${tables2}" | sort | uniq -u) # store the table names that occurred only once
	echo "ERROR: Tables are not the same. Look out for table(s):"
	echo "$difference"
	exit 4
fi

attach="ATTACH DATABASE '$1' AS db1; ATTACH DATABASE '$2' AS db2; "

# for each table write the differences to a new database
while read -r table; do
	echo "Processing table $table"

	#TODO: calculate differences and write them to db3
	spatialite dummydbdeleteme "$attach ; select astext(geometry) from db1.ausschnitt limit 1"
	
done <<< "$tables1"

# delete dummy db
rm dummydbdeleteme
