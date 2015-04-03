#ifndef FIRSTHANDLER_HPP_
#define FIRSTHANDLER_HPP_

#include "GeometryHelper.hpp"

class FirstHandler : public osmium::handler::Handler {
	osmium::geom::OGRFactory<> m_factory {};

public:
	FirstHandler(
			node_set& addr_interpolation_node_set,
			name2highways_type& name2highway_area,
			name2highways_type& name2highway_nonarea)
		: m_addr_interpolation_node_set(addr_interpolation_node_set),
		  m_name2highways_area(name2highway_area),
		  m_name2highways_nonarea(name2highway_nonarea){

	}

	~FirstHandler() {

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

				if (highway){
					std::set<std::string> streetnames = get_streetnames(way.tags());

					// TODO: reuse mylookup struct
					for (std::string streetname : streetnames) {
						highway_lookup_type mylookup;

						mylookup.way_id = way.id();

						double_bbox bbox = m_geometry_helper.get_bbox(way);
						mylookup.bbox_n = m_geometry_helper.lat2int16(bbox.north, INCREMENT_WHEN_ROUNDING);
						mylookup.bbox_e = m_geometry_helper.lon2int16(bbox.east,  INCREMENT_WHEN_ROUNDING);
						mylookup.bbox_s = m_geometry_helper.lat2int16(bbox.south, DECREMENT_WHEN_ROUNDING);
						mylookup.bbox_w = m_geometry_helper.lon2int16(bbox.west,  DECREMENT_WHEN_ROUNDING);

						const char* area = way.tags().get_value_by_key("area");

						mylookup.compr_way = std::unique_ptr<CompressedWay>(new CompressedWay(m_factory.create_linestring(way)));

						if (area && ( (!strcmp(area, "yes")) || (!strcmp(area, "true")) ) )
						{
							m_name2highways_area.insert(name2highways_element_type(streetname, std::move(mylookup)));
						} else {
							m_name2highways_nonarea.insert(name2highways_element_type(streetname, std::move(mylookup)));
						}

					}
				}
			}
		} catch (osmium::geometry_error&) {
			std::cerr << "Ignoring illegal geometry for way " << way.id() << std::endl;
        } catch (osmium::invalid_location&) {
            std::cerr << "Ignoring dangling reference in way " << way.id() << std::endl;
        }

	}

private:
	node_set& m_addr_interpolation_node_set;
	name2highways_type& m_name2highways_area;
	name2highways_type& m_name2highways_nonarea;
	GeometryHelper m_geometry_helper;

	std::set<std::string> get_streetnames(const osmium::TagList& taglist) {
		std::set<std::string> streetnames; // std::set checks for duplicates when inserting elements. See http://stackoverflow.com/a/3451045
		std::vector<std::string> keys = {"name", "name:left", "name:right", "alt_name", "official_name", "name_1"};
		for (std::string key : keys) {
			const char* value = taglist.get_value_by_key(key.c_str());
			if (value) {
				streetnames.insert(value);
			}
		}
		return streetnames;
	}
};


#endif /* FIRSTHANDLER_HPP_ */
