# OSM Inspector Address View

This is the backend of the new OSM Inspector Address view. OSM Inspector is a quality assurance service for OpenStreetMap data by the german consulting company Geofabrik GmbH and can be found at http://tools.geofabrik.de/osmi/?view=addresses. This software was written by Lukas Toggenburger as part of a project thesis for his master studies. It makes heavy use of libosmium (https://github.com/osmcode/libosmium) written by Jochen Topf.

I'd like to hear your feedback about this software. You can reach me per e-mail: lukas.toggenburgerXXhtwchur.ch (replace XX with @)

## Usage

Casual users interested in seeing address data may find the hosted version at http://tools.geofabrik.de/osmi/?view=addresses much more useful than running this software. A WxS is also available there, see http://wiki.openstreetmap.org/wiki/OSM_Inspector/WxS on how to use it.

If you are indeed interested in the backend, e.g. to see usage of libosmium in a bigger project, you have come to the right place. This software will take OSM data (XML or PBF; planet file or parts of it) data as input and produce SpatiaLite files (one for each layer) as output. The output format is easily changeable due to GDAL/OGR being utilized to hold and write data.

A file can be processed like this:

    ./osmi_addresses planet-latest.osm.pbf

By default an output directory called `osmi_sqlite_out` is created in the current directory (that means: not necessarily the directory the binary resides in). If a second parameter is given, the name of the output directory can be changed:

    ./osmi_addresses planet-latest.osm.pbf my-output-dir

An existing directory will not be overwritten, the software will abort instead.

In order to improve overall calculation speed, spatial indices are not calculated while writing the SpatiaLite files. Instead you are supposed to call

    ./create_spatial_indices.sh osmi_sqlite_out

to add them afterwards. 

The software was tested on Ubuntu but probably runs under other Unix variants (incl. Mac OS X) as well.

## Branches

You should find at least two branches in the Git repository:

- **master**: The software as it is running at http://tools.geofabrik.de/osmi/?view=addresses
- **dev**: The development version containing the newest features and fixes

## Compilation

### Dependencies

You will need a 64-bit system, a C++11 compiler and libosmium to compile the software. You can find libosmium at https://github.com/osmcode/libosmium .

Jochen updates libosmium quite often, so I cloned libosmium and made a soft-link to it (after making sure that `/usr/local/include` did not exist before):

    sudo ln -s /path/to/libosmium/include /usr/local/include

By this I can easily pull in new versions of libosmium.

libosmium itself comes with a list of prerequisites, which you can find at https://github.com/osmcode/libosmium#prerequisites.

On Ubuntu/Debian, you should be able to get all of libosmium's and osmi-addresses's dependencies with:

    sudo apt-get install clang libboost-program-options-dev libboost-dev libboost-filesystem-dev libgdal1-dev libsparsehash-dev libbz2-dev libosmpbf-dev libexpat1-dev libgeos++-dev sqlite3 spatialite-bin colordiff

### Compiling using make

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

## Debugging

### Using gdb

You can compile the software with debug information by activating the corresponding line at the top of `osmi/Makefile` (basically adding `-g` to the compile options).

Start gdb:

    gdb [-ex run] --args osmi_addresses myfile.osm.pbf

Use `-ex run` to immediately run the executable or do it in the gdb prompt (`(gdb)`):

    run

Attach the debugger after running a program (useful for analyzing deadlocks):

    sudo gdb osmi_addresses $(pgrep osmi_addresses)

Show the stack:

    bt

(This shows only the stack of one thread.)

Show all threads:

    info threads

Switch to another thread:

    thread 7

Show stacks of all threads:

    thread apply all bt

Quit gdb with:

    quit

### Known issues

Not having enough memory results in a segmentation fault. gdb will show a message like this:

```
Program received signal SIGSEGV, Segmentation fault.
google::sparsegroup<osmium::Location, (unsigned short)48, google::libc_allocator_with_realloc<osmium::Location> >::sparsegroup (this=0x1ba5d560, x=...)
    at /usr/include/sparsehash/sparsetable:992
    992	  sparsegroup(const sparsegroup& x) : group(0), settings(x.settings) {
```

This is also discussed in https://github.com/osmcode/libosmium/issues/23 . 
