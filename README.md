# OSM Inspector Address View

This is the backend of the new OSM Inspector Address view. OSM Inspector is a quality assurance service for OpenStreetMap data by the german consulting company Geofabrik GmbH and can be found at http://tools.geofabrik.de/osmi/?view=addresses. This software was written by Lukas Toggenburger as part of a project thesis for his master studies. It makes heavy use of libosmium (https://github.com/osmcode/libosmium) written by Jochen Topf.

I'd like to hear your feedback about this software. You can reach me per e-mail: lukas.toggenburgerXXhtwchur.ch (replace XX with @)

## License

This software is available under the Boost Software License 1.0, see http://www.boost.org/LICENSE_1_0.txt.

## Usage

Casual users interested in seeing address data may find the hosted version at http://tools.geofabrik.de/osmi/?view=addresses much more useful than running this software. A WxS is also available there, see http://wiki.openstreetmap.org/wiki/OSM_Inspector/WxS on how to use it.

If you are indeed interested in the backend, e.g. to see usage of libosmium in a bigger project, you have come to the right place. This software will take OSM data (XML or PBF; planet file or parts of it) data as input and produce SpatiaLite files (one for each layer) as output. The output format is easily changeable due to GDAL/OGR being utilized to hold and write data.

A file can be processed like this:

    ./osmi-addresses planet-latest.osm.pbf

By default an output directory called `osmi-addresses_sqlite_out` is created in the current directory (that means: not necessarily the directory the binary resides in). If a second parameter is given, the name of the output directory can be changed:

    ./osmi-addresses planet-latest.osm.pbf my-output-dir

An existing directory will not be overwritten, the software will abort instead.

In order to improve overall calculation speed, spatial indices are not calculated while writing the SpatiaLite files. Instead you are supposed to call

    ./create_spatial_indices.sh osmi-addresses_sqlite_out

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

### Make Eclipse understand C++11 syntax

In Eclipse go to: Window -> Preferences -> C/C++ -> Build -> Settings -> Discovery -> CDT GCC Built-in Compiler Settings [Shared]

You should see

    ${COMMAND} -E -P -v -dD "${INPUTS}"

or

    ${COMMAND} ${FLAGS} -E -P -v -dD "${INPUTS}"

Change it to:

    ${COMMAND} -std=c++11 ${FLAGS} -D__cplusplus=201103L -E -P -v -dD "${INPUTS}"

Find further information here: https://www.eclipse.org/forums/index.php/mv/msg/490066/1068004/#msg_1068004

### Include GDAL

Project properties -> C/C++ General -> Paths and Symbols -> Tab: Includes -> Languages: GNU C++ -> Add... -> `/usr/include/gdal` ([X] Add to all configurations)

### Set C++11 flag

Project properties -> C/C++ Build -> Settings -> Tab: Tool Settings -> GCC C++ Compiler -> Miscellaneous -> Other flags: Change `-c -fmessage-length=0` to `-c -fmessage-length=0 -std=c++11`.

### Define the preprocessor symbol `OSMIUM_WITH_SPARSEHASH`

Project properties -> C/C++ General -> Paths and Symbols -> Tab: Symbols -> Languages C++ -> Add... -> Name: `OSMIUM_WITH_SPARSEHASH`, Value: 1, [X] Add to all configurations

## MapServer setup

### Installation instructions

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

The `addresses.map` file is the configuration file as used on the server. The file `addresses.local.map` is adjusted to locally view MapServer's output. It was generated doing the following changes:

- For each layer disable the lines starting with `TILEINDEX` or `TILEITEM`
- For each layer add a line `CONNECTION "X"` where X is the path to the .sqlite file
- For each layer add a line `DATA "X"` where X is the name of the table in the sqlite file, e.g. `osmi_addresses_nearest_roads` (the tables carry always the same name as the file)

Set the default location of the .map file: Add the line

    SetEnvIf Request_URI "/cgi-bin/mapserv" MS_MAPFILE=X

