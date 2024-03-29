#ifndef SECONDHANDLER_HPP_
#define SECONDHANDLER_HPP_

#include "AltTagList.hpp"
#include "Writer.hpp"
#include "InterpolationWriter.hpp"
#include "BuildingsWriter.hpp"
#include "AddrXOnNonClosedWayWriter.hpp"
#include "NodesWithAddressesWriter.hpp"
#include "WaysWithAddressesWriter.hpp"
#include "WaysWithPostalCodeWriter.hpp"
#include "EntrancesWriter.hpp"
#include "ConnectionLinePreprocessor.hpp"


class SecondHandler: public osmium::handler::Handler{

public:
	SecondHandler(
			const std::string& dir_name,
			node_set& addr_interpolation_node_set,
			name2highways_type& name2highways_area,
			name2highways_type& name2highways_nonarea,
			name2place_type& name2place_nody,
			name2place_type& name2place_wayy)
	: mp_addr_interpolation_node_set(addr_interpolation_node_set),
	  mp_name2highways_area(name2highways_area),
	  mp_name2highways_nonarea(name2highways_nonarea),
	  m_name2place_nody(name2place_nody),
	  m_name2place_wayy(name2place_wayy),
	  nodes_with_addresses_writer(dir_name),
	  connection_line_preprocessor(dir_name, mp_name2highways_area, mp_name2highways_nonarea, m_name2place_nody, m_name2place_wayy),
	  entrances_writer(dir_name),
	  interpolation_writer(dir_name, &m_addr_interpolation_node_map, nodes_with_addresses_writer, connection_line_preprocessor),
	  buildings_writer(dir_name),
	  addrx_on_nonclosed_way_writer(dir_name),
	  ways_with_addresses_writer(dir_name),
	  ways_with_postal_code_writer(dir_name)
	{
	}

	void node(const osmium::Node& node) {
		entrances_writer.feed_node(node);

		std::string road_id;
		std::string nody_place_id;
		std::string wayy_place_id;
		connection_line_preprocessor.process_node(node, road_id, nody_place_id, wayy_place_id); // overwrites IDs if matching street/place can be found
		nodes_with_addresses_writer.process_node(node,  road_id, nody_place_id, wayy_place_id);

		// save tags of nodes relevant for address interpolation
		std::set<osmium::unsigned_object_id_type>::iterator it = mp_addr_interpolation_node_set.find(node.id());
		if (it != mp_addr_interpolation_node_set.end()) {

			std::unordered_set<std::string> list_of_keys;
			list_of_keys.insert("addr:housenumber");
			list_of_keys.insert("addr:street");
			list_of_keys.insert("addr:postcode");
			list_of_keys.insert("addr:city");
			list_of_keys.insert("addr:country");
			list_of_keys.insert("addr:full");
			list_of_keys.insert("addr:place");
			AltTagList alt_tag_list(&(node.tags()), &list_of_keys);
			m_addr_interpolation_node_map.set(node.id(), alt_tag_list);
		}
	}


	void way(const osmium::Way& way) {
		try {
			if (m_geometry_helper.is_way_with_nonzero_length(way)) {
				interpolation_writer.feed_way(way);
				//buildings_writer.feed_way(way);
				addrx_on_nonclosed_way_writer.feed_way(way);
				ways_with_addresses_writer.feed_way(way);
				ways_with_postal_code_writer.feed_way(way);

				std::string road_id("");
				std::string nody_place_id("");
				std::string wayy_place_id("");
				connection_line_preprocessor.process_way(way, road_id, nody_place_id, wayy_place_id); // overwrites IDs if matching street/place can be found
				nodes_with_addresses_writer.process_way(way, road_id, nody_place_id, wayy_place_id);
			}
		} catch (const osmium::geometry_error&) {
			std::cerr << "Ignoring illegal geometry for way " << way.id() << std::endl;
		} catch (const osmium::invalid_location&) {
			std::cerr << "Ignoring dangling reference in way " << way.id() << std::endl;
		}
	}


private:
	node_set& mp_addr_interpolation_node_set;
	name2highways_type& mp_name2highways_area;
	name2highways_type& mp_name2highways_nonarea;
	node_map_type m_addr_interpolation_node_map;
	name2place_type& m_name2place_nody;
	name2place_type& m_name2place_wayy;
	GeometryHelper m_geometry_helper;

	NodesWithAddressesWriter nodes_with_addresses_writer;
	ConnectionLinePreprocessor connection_line_preprocessor;
	EntrancesWriter entrances_writer;
	InterpolationWriter interpolation_writer;
	BuildingsWriter buildings_writer;
	AddrXOnNonClosedWayWriter addrx_on_nonclosed_way_writer;
	WaysWithAddressesWriter ways_with_addresses_writer;
	WaysWithPostalCodeWriter ways_with_postal_code_writer;
};



#endif /* SECONDHANDLER_HPP_ */
