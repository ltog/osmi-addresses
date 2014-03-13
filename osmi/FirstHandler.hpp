#ifndef FIRSTHANDLER_HPP_
#define FIRSTHANDLER_HPP_

#include "GeometryHelper.hpp"

class FirstHandler : public osmium::handler::Handler {
	osmium::geom::OGRFactory m_factory {};

public:
	FirstHandler(
			node_set& addr_interpolation_node_set,
			name2highways_type& name2highway)
		: m_addr_interpolation_node_set(addr_interpolation_node_set),
		  m_name2highways(name2highway) {

	}

	~FirstHandler() {

	}

	void after_nodes() {
		std::cout << "node processing in FirstHandler finished" << std::endl;
	}


	void way(const osmium::Way& way) {

		try {
			if (way.nodes().size() >= 2) {
				// save node ids of first/last node of interpolation
				const char* interpolation = way.tags().get_value_by_key("addr:interpolation");
				if (interpolation) {
					const osmium::unsigned_object_id_type node_begin_id = m_geometry_helper.get_first_node_id(way);
					const osmium::unsigned_object_id_type node_end_id   = m_geometry_helper.get_last_node_id(way);

					m_addr_interpolation_node_set.insert(node_begin_id);
					m_addr_interpolation_node_set.insert(node_end_id);
				}


				// -------------------------------------------------------------------

				const char* highway = way.tags().get_value_by_key("highway");
				const char* name    = way.tags().get_value_by_key("name");
				if (highway && name){

					highway_lookup_type mylookup;

					mylookup.way_id    = way.id();
					mylookup.lat       = m_geometry_helper.get_lat_estimate(way);
					mylookup.lon       = m_geometry_helper.get_lon_estimate(way);
					mylookup.compr_way = std::unique_ptr<CompressedWay>(new CompressedWay(m_factory.create_linestring(way)));

					m_name2highways.insert(name2highways_element_type(std::string(name), std::move(mylookup)));
				}
			}
		} catch (osmium::geom::geometry_error&) {
			std::cerr << "Ignoring illegal geometry for way " << way.id() << std::endl;
		}
	}

	void after_ways() {
		std::cout << "way processing in FirstHandler finished" << std::endl;
	}

private:
	node_set& m_addr_interpolation_node_set;
	name2highways_type& m_name2highways;
	GeometryHelper m_geometry_helper;
};


#endif /* FIRSTHANDLER_HPP_ */
