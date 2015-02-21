#!/bin/bash

# Author:  Lukas Toggenburger
# Website: https://github.com/ltog/osmi-addresses

# Note on the output of spatialite: spatialite tries to be smart and sniffs the kind of output destination. If the output destination is a terminal, it will output some verbose information each time you run it. This is the reason why most (probably all) calls to spatialite in this software have a pipe after them.

description="This tool reads two spatialite files and creates a third containing the difference (deleted and newly added rows) of the tables of those two files."

parallel_options="--noswap"

export geometry_column='geometry' # name of the column where geometries are stored
export old_suffix='_deleted'
export new_suffix='_added'
export tmpdir="/tmp/${RANDOM}${RANDOM}/"
export originaldb1=$1 # path to original db1
export originaldb2=$2 # path to original db2
export outputdb=$3           # path to output db
export tmpdbprefix=${tmpdir}deleteme_
export tables1 # table names from original db1
export tables2 # table names from original db2

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

handle_signals() {
	echo "Signal handling function was called..."
	cleanup
	exit 1
}

trap handle_signals SIGINT SIGTERM

read_table_names() {
	# make note of the tables names of the two files (can't use ".tables" since its output has two columns)
	tables1=$(spatialite $originaldb1 '.schema' | grep "CREATE TABLE '" | sed -e "s/^CREATE TABLE '\([^']*\)'.*$/\1/" | sort)
	tables2=$(spatialite $originaldb2 '.schema' | grep "CREATE TABLE '" | sed -e "s/^CREATE TABLE '\([^']*\)'.*$/\1/" | sort)
}

ensure_table_names_identical() {
	# make sure the table names are the same
	if [ "$tables1" != "$tables2" ]
	then
		difference=$(echo -e "${tables1}\n${tables2}" | sort | uniq -u) # store the table names that occurred only once
		echo "ERROR: Tables are not the same. Look out for table(s):"
		echo "$difference"
		exit 4
	fi
}

ensure_schemas_identical() {
	#TODO: check for identical schemas
	# exit 5
	true
}

# clone the schema of a geometry table
clone_geometry_table_schema() {
		# assign variable names
		local srctable=$1
		local dsttable=$2
		local srcdb=$3
		local dstdb=$4

		# get schema
		schema=$(spatialite $srcdb ".schema $srctable" | grep 'CREATE TABLE' | grep "$srctable")

		# get geometry type (extract what is written after "GEOMETRY"
		geometry_type=$(echo "$schema" | sed -e 's/.*\"GEOMETRY\" \([A-Za-z]*\)[,)].*/\1/i')

		# adjust table names in 'CREATE TABLE ...' expressions by appending suffixes
		schema_adjusted=$(echo "$schema" | sed -e "s/${srctable}/${dsttable}/g")

		# create new tables in new file from adjusted schema
		spatialite $dstdb "$schema_adjusted" | grep -ve '^$'

		# read srid from the input file
		srid=$(spatialite $srcdb "SELECT srid FROM geometry_columns WHERE f_table_name='$srctable';" | grep -ve '^$')

		spatialite $dstdb "SELECT RecoverGeometryColumn('$dsttable', '$geometry_column', $srid, '$geometry_type');" | grep -ve '^$' | grep -v '^1$'
}

create_tmp_dbs_schemas() {
	mkdir -p $tmpdir

	while read -r table; do
		clone_geometry_table_schema $table ${table}${old_suffix} $originaldb1 ${tmpdbprefix}$table
		clone_geometry_table_schema $table ${table}${new_suffix} $originaldb2 ${tmpdbprefix}$table
	done <<< "$tables1"
}

create_output_db_schemas() {
	while read -r table; do
		clone_geometry_table_schema ${table}${old_suffix} ${table}${old_suffix} ${tmpdbprefix}$table $outputdb
		clone_geometry_table_schema ${table}${new_suffix} ${table}${new_suffix} ${tmpdbprefix}$table $outputdb
	done <<< "$tables1"
}

copy_table_content() {
	local srctable=$1
	local dsttable=$2
	local srcdb=$3
	local dstdb=$4

	spatialite $dstdb "ATTACH DATABASE '$srcdb' as input; INSERT INTO main.'$dsttable' SELECT * FROM input.'$srctable'" | grep -ve '^$'
}

fill_tmp_dbs() {
	while read -r table; do
		# fill oldtable
		copy_table_content $table ${table}${old_suffix} $originaldb1 ${tmpdbprefix}${table}

		# fill newtable
		copy_table_content $table ${table}${new_suffix} $originaldb2 ${tmpdbprefix}${table}
	done <<< "$tables1"
}

