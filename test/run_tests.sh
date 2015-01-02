#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Usage: $0 filename"
	exit
fi

# template:
#./test_engine.pl $1 "" "" "=" ""

./test_engine.pl $1 "Total number of entries in osmi_addresses_connection_line" "SELECT COUNT(*) FROM osmi_addresses_connection_line" "=" "82"
./test_engine.pl $1 "Total number of entries in osmi_addresses_nearest_points"  "SELECT COUNT(*) FROM osmi_addresses_nearest_points"  "=" "82"
./test_engine.pl $1 "Total number of entries in osmi_addresses_nearest_roads"   "SELECT COUNT(*) FROM osmi_addresses_nearest_roads"   "=" "5"

./test_engine.pl $1 "Total number of entries in osmi_addresses_interpolation" "SELECT COUNT(*) FROM osmi_addresses_interpolation" "=" "24"

./test_engine.pl $1 "Total number of entries in osmi_addresses_nodes_with_addresses" "SELECT COUNT(*) FROM osmi_addresses_nodes_with_addresses" "=" "96"
./test_engine.pl $1 "Number of entries in osmi_addresses_nodes_with_addresses with is_ip=false" "SELECT COUNT(*) FROM osmi_addresses_nodes_with_addresses WHERE is_ip=0" "=" "80"
./test_engine.pl $1 "Number of entries in osmi_addresses_nodes_with_addresses with is_ip=true" "SELECT COUNT(*) FROM osmi_addresses_nodes_with_addresses WHERE is_ip=1" "=" "16"
./test_engine.pl $1 "Number of entries in osmi_addresses_nodes_with_addresses with road_id not null" "SELECT COUNT(*) FROM osmi_addresses_nodes_with_addresses WHERE road_id IS NOT NULL" "=" "82"

./test_engine.pl $1 "Total number of entries in osmi_addresses_ways_with_addresses" "SELECT COUNT(*) FROM osmi_addresses_ways_with_addresses" "=" "29"

./test_engine.pl $1 "Total number of entries in osmi_addresses_buildings" "SELECT COUNT(*) FROM osmi_addresses_buildings" "=" "0"

./test_engine.pl $1 "Total number of entries in osmi_addresses_entrances" "SELECT COUNT(*) FROM osmi_addresses_entrances" "=" "6"

./test_engine.pl $1 "Total number of entries in osmi_addresses_ways_with_postal_code" "SELECT COUNT(*) FROM osmi_addresses_ways_with_postal_code" "=" "8"

./test_engine.pl $1 "No elements outside of bbox of osmi_addresses_connection_line" "osmi_addresses_connection_line;8.783;8.793;47.25;47.2544" "outofbbox" "0"
./test_engine.pl $1 "No elements outside of bbox of osmi_addresses_interpolation" "osmi_addresses_interpolation;8.783;8.793;47.25;47.2544" "outofbbox" "0"
./test_engine.pl $1 "No elements outside of bbox of osmi_addresses_nearest_points" "osmi_addresses_nearest_points;8.783;8.793;47.25;47.2544" "outofbbox" "0"
./test_engine.pl $1 "No elements outside of bbox of osmi_addresses_nearest_roads" "osmi_addresses_nearest_roads;8.783;8.793;47.25;47.2544" "outofbbox" "0"
./test_engine.pl $1 "No elements outside of bbox of osmi_addresses_nodes_with_addresses" "osmi_addresses_nodes_with_addresses;8.783;8.793;47.25;47.2544" "outofbbox" "0"
./test_engine.pl $1 "No elements outside of bbox of osmi_addresses_ways_with_addresses" "osmi_addresses_ways_with_addresses;8.783;8.793;47.25;47.2544" "outofbbox" "0"
./test_engine.pl $1 "No elements outside of bbox of osmi_addresses_buildings" "osmi_addresses_buildings;8.783;8.793;47.25;47.2544" "outofbbox" "0"
./test_engine.pl $1 "No elements outside of bbox of osmi_addresses_entrances" "osmi_addresses_entrances;8.783;8.793;47.25;47.2544" "outofbbox" "0"

./test_engine.pl $1 "Number of 'endpoint has wrong format' errors in osmi_addresses_interpolation" "SELECT COUNT(*) FROM osmi_addresses_interpolation WHERE error=\"endpoint has wrong format\";" "=" "5"
./test_engine.pl $1 "Number of 'different tags on endpoints' errors in osmi_addresses_interpolation" "SELECT COUNT(*) FROM osmi_addresses_interpolation WHERE error=\"different tags on endpoints\";" "=" "6"
./test_engine.pl $1 "Number of 'needless interpolation' errors in osmi_addresses_interpolation" "SELECT COUNT(*) FROM osmi_addresses_interpolation WHERE error=\"needless interpolation\";" "=" "3"
./test_engine.pl $1 "Number of 'interpolation even but number odd' errors in osmi_addresses_interpolation" "SELECT COUNT(*) FROM osmi_addresses_interpolation WHERE error=\"interpolation even but number odd\";" "=" "1"
./test_engine.pl $1 "Number of 'interpolation odd but number even' errors in osmi_addresses_interpolation" "SELECT COUNT(*) FROM osmi_addresses_interpolation WHERE error=\"interpolation odd but number even\";" "=" "1"
./test_engine.pl $1 "Number of 'range too large' errors in osmi_addresses_interpolation" "SELECT COUNT(*) FROM osmi_addresses_interpolation WHERE error=\"range too large\";" "=" "1"
./test_engine.pl $1 "Number of 'unknown interpolation type' errors in osmi_addresses_interpolation" "SELECT COUNT(*) FROM osmi_addresses_interpolation WHERE error=\"unknown interpolation type\";" "=" "2"

./test_engine.pl $1 "Correct location of Karlsruher Strasse 8" "osmi_addresses_nodes_with_addresses;8.784524590652961;8.784524590652963;47.25353605657822;47.25353605657824" "inbbox" "1"

./test_engine.pl $1 "Number of entries in osmi_addresses_entrances without value for 'entrance'" "SELECT COUNT(*) FROM osmi_addresses_entrances WHERE entrance IS NULL" "=" "1"
./test_engine.pl $1 "Number of entries in osmi_addresses_entrances with entrance=emergency" "SELECT COUNT(*) FROM osmi_addresses_entrances WHERE entrance=\"emergency\"" "=" "1"
