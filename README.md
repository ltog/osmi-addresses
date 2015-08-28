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

    sudo apt-get install libboost-program-options-dev libboost-dev libboost-filesystem-dev libgdal1-dev libsparsehash-dev libbz2-dev libosmpbf-dev libexpat1-dev libgeos++-dev sqlite3 spatialite-bin colordiff

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

## MapServer setup

Install MapServer (Ubuntu):  
`sudo apt-get install mapserver-bin cgi-mapserver apache2 proj-data unifont`

Activate CGI:  
`sudo a2enmod cgi; service apache2 restart`

Create a logfile with suitable permissions:  
`sudo mkdir /var/log/mapserver/ ; sudo touch /var/log/mapserver/addresses.log ; chmod a+w /var/log/mapserver/addresses.log`

Create a softlink to the proj files:  
`sudo mkdir -p /srv/tools/svn-tools/osm-inspector ; sudo ln -s /usr/share/proj/ /srv/tools/svn-tools/osm-inspector/`

Open `/usr/share/proj/epsg`, duplicate the line starting with `<3857>` and change the beginning to `<900913>` in one of the lines.

Create a softlink for the symbolset and fontset files:  
`cd X` where X is the directory containing the files `symbolset` and `fontset`
`cd ..`
`ln -s mapserver/symbolset`
`ln -s mapserver/fontset`

Create softlinks for entrance images (in the same directory as above):  
`ln -s mapserver/entrance-emergency.png`
`ln -s mapserver/entrance-exit.png`
`ln -s mapserver/entrance-main.png`
`ln -s mapserver/entrance-service.png`
`ln -s mapserver/entrance-yes.png`
`ln -s mapserver/entrance-deprecated.png`

Change the file `addresses.map`:  
- For each layer disable the lines starting with `TILEINDEX` or `TILEITEM`
- For each layer add a line `CONNECTION "X"` where X is the path to the .sqlite file
- For each layer add a line `DATA "X"` where X is the name of a table in the sqlite file, e.g. `osmi_addresses_nearest_roads` (you can derive the table name from the name of the .shp file in the `TILEINDEX ...` line)

Request string:  
`http://localhost/cgi-bin/mapserv?LAYERS=nearest_roads%2Cconnection_lines%2Cnearest_points%2Cinterpolation%2Cbuildings%2Cbuildings_with_addresses%2Cnodes_with_addresses_interpolated%2Cnodes_with_addresses_defined%2Cpostal_code%2Cinterpolation_errors%2Cno_addr_street%2Cstreet_not_found&FORMAT=image%2Fpng%3B%20mode%3D24bit&PROJECTION=EPSG%3A900913&DISPLAYPROJECTION=EPSG%3A4326&TRANSPARENT=TRUE&SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&STYLES=&SRS=EPSG%3A900913&BBOX=928402.87147211,5959885.5572013,938540.33234703,5962035.3486215&WIDTH=1061&HEIGHT=225&map=X` where X is the full path to the cloned .map file

## Author

Lukas Toggenburger ( firstname.lastname@htwchur.ch )


