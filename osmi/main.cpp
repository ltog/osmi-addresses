#include <iostream>
#include <getopt.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#include <ogr_api.h>
#include <ogrsf_frmts.h>
#pragma GCC diagnostic pop

#include <stdlib.h>
#include <set>
#include <unordered_set>
#include <string>

// usually you only need one or two of these
#include <osmium/index/map/dummy.hpp>
#include <osmium/index/map/sparse_table.hpp>
#include <osmium/index/map/stl_map.hpp>
#include <osmium/index/map/mmap_vector_anon.hpp>

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

	if (argc < 2 || argc > 3) {
		std::cerr << "Usage: " << argv[0] << " INFILE [OUTFILE]" << std::endl;
		exit(1);
	}

	std::string input_filename(argv[1]);
	std::string output_filename;
	if (argc != 3) {
		output_filename = std::string("out.sqlite");
	} else {
		output_filename = std::string(argv[2]);
	}

	{
	// from http://stackoverflow.com/questions/1647557/ifstream-how-to-tell-if-specified-file-doesnt-exist/3071528#3071528
	struct stat file_info;
	if (stat(output_filename.c_str(), &file_info) == 0) {
		std::cerr << "ERROR: Output file '" << output_filename << "' exists. Aborting..." << std::endl;
		exit(1);
	}
	}

	std::set<osmium::unsigned_object_id_type> addr_interpolation_node_set;
	name2highways_type name2highway;

	index_pos_type index_pos;
	index_neg_type index_neg;
	location_handler_type location_handler(index_pos, index_neg);
	location_handler.ignore_errors();

	MemHelper mem_helper;
	//mem_helper.start();
	{
	osmium::io::Reader reader(input_filename);

	FirstHandler first_handler(addr_interpolation_node_set, name2highway);

	osmium::apply(reader, location_handler, first_handler);
	reader.close();
	}
	//mem_helper.stop();

	osmium::io::Reader reader2(input_filename);
	SecondHandler second_handler(output_filename, addr_interpolation_node_set, name2highway);
	osmium::apply(reader2, location_handler, second_handler);
	reader2.close();

	google::protobuf::ShutdownProtobufLibrary();

	std::cout << std::endl;
	mem_helper.print_max();

	std::cout << "\nsoftware finished properly\n" << std::endl;
	return 0;
}

