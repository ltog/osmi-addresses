#include <iostream>
#include <getopt.h>

#include <ogr_api.h>
#include <ogrsf_frmts.h>

#include <stdlib.h>
#include <set>
#include <unordered_set>
#include <string>

// usually you only need one or two of these
#include <osmium/index/map/dummy.hpp>
#include <osmium/index/map/sparse_mem_table.hpp>
#include <osmium/index/map/sparse_mem_map.hpp>
#include <osmium/index/map/sparse_mmap_array.hpp>

#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/visitor.hpp>

#include <osmium/geom/ogr.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>

#include <geos/util/IllegalArgumentException.h>
#include <geos/util/GEOSException.h>

#include "main.hpp"
#include "AltTagList.hpp"
#include "FirstHandler.hpp"
#include "SecondHandler.hpp"
#include "ConnectionLineWriter.hpp"
#include "MemHelper.hpp"

int main(int argc, char* argv[]) {

	OGRRegisterAll();

	if (argc < 2 || argc > 3) {
		std::cerr << "Usage: " << argv[0] << " INFILE [OUTFILE_DIR]" << std::endl;
		exit(1);
	}

	std::string input_filename(argv[1]);
	std::string output_dirname;
	if (argc != 3) {
		output_dirname = std::string("osmi-addresses_sqlite_out");
	} else {
		output_dirname = std::string(argv[2]);
	}

	{
	// from http://stackoverflow.com/questions/1647557/ifstream-how-to-tell-if-specified-file-doesnt-exist/3071528#3071528
	struct stat dir_info;
	if (stat(output_dirname.c_str(), &dir_info) == 0) {
		std::cerr << "ERROR: Output directory '" << output_dirname << "' exists. Aborting..." << std::endl;
		exit(1);
	}
	}

	std::set<osmium::unsigned_object_id_type> addr_interpolation_node_set;
	name2highways_type name2highway_area;
	name2highways_type name2highway_nonarea;
	name2place_type name2place_nody;
	name2place_type name2place_wayy;

	index_pos_type index_pos;
	index_neg_type index_neg;
	location_handler_type location_handler(index_pos, index_neg);
	location_handler.ignore_errors();

	MemHelper mem_helper;
	//mem_helper.start();
	{
	osmium::io::Reader reader(input_filename);

	FirstHandler first_handler(
			addr_interpolation_node_set,
			name2highway_area,
			name2highway_nonarea,
			name2place_nody,
			name2place_wayy);

	osmium::apply(reader, location_handler, first_handler);
	reader.close();
	}
	//mem_helper.stop();

	osmium::io::Reader reader2(input_filename);
	SecondHandler second_handler(
			output_dirname,
			addr_interpolation_node_set,
			name2highway_area,
			name2highway_nonarea,
			name2place_nody,
			name2place_wayy);

	osmium::apply(reader2, location_handler, second_handler);
	reader2.close();

	OGRCleanupAll();

	std::cout << std::endl;
	mem_helper.print_max();

	std::cout << "\nsoftware finished properly\n" << std::endl;
	return 0;
}

