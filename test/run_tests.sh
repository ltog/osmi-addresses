#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Usage: $0 DIRECTORY"
	exit
fi

error=0

# Note: ** is a shortcut for the name of the current table

# template:
#./test_engine.pl $1 "Description" "Query" "Table/File(.sqlite)" "=" "Expected result"; ((error+=$?))

./test_engine.pl $1 "Total number of entries in **" "SELECT COUNT(*) FROM **" "osmi_addresses_connection_line" "=" "115"; ((error+=$?))
./test_engine.pl $1 "Total number of entries in **" "SELECT COUNT(*) FROM **" "osmi_addresses_nearest_points"  "=" "111"; ((error+=$?))
./test_engine.pl $1 "Total number of entries in **" "SELECT COUNT(*) FROM **" "osmi_addresses_nearest_roads"   "=" "8";   ((error+=$?))
./test_engine.pl $1 "Total number of entries in **" "SELECT COUNT(*) FROM **" "osmi_addresses_nearest_areas"   "=" "1";   ((error+=$?))

./test_engine.pl $1 "Total number of entries in **" "SELECT COUNT(*) FROM **" "osmi_addresses_interpolation"   "=" "27";  ((error+=$?))

./test_engine.pl $1 "Total number of entries in **" "SELECT COUNT(*) FROM **" "osmi_addresses_nodes_with_addresses" "=" "133"; ((error+=$?))
./test_engine.pl $1 "Number of entries in ** with is_ip=false" "SELECT COUNT(*) FROM ** WHERE is_ip=0" "osmi_addresses_nodes_with_addresses" "=" "114"; ((error+=$?))
./test_engine.pl $1 "Number of entries in ** with is_ip=true" "SELECT COUNT(*) FROM ** WHERE is_ip=1" "osmi_addresses_nodes_with_addresses" "=" "19"; ((error+=$?))
./test_engine.pl $1 "Number of entries in ** with road_id not null" "SELECT COUNT(*) FROM ** WHERE road_id IS NOT NULL" "osmi_addresses_nodes_with_addresses" "=" "111"; ((error+=$?))

./test_engine.pl $1 "Total number of entries in **" "SELECT COUNT(*) FROM **" "osmi_addresses_ways_with_addresses" "=" "48"; ((error+=$?))

./test_engine.pl $1 "Total number of entries in **" "SELECT COUNT(*) FROM **" "osmi_addresses_buildings" "=" "0"; ((error+=$?))

./test_engine.pl $1 "Total number of entries in **" "SELECT COUNT(*) FROM **" "osmi_addresses_addrx_on_nonclosed_way" "=" "9"; ((error+=$?))

./test_engine.pl $1 "Total number of entries in **" "SELECT COUNT(*) FROM **" "osmi_addresses_entrances" "=" "6"; ((error+=$?))

./test_engine.pl $1 "Total number of entries in **" "SELECT COUNT(*) FROM **" "osmi_addresses_ways_with_postal_code" "=" "8"; ((error+=$?))

./test_engine.pl $1 "No elements outside of bbox of **" "**;8.783;8.793;47.25;47.2544" "osmi_addresses_connection_line" "outofbbox" "0"; ((error+=$?))
./test_engine.pl $1 "No elements outside of bbox of **" "**;8.783;8.793;47.25;47.2544" "osmi_addresses_interpolation" "outofbbox" "0"; ((error+=$?))
./test_engine.pl $1 "No elements outside of bbox of **" "**;8.783;8.793;47.25;47.2544" "osmi_addresses_nearest_points" "outofbbox" "0"; ((error+=$?))
./test_engine.pl $1 "No elements outside of bbox of **" "**;8.783;8.793;47.25;47.2544" "osmi_addresses_nearest_roads" "outofbbox" "0"; ((error+=$?))
./test_engine.pl $1 "No elements outside of bbox of **" "**;8.783;8.793;47.25;47.2544" "osmi_addresses_nearest_areas" "outofbbox" "0"; ((error+=$?))
./test_engine.pl $1 "No elements outside of bbox of **" "**;8.783;8.793;47.25;47.2544" "osmi_addresses_nodes_with_addresses" "outofbbox" "0"; ((error+=$?))
./test_engine.pl $1 "No elements outside of bbox of **" "**;8.783;8.793;47.25;47.2544" "osmi_addresses_ways_with_addresses" "outofbbox" "0"; ((error+=$?))
./test_engine.pl $1 "No elements outside of bbox of **" "**;8.783;8.793;47.25;47.2544" "osmi_addresses_buildings" "outofbbox" "0"; ((error+=$?))
./test_engine.pl $1 "No elements outside of bbox of **" "**;8.783;8.793;47.25;47.2544" "osmi_addresses_entrances" "outofbbox" "0"; ((error+=$?))

