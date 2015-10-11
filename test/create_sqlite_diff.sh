#!/bin/bash

# Author:  Lukas Toggenburger
# Website: https://github.com/ltog/osmi-addresses

# Note on the output of spatialite: spatialite tries to be smart and sniffs the kind of output destination. If the output destination is a terminal, it will output some verbose information each time you run it. This is the reason why most (all?) calls to the spatialite tool in this software have a pipe after them.

description="This tool reads two directories containing spatialite files and creates an output sqlite file containing the differences (deleted and newly added rows) of the concerning tables. The resulting file contains two tables per input file/table (one for deleted, one for added content). This script works under the assumption that each input file of each input directory contains only one table and that its name is the same as the filename (without .sqlite suffix)."

parallel_options="--noswap"

export geometry_column='geometry' # name of the column where geometries are stored
export removed_suffix='_deleted'
export added_suffix='_added'
export tmpdir="/tmp/$(basename ${0})-$(date +%s%N)/" # will be deleted in cleanup() when the software finishes
export tables1 # table names from original db1
export tables2 # table names from original db2

# define signal handler
handle_signals() {
	echo "Signal handling function was called. Going to clean up..."
	cleanup
	exit 1
}
trap handle_signals SIGINT SIGTERM

ensure_schemas_identical() {
	# TODO: check for identical schemas (is #60)
	# exit 5
	true
}

# check if filenames in both directories are identical
# PRE: $1, $2 are paths to directories
ensure_filenames_identical() { # copied from compare_sqlite.sh
	local path1="$1"
	local path2="$2"
	filenames1=$(find "$path1" -maxdepth 1 -type f -printf '%f\n')
	filenames2=$(find "$path2" -maxdepth 1 -type f -printf '%f\n')
	filenamesdiff=$(echo -e "${filenames1}\n${filenames2}" | sort | uniq -u) # combine file lists, remove double entries
	if [[ ! -z "$filenamesdiff" ]]; then
		echo "ERROR: The files in the given directories differ. Look out for file(s):"
		echo "$filenamesdiff"
		exit 1
	fi
}

# clone the schema of a geometry table
clone_geometry_table_schema() {
	# assign variable names
	local srctable="$1"
	local dsttable="$2"
	local srcdb="$3"
	local dstdb="$4"

	# get schema
	schema=$(spatialite "$srcdb" ".schema $srctable" | grep 'CREATE TABLE' | grep "$srctable")

	# get geometry type (extract what is written after "GEOMETRY"
	geometry_type=$(echo "$schema" | sed -e 's/.*\"GEOMETRY\" \([A-Za-z]*\)[,)].*/\1/i')

	# adjust table names in 'CREATE TABLE ...' expressions by appending suffixes
	schema_adjusted=$(echo "$schema" | sed -e "s/${srctable}/${dsttable}/g")

	# create new tables in new file from adjusted schema
	spatialite "$dstdb" "$schema_adjusted" 2>&1 | grep -ve '^$' | fgrep -v -e 'the SPATIAL_REF_SYS table already contains some row(s)'

	# read srid from the input file
	srid=$(spatialite "$srcdb" "SELECT srid FROM geometry_columns WHERE f_table_name='$srctable';" | grep -ve '^$')

	spatialite "$dstdb" "SELECT RecoverGeometryColumn('$dsttable', '$geometry_column', $srid, '$geometry_type');" | grep -ve '^$' | grep -v '^1$'
}
export -f clone_geometry_table_schema

copy_table_content() {
	local srctable="$1"
	local dsttable="$2"
	local srcdb="$3"
	local dstdb="$4"

	spatialite "$dstdb" "ATTACH DATABASE '$srcdb' as input; INSERT INTO main.'$dsttable' SELECT * FROM input.'$srctable'" | grep -ve '^$'
}
export -f copy_table_content