(with X being the absolute path to the file `addresses.local.map`) into your apache site config, e.g. into `/etc/apache2/sites-available/000-default.conf`. Restart apache with `service apache2 restart`.

To comfortably look (locally) at the MapServer output, open `viewer/index.html` that accesses `addresses.local.map`, which in turn accesses the data in `test/osmi-addresses_sqlite_out/`.

### Debugging MapServer

#### Error message: MS_DEFAULT_MAPFILE_PATTERN validation failed

    msLoadMap(): Regular expression error. MS_DEFAULT_MAPFILE_PATTERN validation failed. msEvalRegex(): Regular expression error. String failed expression test.

Make sure that your .map file actually ends with `.map`. See also http://gis.stackexchange.com/a/11444

#### MapServer logfile

The logfile's location is configured using `CONFIG MS_ERRORFILE`. Try `/var/log/mapserver/addresses.log`

#### Contents of layer not showing up

Have you included the desired layer in your HTTP request?

## Information for developers

### Call hierarchy

A given input file will be processed in two passes. This happens by the classes `FirstHandler` and `SecondHandler`. The first pass is necessary to build a lookup structure to get street geometries based on the street's name. The second pass accesses this structure and writes the .sqlite files.

Here is a hierarchical overview of calls/accesses:

- `FirstHandler`
  - `way()`
    - `addr_interpolation_node_set`
    - `highway_lookup_type` -> `name2highway_area`
    - `highway_lookup_type` -> `name2highway_nonarea`
- `SecondHandler`
  - `node()`
    - `ConnectionLinePreprocessor.process_node()`
      - *
    - `EntrancesWriter.feed_node()`
    - `NodesWithAddressesWriter.process_node()`
  - `way()`
    - `BuildingsWriter.feed_way()`
    - `ConnectionLinePreprocessor.process_way()` (reads buildings, etc. with `addr:street` tag)
      - *
    - `InterpolationWriter.feed_way()` (reads interpolation lines with `addr:interpolation` tag)
      - `ConnectionLinePreprocessor.process_interpolated_node()`
        - *
      - `NodesWithAddressesWriter.process_interpolated_node()`
    - `NodesWithAddressesWriter.process_way()`
    - `WaysWithAddressesWriter.feed_way()`
    - `WaysWithPostalCodeWriter.feed_way()`
- * = `ConnectionLinePreprocessor.handle_connection_line()`
  - `ConnectionLineWriter.write_line()`
  - `NearestRoadsWriter.write_road()` XOR `NearestAreasWriter.write_area()`
  - `NearestPointsWriter.write_point()`

## Debugging

### Using gdb

You can compile the software with debug information by activating the corresponding line at the top of `osmi/Makefile` (basically adding `-g` to the compile options).

Start gdb:

    gdb [-ex run] --args osmi-addresses myfile.osm.pbf

Use `-ex run` to immediately run the executable or do it in the gdb prompt (`(gdb)`):

    run

Attach the debugger after running a program (useful for analyzing deadlocks):

    sudo gdb osmi-addresses $(pgrep osmi-addresses)

Show the stack:

    bt

(This shows only the stack of one thread.)

Show all threads:

    info threads

Switch to another thread:

    thread 7

Show stacks of all threads:

    thread apply all bt

Show where the debugged program stopped:

    list

Go *over* function call:

    next

Go *into* function call:

    step

Quit gdb with:

    quit

A nice tutorial: http://www.unknownroad.com/rtfm/gdbtut/gdbtoc.html

### Known issues

Not having enough memory results in a segmentation fault. gdb will show a message like this:

```
Program received signal SIGSEGV, Segmentation fault.
google::sparsegroup<osmium::Location, (unsigned short)48, google::libc_allocator_with_realloc<osmium::Location> >::sparsegroup (this=0x1ba5d560, x=...)
    at /usr/include/sparsehash/sparsetable:992
    992	  sparsegroup(const sparsegroup& x) : group(0), settings(x.settings) {
```

This is also discussed in https://github.com/osmcode/libosmium/issues/23 . 
