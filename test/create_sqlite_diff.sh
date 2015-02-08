#!/bin/bash

# Author:  Lukas Toggenburger
# Website: https://github.com/ltog/osmi-addresses

description="This tool reads two spatialite files and creates a third containing the difference (deleted and newly added rows) of the tables of those two files."

dummydb='dummydbdeleteme'
old_suffix='_deleted'
new_suffix='_added'

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
	echo "Processing table '$table' ..."

	# create new geometry tables in db3
	# variant 1: manually call AddGeometryColumn: http://stackoverflow.com/a/23366622 https://www.gaia-gis.it/spatialite-2.3.0/spatialite-sql-2.3.0.html#p16
	# variant 2: clone table (use RecoverGeometryColumn (?))

	# get schema
	schema=$(spatialite $1 ".schema $table" | grep 'CREATE TABLE' | grep "$table")

	# get geometry type (extract whats written after "GEOMETRY"
	geometry_type=$(echo $schema | sed -e 's/.*\"GEOMETRY\" \([A-Za-z]*\)[,)].*/\1/')

	# remove the "GEOMETRY" column from the 'CREATE TABLE ...' expression
	schema_without_geometry=$(echo $schema | sed -e 's/, \"GEOMETRY\" [A-Za-z]*//')

	# adjust table names in 'CREATE TABLE ...' expressions
	schema_old=$(echo $schema_without_geometry | sed -e "s/\(${table}\)/\1${old_suffix}/")
	schema_new=$(echo $schema_without_geometry | sed -e "s/\(${table}\)/\1${new_suffix}/")

	# get srid
	srid=$(spatialite $1 "SELECT srid FROM geometry_columns WHERE f_table_name='$table';" | tail)

	echo DEBUG: $schema_old
	echo DEBUG: $schema_new
	echo DEBUG: $geometry_type,

	# create new tables in new file
	spatialite $3 "$schema_old"
	spatialite $3 "$schema_new"

	# add geometry column
	spatialite $3 "SELECT AddGeometryColumn('${table}${old_suffix}', 'geometry', $srid, '$geometry_type');"
	spatialite $3 "SELECT AddGeometryColumn('${table}${new_suffix}', 'geometry', $srid, '$geometry_type');"

	# calculate differences and write them to db3
	spatialite $dummydb "$attachdbs INSERT INTO db3.'${table}${old_suffix}' SELECT * from db1.'${table}' WHERE NOT EXISTS (SELECT * FROM db2.'${table}' WHERE EQUALS(db1.'${table}'.geometry, db2.'${table}'.geometry));"
	spatialite $dummydb "$attachdbs INSERT INTO db3.'${table}${new_suffix}' SELECT * from db2.'${table}' WHERE NOT EXISTS (SELECT * FROM db1.'${table}' WHERE EQUALS(db1.'${table}'.geometry, db2.'${table}'.geometry));"

done <<< "$tables1"

# delete dummy db
rm $dummydb