# check for different shifts and delete entries with identical geometries
check_shift() {
	local shift="$1"
	local tmpdb="$2"
	local table_removed="$3"
	local table_added="$4"

	# load ids of rows to be deleted
	spatialite "$tmpdb" "INSERT INTO '${table_removed}_doomed' SELECT ${table_removed}.ogc_fid FROM $table_removed INNER JOIN $table_added ON ${table_removed}.ogc_fid+($shift)=${table_added}.ogc_fid WHERE Equals(${table_removed}.${geometry_column},${table_added}.${geometry_column})" | grep -ve '^$'
	spatialite "$tmpdb" "INSERT INTO '${table_added}_doomed' SELECT ${table_added}.ogc_fid FROM $table_added INNER JOIN $table_removed ON ${table_removed}.ogc_fid+($shift)=${table_added}.ogc_fid WHERE Equals(${table_removed}.${geometry_column},${table_added}.${geometry_column})" | grep -ve '^$'

	# delete duplicated rows
	spatialite "$tmpdb" "DELETE FROM '$table_removed' WHERE ogc_fid IN (SELECT id FROM ${table_removed}_doomed)" | grep -ve '^$'
	spatialite "$tmpdb" "DELETE FROM '$table_added' WHERE ogc_fid IN (SELECT id FROM ${table_added}_doomed)" | grep -ve '^$'

	# delete _doomed entries
	spatialite "$tmpdb" "DELETE FROM ${table_removed}_doomed" | grep -ve '^$'
	spatialite "$tmpdb" "DELETE FROM ${table_added}_doomed" | grep -ve '^$'
}
export -f check_shift

# open the temporary db file for the table given in the first argument
kill_duplicates() {
	local table="$1"
	local tmpdb="$2"
	local table_removed="$3"
	local table_added="$4"

	# create temporary tables for ids to be deleted
	spatialite "$tmpdb" "DROP TABLE IF EXISTS ${table_removed}_doomed;" | grep -ve '^$'
	spatialite "$tmpdb" "DROP TABLE IF EXISTS ${table_added}_doomed;" | grep -ve '^$'
	spatialite "$tmpdb" "CREATE TABLE ${table_removed}_doomed (id INTEGER);" | grep -ve '^$'
	spatialite "$tmpdb" "CREATE TABLE ${table_added}_doomed (id INTEGER);" | grep -ve '^$'

	for shift in 0 {1..10} {-1..-10} {11..100} {-11..-100}; do
		check_shift $shift "$tmpdb" $table_removed $table_added
		local table_removed_count=$(spatialite "$tmpdb" "SELECT COUNT(*) FROM ${table_removed}")
		local table_added_count=$(spatialite "$tmpdb" "SELECT COUNT(*) FROM ${table_added}")
		if [[ "$table_removed_count" == "0" || "$table_added_count" == "0" ]]; then
			break
		fi
	done

	# delete all duplicate geometries, independant of their ogc_fid (This is a last resort to find remaining duplicates. It is computationally expensive therefore the code above should find as many hits as possible.)

	spatialite "$tmpdb" "INSERT INTO '${table_removed}_doomed' SELECT ${table_removed}.ogc_fid FROM $table_removed INNER JOIN $table_added ON Equals(${table_removed}.${geometry_column},${table_added}.${geometry_column})" | grep -ve '^$'
	spatialite "$tmpdb" "INSERT INTO '${table_added}_doomed' SELECT ${table_added}.ogc_fid FROM $table_added INNER JOIN $table_removed ON Equals(${table_removed}.${geometry_column},${table_added}.${geometry_column})" | grep -ve '^$'

	# delete duplicated rows
	spatialite "$tmpdb" "DELETE FROM '$table_removed' WHERE ogc_fid IN (SELECT id FROM ${table_removed}_doomed)" | grep -ve '^$'
	spatialite "$tmpdb" "DELETE FROM '$table_added' WHERE ogc_fid IN (SELECT id FROM ${table_added}_doomed)" | grep -ve '^$'

	# delete _doomed entries
	spatialite "$tmpdb" "DELETE FROM ${table_removed}_doomed" | grep -ve '^$'
	spatialite "$tmpdb" "DELETE FROM ${table_added}_doomed" | grep -ve '^$'

	# delete temporary tables
	spatialite "$tmpdb" "DROP TABLE '${table_removed}_doomed'" | grep -ve '^$'
	spatialite "$tmpdb" "DROP TABLE '${table_added}_doomed'" | grep -ve '^$'
}
export -f kill_duplicates

