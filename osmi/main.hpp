#ifndef MAIN_HPP_
#define MAIN_HPP_

#include "CompressedWay.hpp"

typedef osmium::index::map::Dummy<osmium::unsigned_object_id_type, osmium::Location> index_neg_type;
typedef osmium::index::map::SparseMemTable<osmium::unsigned_object_id_type, osmium::Location> index_pos_type;

typedef osmium::handler::NodeLocationsForWays<index_pos_type, index_neg_type> location_handler_type;

class AltTagList;
typedef osmium::index::map::SparseMemTable<osmium::unsigned_object_id_type, AltTagList> node_map_type;

typedef std::set<osmium::unsigned_object_id_type> node_set;

#pragma pack(push, 1)
struct highway_lookup_type {
	std::unique_ptr<CompressedWay> compr_way;
	osmium::object_id_type         way_id;
	int16_t                        bbox_n, bbox_e, bbox_s, bbox_w;
	std::string                    lastchange;
};

struct place_lookup_type {
	osmium::object_id_type    id;
	std::unique_ptr<OGRPoint> ogrpoint;
};
#pragma pack(pop)

struct double_bbox {
	double north; // max lat
	double east;  // max lon
	double south; // min lat
	double west;  // min lon
};

enum class object_type : int {
	node_object,
	way_object,
	relation_object,
	interpolated_node_object
};

constexpr bool INCREMENT_WHEN_ROUNDING = true;
constexpr bool DECREMENT_WHEN_ROUNDING = false;

typedef std::pair<std::string, highway_lookup_type> name2highways_element_type;
typedef std::multimap<const std::string, highway_lookup_type> name2highways_type;

typedef std::pair<std::string, place_lookup_type> name2place_element_type;
typedef std::multimap<const std::string, place_lookup_type> name2place_type;

#endif /* MAIN_HPP_ */
