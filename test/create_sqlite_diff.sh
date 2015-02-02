#!/bin/bash

# Author:  Lukas Toggenburger
# Website: https://github.com/ltog/osmi-addresses

description="This tool reads two spatialite files and creates a third containing the difference (deleted and newly added rows) of the tables of those two files."
dummydb='dummydbdeleteme'

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

attachdbs="ATTACH DATABASE '$1' AS db1; ATTACH DATABASE '$2' AS db2; ATTACH DATABASE '$3' AS db3;"

# for each table write the differences to a new database
while read -r table; do
	echo "Processing table $table"

	# create new geometry tables in db3
	# variant 1: manually call AddGeometryColumn: http://stackoverflow.com/a/23366622 https://www.gaia-gis.it/spatialite-2.3.0/spatialite-sql-2.3.0.html#p16
	# variant 2: clone table

	# get schema
	schema=$(spatialite $1 ".schema $table" | grep 'CREATE TABLE' | grep "$table")

	# get geometry type
	geometry= #TODO

	# TODO: remove geometry column from schema (sqlite/spatialite can't rename/remove columns)
	schema=

	# create new tables
	spatialite $3 "$schema"
	spatialite $3 "$schema"

	#CREATE TABLE 'blub_deleted' (ogc_fid INTEGER PRIMARY KEY);
	#CREATE TABLE 'blub_added' (ogc_fid INTEGER PRIMARY KEY);
	# TODO: how to copy all columns except geometry? add all and delete geometry column again?

	# add geometry column
	spatialite $3 "AddGeometryColumn('${table}_deleted', 'geometry', '4326', '$geometry');"
	spatialite $3 "AddGeometryColumn('${table}_added',   'geometry', '4326', '$geometry');"

	#TODO: calculate differences and write them to db3
	spatialite $dummydb "$attachdbs INSERT INTO db3.'${table}_deleted' SELECT * from db1.'${table}' WHERE NOT EXISTS (SELECT * FROM db2.'${table}' WHERE EQUALS(db1.'${table}'.geometry, db2.'${table}'.geometry));"
	spatialite $dummydb "$attachdbs INSERT INTO db3.'${table}_added'   SELECT * from db2.'${table}' WHERE NOT EXISTS (SELECT * FROM db1.'${table}' WHERE EQUALS(db1.'${table}'.geometry, db2.'${table}'.geometry));
	
done <<< "$tables1"

# delete dummy db
rm dummydbdeleteme
