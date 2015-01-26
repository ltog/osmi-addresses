#!/bin/bash

# get the directory of the script (according to http://stackoverflow.com/a/246128 )
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

cd $DIR

# compile osmi
cd ../osmi
CXX=clang++ make
cd $DIR

# generate 'pos-osmi-testzone.osm'
./makeidpositive.sh osmi-testzone.osm

# remove old file 'out.sqlite'
rm out.sqlite

# read 'pos-osmi-testzone.osm' and generate file 'out.sqlite'
../osmi/osmi pos-osmi-testzone.osm

# create spatial indices for 'out.sqlite'
../create_spatial_indices.sh out.sqlite

# run tests on 'out.sqlite'
./run_tests.sh out.sqlite