# check for different shifts and delete entries with identical geometries
check_shift() {
	local shift=$1
	local tmpdb=$2
	local oldtable=$3
	local newtable=$4

	echo shift=$shift

	# load ids of rows to be deleted
	spatialite $tmpdb "INSERT INTO '${oldtable}_doomed' SELECT ${oldtable}.ogc_fid FROM $oldtable INNER JOIN $newtable ON ${oldtable}.ogc_fid+($shift)=${newtable}.ogc_fid WHERE Equals(${oldtable}.${geometry_column},${newtable}.${geometry_column})" | grep -ve '^$'
	spatialite $tmpdb "INSERT INTO '${newtable}_doomed' SELECT ${newtable}.ogc_fid FROM $newtable INNER JOIN $oldtable ON ${oldtable}.ogc_fid+($shift)=${newtable}.ogc_fid WHERE Equals(${oldtable}.${geometry_column},${newtable}.${geometry_column})" | grep -ve '^$'

	local doomed_counter_old=$(spatialite $tmpdb "SELECT COUNT(*) FROM ${oldtable}_doomed")
	local doomed_counter_new=$(spatialite $tmpdb "SELECT COUNT(*) FROM ${newtable}_doomed")
	echo "  doomed_counter_old=$doomed_counter_old"
	echo "  doomed_counter_new=$doomed_counter_new"

	# delete duplicated rows
	spatialite $tmpdb "DELETE FROM '$oldtable' WHERE ogc_fid IN (SELECT id FROM ${oldtable}_doomed)" | grep -ve '^$'
	spatialite $tmpdb "DELETE FROM '$newtable' WHERE ogc_fid IN (SELECT id FROM ${newtable}_doomed)" | grep -ve '^$'

	# delete _doomed entries
	spatialite $tmpdb "DELETE FROM ${oldtable}_doomed" | grep -ve '^$'
	spatialite $tmpdb "DELETE FROM ${newtable}_doomed" | grep -ve '^$'
}
export -f check_shift

# open the temporary db file for the table given in the first argument
kill_duplicates() {
	local table=$1
	local tmpdb=${tmpdbprefix}$table
	local oldtable=${table}${old_suffix}
	local newtable=${table}${new_suffix}

	echo "Processing table '$table' ..."

	echo "  removing identical rows in copies of databases..."

	echo count in $oldtable before check_shift =
	spatialite $tmpdb "SELECT COUNT(*) FROM ${oldtable}" | grep -ve '^$'
	echo count in $newtable before check_shift =
	spatialite $tmpdb "SELECT COUNT(*) FROM ${newtable}" | grep -ve '^$'

	# create temporary tables for ids to be deleted
	spatialite $tmpdb "DROP TABLE IF EXISTS ${oldtable}_doomed;" | grep -ve '^$'
	spatialite $tmpdb "DROP TABLE IF EXISTS ${newtable}_doomed;" | grep -ve '^$'
	spatialite $tmpdb "CREATE TABLE ${oldtable}_doomed (id INTEGER);" | grep -ve '^$'
	spatialite $tmpdb "CREATE TABLE ${newtable}_doomed (id INTEGER);" | grep -ve '^$'

	for shift in 0 {1..10} {-1..-10} {11..100} {-11..-100}; do
		check_shift $shift $tmpdb $oldtable $newtable
		local oldtable_count=$(spatialite $tmpdb "SELECT COUNT(*) FROM ${oldtable}")
		local newtable_count=$(spatialite $tmpdb "SELECT COUNT(*) FROM ${newtable}")
		if [[ "$oldtable_count" == "0" || "$newtable_count" == "0" ]]; then
			break
		fi
	done

	echo count in $oldtable after check_shift =
	spatialite $tmpdb "SELECT COUNT(*) FROM ${oldtable}" | grep -ve '^$'
	echo count in $newtable after check_shift =
	spatialite $tmpdb "SELECT COUNT(*) FROM ${newtable}" | grep -ve '^$'

	# delete all duplicate geometries, independant of their ogc_fid (This is a last resort to find remaining duplicates. It is computationally expensive therefore the code above should find as many hits as possible.)

	spatialite $tmpdb "INSERT INTO '${oldtable}_doomed' SELECT ${oldtable}.ogc_fid FROM $oldtable INNER JOIN $newtable ON Equals(${oldtable}.${geometry_column},${newtable}.${geometry_column})" | grep -ve '^$'
	spatialite $tmpdb "INSERT INTO '${newtable}_doomed' SELECT ${newtable}.ogc_fid FROM $newtable INNER JOIN $oldtable ON Equals(${oldtable}.${geometry_column},${newtable}.${geometry_column})" | grep -ve '^$'

	local doomed_counter_old_in_kd=$(spatialite $tmpdb "SELECT COUNT(*) FROM ${oldtable}_doomed")
	local doomed_counter_new_in_kd=$(spatialite $tmpdb "SELECT COUNT(*) FROM ${newtable}_doomed")
	echo "  doomed_counter_old_in_kd=$doomed_counter_old_in_kd"
	echo "  doomed_counter_new_in_kd=$doomed_counter_new_in_kd"

	# delete duplicated rows
	spatialite $tmpdb "DELETE FROM '$oldtable' WHERE ogc_fid IN (SELECT id FROM ${oldtable}_doomed)" | grep -ve '^$'
	spatialite $tmpdb "DELETE FROM '$newtable' WHERE ogc_fid IN (SELECT id FROM ${newtable}_doomed)" | grep -ve '^$'

	# delete _doomed entries
	spatialite $tmpdb "DELETE FROM ${oldtable}_doomed" | grep -ve '^$'
	spatialite $tmpdb "DELETE FROM ${newtable}_doomed" | grep -ve '^$'

	echo count in $oldtable after where exists =
	spatialite $tmpdb "SELECT COUNT(*) FROM ${oldtable}" | grep -ve '^$'
	echo count in $newtable after where exists =
	spatialite $tmpdb "SELECT COUNT(*) FROM ${newtable}" | grep -ve '^$'

	# delete temporary tables
	spatialite $tmpdb "DROP TABLE '${oldtable}_doomed'" | grep -ve '^$'
	spatialite $tmpdb "DROP TABLE '${newtable}_doomed'" | grep -ve '^$'
}
export -f kill_duplicates

