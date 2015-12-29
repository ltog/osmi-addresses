#!/bin/bash

# get the directory of the script (according to http://stackoverflow.com/a/246128 )
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

cd $DIR

# compile osmi-addresses
cd ../osmi
CXX=clang++ make
cd $DIR

# generate 'pos-osmi-testzone.osm'
./makeidpositive.sh osmi-testzone.osm

# remove old directory 'osmi_sqlite_out/'
rm -rf osmi_sqlite_out/

# read 'pos-osmi-testzone.osm' and generate directory 'osmi_sqlite_out/'
../osmi/osmi-addresses pos-osmi-testzone.osm

# create spatial indices for 'osmi_sqlite_out/'
../create_spatial_indices.sh osmi_sqlite_out/

# run tests on directory 'osmi_sqlite_out/'
./run_tests.sh osmi_sqlite_out/
