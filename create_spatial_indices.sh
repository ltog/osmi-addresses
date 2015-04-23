#!/bin/bash

# http://www.sqlite.org/pragma.html#pragma_cache_size
# http://www.sqlite.org/pragma.html#pragma_journal_mode journal_mode=MEMORY is maybe a small bit faster than =OFF
spatialite_pragmas="PRAGMA synchronous=OFF; PRAGMA cache_size=-4000000; PRAGMA journal_mode=OFF; PRAGMA read_uncommitted=1;"

parallel_options="--noswap"

if [ $# -ne 1 ]; then
	echo "Usage: $0 DIR_WITH_SQLITE_FILES"
	exit 1
fi

#TODO: check if $1 is a directory

create_spatial_indices() {
	local tables=$(spatialite $1 ".schema" | grep "CREATE TABLE '" | sed -e "s/^CREATE TABLE '\([^']*\)'.*$/\1/")
	
	# can't call .schema after usage of $spatialite_pragmas for some unknown reason...
	# tables=$(spatialite $1 "$spatialite_pragmas .schema" | grep "CREATE TABLE '" | sed -e "s/^CREATE TABLE '\([^']*\)'.*$/\1/")
	
	while read -r table; do
		# starting from version 4.2.0 we can call spatialite with the option '-silent' which reduces output
		# see e.g. https://stackoverflow.com/questions/23579001/how-do-i-make-the-spatialite-banner-go-away-under-django-manage-py
		spatialite $1 "$spatialite_pragmas BEGIN; SELECT CreateSpatialIndex('$table', 'GEOMETRY'); COMMIT;" > /dev/null
	done <<< "$tables"
}
export -f create_spatial_indices

parallel $parallel_options create_spatial_indices ::: $(realpath $1/*.sqlite)