cleanup() {
	rm -rf "$tmpdir"
}

drop_table_if_empty() {
	local table="$1"
	local dbfile="$2"

	local count=$(spatialite "$dbfile" "SELECT COUNT(*) FROM '$table'" | grep -ve '^$')
	if [[ "$count" == "0" ]]; then
		spatialite "$dbfile" "SELECT DiscardGeometryColumn('$table', '$geometry_column');" | grep -ve '^$' | fgrep -ve '1'
		spatialite "$dbfile" "DROP TABLE '$table'" | grep -ve '^$'
	fi
}
export -f drop_table_if_empty

check_number_of_args() {
	# make sure exactly 3 arguments are given
	if [ $1 -ne 3 ]; then
		echo "Usage: $0 old_dir new_dir out_dir"
		echo "Description: $description"
		exit 1
	fi
}

check_existencies() {
	local source_dir1="$1"
	local source_dir2="$2"
	local target_file="$3"

	if [ ! -d "$source_dir1" ]; then
		echo "ERROR: $source_dir1 is not a directory"
		exit 1
	fi

	if [ ! -d "$source_dir2" ]; then
		echo "ERROR: $source_dir2 is not a directory"
		exit 2
	fi

	if [ -e "$target_file" ]; then
		echo "ERROR: $target_file exists."
		exit 3
	fi
}

process_files() {
	local infile1="$1"
	local infile2="$2"
	local table=$(basename "$1" .sqlite) # we assume xy.sqlite files (in both directories) contain only one table named xy
	local tmpfile="${tmpdir}/$table"
	local table_removed="${table}${removed_suffix}"
	local table_added="${table}${added_suffix}"

	#echo "Ensuring schemas are identical..."
	#ensure_schemas_identical

	# TODO: create temporary directory

	echo "Creating temporary database schemas..."
	#clone_geometry_table_schema srctable dsttable srcdb dstdb
	clone_geometry_table_schema "$table" "$table_removed" "$infile1" "$tmpfile"
	clone_geometry_table_schema "$table" "$table_added"   "$infile2" "$tmpfile"

	echo "Copying content into temporary databases..."
	#copy_table_content srctable dsttable srcdb dstdb
	copy_table_content "$table" "$table_removed" "$infile1" "$tmpfile"
	copy_table_content "$table" "$table_added"   "$infile2" "$tmpfile"

	echo "Removing duplicate geometries..."
	#kill_duplicates table tmpdb table_removed table_added
	kill_duplicates "$table" "$tmpfile" "$table_removed" "$table_added"

	echo "Creating output database schemas..."
	clone_geometry_table_schema "$table_removed" "$table_removed" "$tmpfile" "$target_file"
	clone_geometry_table_schema "$table_added"   "$table_added"   "$tmpfile" "$target_file"

	echo "Merging temporary database files into output file..."
	copy_table_content "$table_removed" "$table_removed" "$tmpfile" "$target_file"
	copy_table_content "$table_added"   "$table_added"   "$tmpfile" "$target_file"

	echo "Dropping empty tables..."
	drop_table_if_empty "$table_removed" "$target_file"
	drop_table_if_empty "$table_added"   "$target_file"
}
export -f process_files

# main program starts here -----------------------------------------------------

mkdir -p "$tmpdir"

# make sure this script is started with the correct number of arguments
check_number_of_args $#

export source_dir1="$1"
export source_dir2="$2"
export target_file="$3"

# make sure the input directories DO exist and the output directory DOESN'T exist
check_existencies "$source_dir1" "$source_dir2" "$target_file"

ensure_filenames_identical "$source_dir1" "$source_dir2"

# aggregate list of files (copied from compare_sqlite.sh)
files1=$(find "$source_dir1" -maxdepth 1 -type f | sort)
files2=$(find "$source_dir2" -maxdepth 1 -type f | sort)

parallel $parallel_options --xapply process_files ::: "$files1" ::: "$files2"

echo "Cleaning up..."
cleanup

echo "Finished."
