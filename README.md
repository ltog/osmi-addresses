# OSM Inspector Address view

This is the backend of the new OSM Inspector Address view. OSM Inspector is a quality assurance service for OpenStreetMap data by the german consulting company Geofabrik GmbH and can be found at http://tools.geofabrik.de/osmi. This software was written by Lukas Toggenburger as part of a project thesis for his master studies. It makes heavy use of libosmium (https://github.com/osmcode/libosmium) written by Jochen Topf.


## Usage

Casual users interested in seeing address data may find the hosted version at http://tools.geofabrik.de/osmi much more useful than running this software. A WxS is also available there, see http://wiki.openstreetmap.org/wiki/OSM_Inspector/WxS on how to use it.

If you are indeed interested in the backend, e.g. to see usage of libosmium in a bigger project, you have come to the right place. This software will take OSM data (XML or PBF; planet file or parts of it) data as input and produce a SpatiaLite file as output. The output format is easily changeable due to GDAL/OGR being utilized to hold and write data.

A file can be processed like this:

    ./osmi planet-latest.osm.pbf

By default a file called `out.sqlite` is written. If a second parameter is given, the name of the output file can be changed:

    ./osmi planet-latest.osm.pbf my-output-file.sqlite

An existing file will not be overwritten, the program will be aborted instead.

To improve overall calculation speed, spatial indices are not calculated while writing the SpatiaLite file. Instead you are supposed to call

    ./create_spatial_indices.sh out.sqlite

to add them afterwards. 

The software was tested on Ubuntu but probably runs under other Unix variants (incl. Mac OS X) as well.


## Compilation

You will need a 64-bit system, a C++11 compiler and libosmium to compile the software. libosmium itself comes with a list of prerequisites, which you can find at https://github.com/osmcode/libosmium#prerequisites.

On Ubuntu/Debian, you should be able to get all dependencies with:

    sudo apt-get install libboost-program-options-dev libboost-dev libgdal1-dev libsparsehash-dev libbz2-dev libosmpbf-dev libexpat1-dev libgeos++-dev sqlite3 spatialite-bin colordiff

Compile using GCC:

    make

Or compile using clang:

    CXX=clang++ make

The compiled executable osmi is a standalone application and needs no installation.

If you have never used clang before, you should give it a try. It compiles (at least this software) slightly faster and gives better understandable error messages if you mess things up.

## Eclipse setup (incomplete)

### Define the preprocessor symbol `OSMIUM_WITH_SPARSEHASH`

Project properties -> C/C++ General -> Paths and Symbols -> Tab: Symbols -> Languages C++ -> Add... -> Name: `OSMIUM_WITH_SPARSEHASH`, Value: 1, [X] Add to all configurations

## License

This software is available under the Boost Software License 1.0, see http://www.boost.org/LICENSE_1_0.txt.


## Author

Lukas Toggenburger ( firstname.lastname@htwchur.ch )