./test_engine.pl $1 "Number of 'no alphabetic part in addr:housenumber' errors in **" "SELECT COUNT(*) FROM ** WHERE error=\"no alphabetic part in addr:housenumber\";" "osmi_addresses_interpolation" "=" "2"; ((error+=$?))
./test_engine.pl $1 "Number of 'numeric parts of housenumbers not identical' errors in **" "SELECT COUNT(*) FROM ** WHERE error=\"numeric parts of housenumbers not identical\";" "osmi_addresses_interpolation" "=" "1"; ((error+=$?))
./test_engine.pl $1 "Number of 'endpoint has wrong format' errors in **" "SELECT COUNT(*) FROM ** WHERE error=\"endpoint has wrong format\";" "osmi_addresses_interpolation" "=" "4"; ((error+=$?))
./test_engine.pl $1 "Number of 'different tags on endpoints' errors in **" "SELECT COUNT(*) FROM ** WHERE error=\"different tags on endpoints\";" "osmi_addresses_interpolation" "=" "6"; ((error+=$?))
./test_engine.pl $1 "Number of 'needless interpolation' errors in **" "SELECT COUNT(*) FROM ** WHERE error=\"needless interpolation\";" "osmi_addresses_interpolation" "=" "3"; ((error+=$?))
./test_engine.pl $1 "Number of 'interpolation even but number odd' errors in **" "SELECT COUNT(*) FROM ** WHERE error=\"interpolation even but number odd\";" "osmi_addresses_interpolation" "=" "1"; ((error+=$?))
./test_engine.pl $1 "Number of 'interpolation odd but number even' errors in **" "SELECT COUNT(*) FROM ** WHERE error=\"interpolation odd but number even\";" "osmi_addresses_interpolation" "=" "1"; ((error+=$?))
./test_engine.pl $1 "Number of 'range too large' errors in **" "SELECT COUNT(*) FROM ** WHERE error=\"range too large\";" "osmi_addresses_interpolation" "=" "1"; ((error+=$?))
./test_engine.pl $1 "Number of 'unknown interpolation type' errors in **" "SELECT COUNT(*) FROM ** WHERE error=\"unknown interpolation type\";" "osmi_addresses_interpolation" "=" "2"; ((error+=$?))

./test_engine.pl $1 "Correct location of Karlsruher Strasse 8" "**;8.784524590652961;8.784524590652963;47.25353605657822;47.25353605657824" "osmi_addresses_nodes_with_addresses" "inbbox" "1"; ((error+=$?))

./test_engine.pl $1 "Number of entries in ** without value for 'entrance'" "SELECT COUNT(*) FROM ** WHERE entrance IS NULL" "osmi_addresses_entrances" "=" "1"; ((error+=$?))
./test_engine.pl $1 "Number of entries in ** with entrance=emergency" "SELECT COUNT(*) FROM ** WHERE entrance=\"emergency\"" "osmi_addresses_entrances" "=" "1"; ((error+=$?))

./test_engine.pl $1 "Minimum occurring length in **" "SELECT MIN(ST_Length(geometry, 0)) FROM **" "osmi_addresses_connection_line" "=" "1.34749447344977"; ((error+=$?))
./test_engine.pl $1 "Maximum occurring length in **" "SELECT MAX(ST_Length(geometry, 0)) FROM **" "osmi_addresses_connection_line" "=" "17.3055759386695"; ((error+=$?))

./test_engine.pl $1 "Minimum occurring length in **" "SELECT MIN(ST_Length(geometry, 0)) FROM **" "osmi_addresses_interpolation" "=" "2.81660826175927"; ((error+=$?))
./test_engine.pl $1 "Maximum occurring length in **" "SELECT MAX(ST_Length(geometry, 0)) FROM **" "osmi_addresses_interpolation" "=" "31.288447031296"; ((error+=$?))

./test_engine.pl $1 "Minimum occurring length in **" "SELECT MIN(ST_Length(geometry, 0)) FROM **" "osmi_addresses_nearest_roads" "=" "16.0156550129829"; ((error+=$?))
./test_engine.pl $1 "Maximum occurring length in **" "SELECT MAX(ST_Length(geometry, 0)) FROM **" "osmi_addresses_nearest_roads" "=" "139.896459880885"; ((error+=$?))

./test_engine.pl $1 "Minimum occurring length in **" "SELECT MIN(ST_Length(geometry, 0)) FROM **" "osmi_addresses_ways_with_postal_code" "=" "85.0653586130059"; ((error+=$?))
./test_engine.pl $1 "Maximum occurring length in **" "SELECT MAX(ST_Length(geometry, 0)) FROM **" "osmi_addresses_ways_with_postal_code" "=" "1932.24220904501"; ((error+=$?))

./test_engine.pl $1 "Minimum area in **" "SELECT MIN(ST_Area(ST_Transform(geometry, 2056))) FROM **" "osmi_addresses_ways_with_addresses" "=" "0.958406202423518"; ((error+=$?))
./test_engine.pl $1 "Maximum area in **" "SELECT MAX(ST_Area(ST_Transform(geometry, 2056))) FROM **" "osmi_addresses_ways_with_addresses" "=" "49.3336374588885"; ((error+=$?))

./test_engine.pl $1 "Minimum area in **" "SELECT MIN(ST_Area(ST_Transform(geometry, 2056))) FROM **" "osmi_addresses_nearest_areas" "=" "198.809381183972"; ((error+=$?))
./test_engine.pl $1 "Maximum area in **" "SELECT MAX(ST_Area(ST_Transform(geometry, 2056))) FROM **" "osmi_addresses_nearest_areas" "=" "198.809381183972"; ((error+=$?))

# color formatting according to http://misc.flogisoft.com/bash/tip_colors_and_formatting
if [[ "$error" -eq 0 ]]; then
	echo -e "\e[92m\e[1mPASS:\e[21m\e[90m All tests passed.\e[0m"
	exit 0
else
	echo -e "\e[91m\e[1mFAIL:\e[39m Test(s) failed.\e[0m"
	exit 1
fi

