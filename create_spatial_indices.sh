#!/bin/bash

# Author:  Lukas Toggenburger
# Website: https://github.com/ltog/osmi-addresses

if [ $# -ne 1 ]; then
	echo "Usage: $0 sqlitefile"
	echo "Description: This tool calculates spatial indexes for geometries in a SpatiaLite database."
	exit
fi

export geometry_column='geometry' # name of the column where geometries are stored
export tmpdir="/tmp/${RANDOM}${RANDOM}${RANDOM}/" # will be deleted when the software finishes
export spatialite_init="PRAGMA synchronous=OFF; PRAGMA cache_size=-4000000; PRAGMA journal_mode=OFF; PRAGMA read_uncommitted=1;"
export originaldb=$1 # path to original db
export tables # table names from original db

handle_signals() {
	echo "Signal handling function was called. Going to clean up..."
	cleanup
	exit 1
}

trap handle_signals SIGINT SIGTERM

cleanup() {
	rm -rf $tmpdir
}

read_table_names() {
	# make note of the tables names (can't use ".tables" since its output has two columns)
	tables=$(spatialite $originaldb '.schema' | grep "CREATE TABLE '" | sed -e "s/^CREATE TABLE '\([^']*\)'.*$/\1/" | sort)
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
	spatialite $dstdb "$schema_adjusted" 2>&1 | grep -ve '^$' | fgrep -v -e 'the SPATIAL_REF_SYS table already contains some row(s)'

	# read srid from the input file
	srid=$(spatialite $srcdb "SELECT srid FROM geometry_columns WHERE f_table_name='$srctable';" | grep -ve '^$')

	spatialite $dstdb "SELECT RecoverGeometryColumn('$dsttable', '$geometry_column', $srid, '$geometry_type');" | grep -ve '^$' | grep -v '^1$'
}

create_tmp_db_schemas() {
	mkdir -p $tmpdir

	while read -r table; do
		clone_geometry_table_schema $table ${table} $originaldb ${tmpdir}$table
	done <<< "$tables"
}

copy_table_content() {
	local srctable=$1
	local dsttable=$2
	local srcdb=$3
	local dstdb=$4

	spatialite $dstdb "ATTACH DATABASE '$srcdb' as input; INSERT INTO main.'$dsttable' SELECT * FROM input.'$srctable'" | grep -ve '^$'
}
export -f copy_table_content

fill_tmp_dbs() {
	parallel $parallel_options copy_table_content {1} {1} $originaldb ${tmpdir}{1} ::: $tables
}

calculate_index() {
	local table=$1
	local db=$2

	spatialite $db "$spatialite_init SELECT CreateSpatialIndex('$table', '$geometry_column');" | grep -ve '1'
}
export -f calculate_index

calculate_indices() {
	parallel $parallel_options calculate_index {1} ${tmpdir}{1} ::: $tables
}

#while read -r table; do
#	csi="$csi SELECT CreateSpatialIndex('$table', 'GEOMETRY'); "
#done <<< "$tables"
#echo "csi=$csi"
#spatialite $1 "$spatialite_init $csi" > /dev/null

# main program starts here ----------------

echo "Reading table names..."
read_table_names # write table names to variable $tables

echo "Creating temporary database schemas..."
create_tmp_db_schemas

echo "Copying content into temporary databases..."
fill_tmp_dbs

echo "Calculate indices of temporary databases..."
calculate_indices

cleanup
