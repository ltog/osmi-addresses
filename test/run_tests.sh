#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Usage: $0 DIRECTORY"
	exit
fi

# Note: ** is a shortcut for the name of the current table

# template:
#./test_engine.pl $1 "Description" "Query" "Table/File(.sqlite)" "=" "Expected result"

./test_engine.pl $1 "Total number of entries in **" "SELECT COUNT(*) FROM **" "osmi_addresses_connection_line" "=" "101"
./test_engine.pl $1 "Total number of entries in **"  "SELECT COUNT(*) FROM **" "osmi_addresses_nearest_points" "=" "97"
./test_engine.pl $1 "Total number of entries in **"   "SELECT COUNT(*) FROM **" "osmi_addresses_nearest_roads" "=" "7"

./test_engine.pl $1 "Total number of entries in **" "SELECT COUNT(*) FROM **" "osmi_addresses_interpolation" "=" "27"

./test_engine.pl $1 "Total number of entries in **" "SELECT COUNT(*) FROM **" "osmi_addresses_nodes_with_addresses" "=" "117"
./test_engine.pl $1 "Number of entries in ** with is_ip=false" "SELECT COUNT(*) FROM ** WHERE is_ip=0" "osmi_addresses_nodes_with_addresses" "=" "98"
./test_engine.pl $1 "Number of entries in ** with is_ip=true" "SELECT COUNT(*) FROM ** WHERE is_ip=1" "osmi_addresses_nodes_with_addresses" "=" "19"
./test_engine.pl $1 "Number of entries in ** with road_id not null" "SELECT COUNT(*) FROM ** WHERE road_id IS NOT NULL" "osmi_addresses_nodes_with_addresses" "=" "97"

./test_engine.pl $1 "Total number of entries in **" "SELECT COUNT(*) FROM **" "osmi_addresses_ways_with_addresses" "=" "35"

./test_engine.pl $1 "Total number of entries in **" "SELECT COUNT(*) FROM **" "osmi_addresses_buildings" "=" "0"

./test_engine.pl $1 "Total number of entries in **" "SELECT COUNT(*) FROM **" "osmi_addresses_entrances" "=" "6"

./test_engine.pl $1 "Total number of entries in **" "SELECT COUNT(*) FROM **" "osmi_addresses_ways_with_postal_code" "=" "8"

./test_engine.pl $1 "No elements outside of bbox of **" "**;8.783;8.793;47.25;47.2544" "osmi_addresses_connection_line" "outofbbox" "0"
./test_engine.pl $1 "No elements outside of bbox of **" "**;8.783;8.793;47.25;47.2544" "osmi_addresses_interpolation" "outofbbox" "0"
./test_engine.pl $1 "No elements outside of bbox of **" "**;8.783;8.793;47.25;47.2544" "osmi_addresses_nearest_points" "outofbbox" "0"
./test_engine.pl $1 "No elements outside of bbox of **" "**;8.783;8.793;47.25;47.2544" "osmi_addresses_nearest_roads" "outofbbox" "0"
./test_engine.pl $1 "No elements outside of bbox of **" "**;8.783;8.793;47.25;47.2544" "osmi_addresses_nodes_with_addresses" "outofbbox" "0"
./test_engine.pl $1 "No elements outside of bbox of **" "**;8.783;8.793;47.25;47.2544" "osmi_addresses_ways_with_addresses" "outofbbox" "0"
./test_engine.pl $1 "No elements outside of bbox of **" "**;8.783;8.793;47.25;47.2544" "osmi_addresses_buildings" "outofbbox" "0"
./test_engine.pl $1 "No elements outside of bbox of **" "**;8.783;8.793;47.25;47.2544" "osmi_addresses_entrances" "outofbbox" "0"

./test_engine.pl $1 "Number of 'no alphabetic part in addr:housenumber' errors in **" "SELECT COUNT(*) FROM ** WHERE error=\"no alphabetic part in addr:housenumber\";" "osmi_addresses_interpolation" "=" "2"
./test_engine.pl $1 "Number of 'numeric parts of housenumbers not identical' errors in **" "SELECT COUNT(*) FROM ** WHERE error=\"numeric parts of housenumbers not identical\";" "osmi_addresses_interpolation" "=" "1"
./test_engine.pl $1 "Number of 'endpoint has wrong format' errors in **" "SELECT COUNT(*) FROM ** WHERE error=\"endpoint has wrong format\";" "osmi_addresses_interpolation" "=" "4"
./test_engine.pl $1 "Number of 'different tags on endpoints' errors in **" "SELECT COUNT(*) FROM ** WHERE error=\"different tags on endpoints\";" "osmi_addresses_interpolation" "=" "6"
./test_engine.pl $1 "Number of 'needless interpolation' errors in **" "SELECT COUNT(*) FROM ** WHERE error=\"needless interpolation\";" "osmi_addresses_interpolation" "=" "3"
./test_engine.pl $1 "Number of 'interpolation even but number odd' errors in **" "SELECT COUNT(*) FROM ** WHERE error=\"interpolation even but number odd\";" "osmi_addresses_interpolation" "=" "1"
./test_engine.pl $1 "Number of 'interpolation odd but number even' errors in **" "SELECT COUNT(*) FROM ** WHERE error=\"interpolation odd but number even\";" "osmi_addresses_interpolation" "=" "1"
./test_engine.pl $1 "Number of 'range too large' errors in **" "SELECT COUNT(*) FROM ** WHERE error=\"range too large\";" "osmi_addresses_interpolation" "=" "1"
./test_engine.pl $1 "Number of 'unknown interpolation type' errors in **" "SELECT COUNT(*) FROM ** WHERE error=\"unknown interpolation type\";" "osmi_addresses_interpolation" "=" "2"

./test_engine.pl $1 "Correct location of Karlsruher Strasse 8" "**;8.784524590652961;8.784524590652963;47.25353605657822;47.25353605657824" "osmi_addresses_nodes_with_addresses" "inbbox" "1"

./test_engine.pl $1 "Number of entries in ** without value for 'entrance'" "SELECT COUNT(*) FROM ** WHERE entrance IS NULL" "osmi_addresses_entrances" "=" "1"
./test_engine.pl $1 "Number of entries in ** with entrance=emergency" "SELECT COUNT(*) FROM ** WHERE entrance=\"emergency\"" "osmi_addresses_entrances" "=" "1"
