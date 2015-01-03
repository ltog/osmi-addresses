#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Usage: $0 sqlitefile"
	exit
fi

# http://www.sqlite.org/pragma.html#pragma_cache_size
# http://www.sqlite.org/pragma.html#pragma_journal_mode journal_mode=MEMORY is maybe a small bit faster than =OFF
prefix="PRAGMA synchronous=OFF; PRAGMA cache_size=-4000000; PRAGMA journal_mode=OFF; PRAGMA read_uncommitted=1;"

t1=osmi_addresses_buildings
t2=osmi_addresses_connection_line
t3=osmi_addresses_interpolation
t4=osmi_addresses_nearest_points
t5=osmi_addresses_nearest_roads
t6=osmi_addresses_nodes_with_addresses
t7=osmi_addresses_ways_with_addresses
t8=osmi_addresses_ways_with_postal_code
t9=osmi_addresses_entrances

for table in $t1 $t2 $t3 $t4 $t5 $t6 $t7 $t8 $t9 

do
	# starting from version 4.2.0 we can call spatialite with the option '-silent' which reduces output
	# see e.g. https://stackoverflow.com/questions/23579001/how-do-i-make-the-spatialite-banner-go-away-under-django-manage-py
	spatialite $1 "$prefix BEGIN; SELECT CreateSpatialIndex('$table', 'GEOMETRY'); COMMIT;" > /dev/null
done

