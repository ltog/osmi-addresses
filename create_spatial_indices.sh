#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Usage: $0 sqlitefile"
	exit
fi

# http://www.sqlite.org/pragma.html#pragma_cache_size
# http://www.sqlite.org/pragma.html#pragma_journal_mode journal_mode=MEMORY is maybe a small bit faster than =OFF
prefix="PRAGMA synchronous=OFF; PRAGMA cache_size=-4000000; PRAGMA journal_mode=OFF; PRAGMA read_uncommitted=1;"


tables=$(spatialite $1 ".schema" | grep "CREATE TABLE '" | sed -e "s/^CREATE TABLE '\([^']*\)'.*$/\1/")

# can't call .schema after usage of $prefix for some unknown reason...
# tables=$(spatialite $1 "$prefix .schema" | grep "CREATE TABLE '" | sed -e "s/^CREATE TABLE '\([^']*\)'.*$/\1/")

while read -r table; do
	# starting from version 4.2.0 we can call spatialite with the option '-silent' which reduces output
	# see e.g. https://stackoverflow.com/questions/23579001/how-do-i-make-the-spatialite-banner-go-away-under-django-manage-py
	spatialite $1 "$prefix BEGIN; SELECT CreateSpatialIndex('$table', 'GEOMETRY'); COMMIT;" > /dev/null
done <<< "$tables"
