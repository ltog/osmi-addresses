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

You should find at least two branches in the GitHub repository:

- `master`: The development version containing the newest features and fixes
- `currently_running_on_geofabrik_server`: The software as it is running at http://tools.geofabrik.de/osmi/?view=addresses

## Compilation

### Dependencies

You will need a 64-bit system, a C++11 compiler and libosmium to compile the software. You can find libosmium at https://github.com/osmcode/libosmium .

Jochen updates libosmium quite often, so I cloned libosmium and made a soft-link to it (after making sure that `/usr/local/include` did not exist before):

    sudo ln -s /path/to/libosmium/include /usr/local/include

By this I can easily pull in new versions of libosmium.

libosmium itself comes with a list of prerequisites, which you can find at https://github.com/osmcode/libosmium#prerequisites.

On Ubuntu/Debian, you should be able to get all of libosmium's and osmi-addresses's dependencies with:

    sudo apt-get install clang libboost-dev libboost-filesystem-dev libgdal-dev libsparsehash-dev libbz2-dev libexpat1-dev sqlite3 spatialite-bin colordiff parallel realpath

### Compiling using make

Switch to the source directory:

    cd osmi

Compile using GCC:

    make

Or compile using clang:

    CXX=clang++ make

The compiled executable osmi is a standalone application and needs no installation.

If you have never used clang before, you should give it a try. It compiles (at least this software) slightly faster and gives better understandable error messages if you mess things up.

## Running the included test suite (after building)

switch to test dir

    cd test

remove the default output directory if it already exists

    rm -rf osmi-addresses_sqlite_out

make all the IDs in the testzone file positive, so that this tool can handle them

    ./makeidpositive.sh osmi-testzone.osm

process the testzone file

    ../osmi/osmi-addresses pos-osmi-testzone.osm

create spatial indices

    ../create_spatial_indices.sh osmi-addresses_sqlite_out/

check results

    ./run_tests.sh osmi-addresses_sqlite_out


## MapServer Setup

To test local changes, you can run a MapServer instance that serves the generated results as map overlays. The base layer tiles are loaded directly from the openstreetmap.org servers.

### Installation instructions

Install MapServer (Ubuntu):  
`sudo apt-get install mapserver-bin cgi-mapserver apache2 proj-data unifont`

Activate CGI:
`sudo a2enmod cgi; service apache2 restart`

Create a logfile with suitable permissions:  
`sudo mkdir /var/log/mapserver/ ; sudo touch /var/log/mapserver/addresses.log ; chmod a+w /var/log/mapserver/addresses.log`

Open `/usr/share/proj/epsg`, duplicate the line starting with `<3857>` and change the beginning to `<900913>` in one of the lines.

The `addresses.map` file is the configuration file as used on the production server running the full OSM inspector. The file `addresses.local.map` is a derived version that configures MapServer to show the local results. It is generated from `adresses.map` as follows:

- For each layer disable the lines starting with `TILEINDEX` or `TILEITEM`
- For each layer add a line `CONNECTION "X"` where X is the path to the .sqlite file
- For each layer add a line `DATA "X"` where X is the name of the table in the sqlite file, e.g. `osmi_addresses_nearest_roads` (the tables carry always the same name as the file)

Set the default location of the .map file: Add the line

    SetEnvIf Request_URI "/cgi-bin/mapserv" MS_MAPFILE=/absolute/path/to/addresses.local.map

into your Apache site config, e.g. into `/etc/apache2/sites-available/000-default.conf`. Restart apache with `service apache2 restart`.
MapServer will then access `addresses.local.map`, which refers to the data in `test/osmi-addresses_sqlite_out/`.

Copy the "viewer" directory to a place where it is served by the web server, e.g. `/var/www/html`. Then, open the contained index.html (e.g. `http://localhost/viewer/index.html`) in a browser to comfortably look at the (locally) detected address problems.

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

### Known issues

Not having enough memory results in a segmentation fault. gdb will show a message like this:

```
Program received signal SIGSEGV, Segmentation fault.
google::sparsegroup<osmium::Location, (unsigned short)48, google::libc_allocator_with_realloc<osmium::Location> >::sparsegroup (this=0x1ba5d560, x=...)
    at /usr/include/sparsehash/sparsetable:992
    992	  sparsegroup(const sparsegroup& x) : group(0), settings(x.settings) {
```

This is also discussed in https://github.com/osmcode/libosmium/issues/23 . 
