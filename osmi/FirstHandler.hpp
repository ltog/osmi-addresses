#ifndef FIRSTHANDLER_HPP_
#define FIRSTHANDLER_HPP_

#include "GeometryHelper.hpp"

class FirstHandler : public osmium::handler::Handler {
	osmium::geom::OGRFactory<> m_factory {};

public:
	FirstHandler(
			node_set& addr_interpolation_node_set,
			name2highways_type& name2highway_area,
			name2highways_type& name2highway_nonarea,
			name2place_type& name2place_nody,
			name2place_type& name2place_wayy)
		: m_addr_interpolation_node_set(addr_interpolation_node_set),
		  m_name2highways_area(name2highway_area),
		  m_name2highways_nonarea(name2highway_nonarea),
		  m_name2place_nody(name2place_nody),
		  m_name2place_wayy(name2place_wayy) {

	}

	~FirstHandler() {

	}

	void node(const osmium::Node& node) {
		const char* place = node.tags().get_value_by_key("place");
		const char* name = node.tags().get_value_by_key("name");

		// look out for places
		if (place && name) {
			place_lookup_type mylookup; // TODO: use unique_ptr?
			mylookup.obj_id   = node.id();
			mylookup.ogrpoint = m_factory.create_point(node);

			m_name2place_nody.insert(name2place_element_type(name, std::move(mylookup)));
		}
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
					for (const std::string& streetname : streetnames) {
						highway_lookup_type mylookup;

						mylookup.way_id = way.id();

						double_bbox bbox = m_geometry_helper.get_bbox(way);
						mylookup.bbox_n = m_geometry_helper.lat2int16(bbox.north, INCREMENT_WHEN_ROUNDING);
						mylookup.bbox_e = m_geometry_helper.lon2int16(bbox.east,  INCREMENT_WHEN_ROUNDING);
						mylookup.bbox_s = m_geometry_helper.lat2int16(bbox.south, DECREMENT_WHEN_ROUNDING);
						mylookup.bbox_w = m_geometry_helper.lon2int16(bbox.west,  DECREMENT_WHEN_ROUNDING);

						const char* area = way.tags().get_value_by_key("area");

						mylookup.compr_way = std::unique_ptr<CompressedWay>(new CompressedWay(m_factory.create_linestring(way)));

						if (area && ( (!strcmp(area, "yes")) || (!strcmp(area, "true")) ) && way.is_closed()) {
							m_name2highways_area.insert(name2highways_element_type(streetname, std::move(mylookup)));
						} else {
							m_name2highways_nonarea.insert(name2highways_element_type(streetname, std::move(mylookup)));
						}

					}
				}

				// -------------------------------------------------------------------

				const char* place = way.tags().get_value_by_key("place");
				const char* name  = way.tags().get_value_by_key("name");

				// look out for places
				// TODO: remove duplicate code copied from void FirstHandler.node(const osmium::Node& node)
				if (place && name && way.is_closed()) {
				    std::unique_ptr<OGRPoint> centroid_point= m_geometry_helper.centroid(way);
					place_lookup_type mylookup {way.id(), std::move(centroid_point)}; // TODO: use unique_ptr?

					m_name2place_wayy.emplace(name2place_element_type(name, std::move(mylookup)));
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
	name2place_type& m_name2place_nody;
	name2place_type& m_name2place_wayy;
	GeometryHelper m_geometry_helper;

	std::set<std::string> get_streetnames(const osmium::TagList& taglist) {
		std::set<std::string> streetnames; // std::set checks for duplicates when inserting elements. See http://stackoverflow.com/a/3451045
		std::vector<std::string> keys = {"name", "name:left", "name:right", "alt_name", "official_name", "short_name", "ref"};
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
