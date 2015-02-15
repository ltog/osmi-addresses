#!/bin/bash

# Author:  Lukas Toggenburger
# Website: https://github.com/ltog/osmi-addresses

# Note on the output of spatialite: spatialite tries to be smart and sniffs the kind of output destination. If the output destination is a terminal, it will output some verbose information each time you run it. This is the reason why most (probably all) calls to spatialite in this software have a pipe after them.

description="This tool reads two spatialite files and creates a third containing the difference (deleted and newly added rows) of the tables of those two files."

geometry_column='geometry' # name of the column where geometries are stored
dummydb='dummydbdeleteme'
old_suffix='_deleted'
new_suffix='_added'
tmpdir='/tmp/'
p1=${tmpdir}db1 # path for temporary file for db1
p2=${tmpdir}db2 # path for temporary file for db2

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

# create copies of the database files
cp -f $1 $p1
cp -f $2 $p2

attach2dbs="ATTACH DATABASE '$p1' AS db1; ATTACH DATABASE '$p2' AS db2;"
attach3dbs="ATTACH DATABASE '$p1' AS db1; ATTACH DATABASE '$p2' AS db2; ATTACH DATABASE '$3' AS db3;"

# for each table write the differences to a new database
while read -r table; do
	echo "Processing table '$table' ..."

	# PHASE 1: erase identical rows of copied databases
	echo "  removing identical rows in copies of databases..."

	spatialite $dummydb "$attach2dbs SELECT COUNT(*) FROM db1.${table}" | grep -ve '^$'
	spatialite $dummydb "$attach2dbs SELECT COUNT(*) FROM db2.${table}" | grep -ve '^$'

	# create temporary tables for ids to be deleted
	spatialite $dummydb "$attach2dbs DROP TABLE IF EXISTS db1.${table}_doomed;" | grep -ve '^$'
	spatialite $dummydb "$attach2dbs DROP TABLE IF EXISTS db2.${table}_doomed;" | grep -ve '^$'
	spatialite $dummydb "$attach2dbs CREATE TABLE db1.${table}_doomed (id INTEGER);" | grep -ve '^$'
	spatialite $dummydb "$attach2dbs CREATE TABLE db2.${table}_doomed (id INTEGER);" | grep -ve '^$'

	shift=0

	# do-while loop, see http://stackoverflow.com/a/16489942
	while : ; do

		echo "  checking shift=${shift}..."

		# TODO: check also for negative shifts
		# load ids of rows to be deleted
		spatialite $dummydb "$attach2dbs INSERT INTO db1.'${table}_doomed' SELECT db1.${table}.ogc_fid FROM db1.$table INNER JOIN db2.$table ON db1.${table}.ogc_fid+$shift=db2.${table}.ogc_fid WHERE Equals(db1.${table}.${geometry_column},db2.${table}.${geometry_column})" | grep -ve '^$'
		spatialite $dummydb "$attach2dbs INSERT INTO db2.'${table}_doomed' SELECT db2.${table}.ogc_fid FROM db2.$table INNER JOIN db1.$table ON db1.${table}.ogc_fid+$shift=db2.${table}.ogc_fid WHERE Equals(db1.${table}.${geometry_column},db2.${table}.${geometry_column})" | grep -ve '^$'

		doomed_counter=$(spatialite $p1 "SELECT COUNT(*) FROM ${table}_doomed LIMIT 1")

		echo doomed_counter=$doomed_counter

		[[ $doomed_counter > 0 ]] || break # note the condition to _stay_ in the loop

		echo "  staying in the loop for another round..."

		# delete duplicated rows
		spatialite $dummydb "$attach2dbs DELETE FROM db1.'$table' WHERE ogc_fid IN (SELECT id FROM db1.${table}_doomed)" | grep -ve '^$'
		spatialite $dummydb "$attach2dbs DELETE FROM db2.'$table' WHERE ogc_fid IN (SELECT id FROM db2.${table}_doomed)" | grep -ve '^$'

		# delete _doomed entries
		spatialite $dummydb "$attach2dbs DELETE FROM db1.${table}_doomed" | grep -ve '^$'
		spatialite $dummydb "$attach2dbs DELETE FROM db2.${table}_doomed" | grep -ve '^$'

		# increment shift
		shift=$((shift+1))
	done

	# delete temporary tables
	spatialite $dummydb "$attach2dbs DROP TABLE db1.'${table}_doomed'" | grep -ve '^$'
	spatialite $dummydb "$attach2dbs DROP TABLE db2.'${table}_doomed'" | grep -ve '^$'

	spatialite $dummydb "$attach2dbs SELECT COUNT(*) FROM db1.${table}" | grep -ve '^$'
	spatialite $dummydb "$attach2dbs SELECT COUNT(*) FROM db2.${table}" | grep -ve '^$'

	# PHASE 2

	# get schema
	schema=$(spatialite $p1 ".schema $table" | grep 'CREATE TABLE' | grep "$table")

	# get geometry type (extract whats written after "GEOMETRY"
	geometry_type=$(echo $schema | sed -e 's/.*\"GEOMETRY\" \([A-Za-z]*\)[,)].*/\1/')

	# remove the "GEOMETRY" column from the 'CREATE TABLE ...' expression
	schema_without_geometry=$(echo $schema | sed -e 's/, \"GEOMETRY\" [A-Za-z]*//')

	# adjust table names in 'CREATE TABLE ...' expressions
	schema_old=$(echo $schema_without_geometry | sed -e "s/\(${table}\)/\1${old_suffix}/")
	schema_new=$(echo $schema_without_geometry | sed -e "s/\(${table}\)/\1${new_suffix}/")

	# get srid
	srid=$(spatialite $p1 "SELECT srid FROM geometry_columns WHERE f_table_name='$table';" | tail)

	# create new tables in new file
	spatialite $3 "$schema_old" | grep -ve '^$'
	spatialite $3 "$schema_new" | grep -ve '^$'

	# add geometry column
	spatialite $3 "SELECT AddGeometryColumn('${table}${old_suffix}', '$geometry_column', $srid, '$geometry_type');" | grep -ve '1'
	spatialite $3 "SELECT AddGeometryColumn('${table}${new_suffix}', '$geometry_column', $srid, '$geometry_type');" | grep -ve '1'

	# calculate differences and write them to db3
	echo "  searching for deleted geometries..."
	spatialite $dummydb "$attach3dbs INSERT INTO db3.'${table}${old_suffix}' SELECT * from db1.'${table}' WHERE NOT EXISTS (SELECT * FROM db2.'${table}' WHERE EQUALS(db1.'${table}'.$geometry_column, db2.'${table}'.$geometry_column));" | grep -ve '^$'

	echo "  searching for newly added geometries..."
	spatialite $dummydb "$attach3dbs INSERT INTO db3.'${table}${new_suffix}' SELECT * from db2.'${table}' WHERE NOT EXISTS (SELECT * FROM db1.'${table}' WHERE EQUALS(db1.'${table}'.$geometry_column, db2.'${table}'.$geometry_column));" | grep -ve '^$'

done <<< "$tables1"

# delete dummy db
rm $dummydb
rm $p1
rm $p2