merge_tmp_dbs() {
	while read -r table; do
		local oldtable=${table}${old_suffix}
		local newtable=${table}${new_suffix}

		spatialite $outputdb "ATTACH DATABASE '${tmpdbprefix}$table' as input; INSERT INTO main.'$oldtable' SELECT * FROM input.'$oldtable'" | grep -ve '^$'

		spatialite $outputdb "ATTACH DATABASE '${tmpdbprefix}$table' as input; INSERT INTO main.'$newtable' SELECT * FROM input.'$newtable'" | grep -ve '^$'
	done <<< "$tables1"
}

cleanup() {
	# delete temporary dbs
	while read -r table; do
		rm -rf ${tmpdbprefix}$table
	done <<< "$tables1"
}

drop_empty_tables() {
	while read -r table; do
		local oldtable=${table}${old_suffix}
		local newtable=${table}${new_suffix}

		count_old=$(spatialite $outputdb "SELECT COUNT(*) FROM '$oldtable'" | grep -ve '^$')
		echo count_old = $count_old
		if [[ "$count_old" == "0" ]]; then
			echo dropping old table $oldtable
			spatialite $outputdb "SELECT DiscardGeometryColumn('$oldtable', '$geometry_column');" | grep -ve '^$'
			spatialite $outputdb "DROP TABLE '$oldtable'" | grep -ve '^$'
		fi

		count_new=$(spatialite $outputdb "SELECT COUNT(*) FROM '$newtable'" | grep -ve '^$')
		echo count_new = $count_new
		if [[ "$count_new" == "0" ]]; then
			echo dropping new table $newtable
			spatialite $outputdb "SELECT DiscardGeometryColumn('$newtable', '$geometry_column');" | grep -ve '^$'
			spatialite $outputdb "DROP TABLE '$newtable'" | grep -ve '^$'
		fi
	done <<< "$tables1"
}

# main program starts here ----------------

echo "Reading table names..."
read_table_names # write table names to variables tables1 and tables2

echo "Ensuring table names are identical..."
ensure_table_names_identical

ensure_schemas_identical

echo "Creating temporary database schemas..."
create_tmp_dbs_schemas

echo "Copying content into temporary databases..." # TODO: parallelisieren
fill_tmp_dbs

echo "Removing duplicate geometries..."
parallel $parallel_options kill_duplicates ::: $tables1

echo "Creating output database schemas..."
create_output_db_schemas

echo "Merging temporary database files..."
merge_tmp_dbs

echo "Dropping empty tables..."
drop_empty_tables

echo "Cleaning up..."
cleanup

echo "Finished."
