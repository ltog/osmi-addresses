#ifndef SECONDHANDLER_HPP_
#define SECONDHANDLER_HPP_

#include "AltTagList.hpp"
#include "Writer.hpp"
#include "InterpolationWriter.hpp"
#include "BuildingsWriter.hpp"
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
			name2highways_type& name2highways_nonarea)
	: mp_addr_interpolation_node_set(addr_interpolation_node_set),
	  mp_name2highways_area(name2highways_area),
	  mp_name2highways_nonarea(name2highways_nonarea)
	{
		nodes_with_addresses_writer  = std::unique_ptr<NodesWithAddressesWriter>  (new NodesWithAddressesWriter(dir_name));
		connection_line_preprocessor = std::unique_ptr<ConnectionLinePreprocessor>(new ConnectionLinePreprocessor(dir_name, mp_name2highways_area, mp_name2highways_nonarea));
		entrances_writer             = std::unique_ptr<EntrancesWriter>           (new EntrancesWriter(dir_name));

		interpolation_writer         = std::unique_ptr<InterpolationWriter>     (new InterpolationWriter     (dir_name, &m_addr_interpolation_node_map, *(nodes_with_addresses_writer.get()), *(connection_line_preprocessor.get()) ));
		buildings_writer             = std::unique_ptr<BuildingsWriter>         (new BuildingsWriter         (dir_name));
		ways_with_addresses_writer   = std::unique_ptr<WaysWithAddressesWriter> (new WaysWithAddressesWriter (dir_name));
		ways_with_postal_code_writer = std::unique_ptr<WaysWithPostalCodeWriter>(new WaysWithPostalCodeWriter(dir_name));
	}


	~SecondHandler(){
		// call destructors to commit sqlite transactions
		entrances_writer.reset();
		interpolation_writer.reset();
		buildings_writer.reset();
		ways_with_addresses_writer.reset();
		ways_with_postal_code_writer.reset();
		nodes_with_addresses_writer.reset();
		connection_line_preprocessor.reset();
	}

	void node(const osmium::Node& node) {
		entrances_writer->feed_node(node);

		std::string road_id("");
		connection_line_preprocessor->process_node(node, road_id);

		nodes_with_addresses_writer->process_node(node, road_id);

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
				interpolation_writer->feed_way(way);
				//buildings_writer->feed_way(way);
				ways_with_addresses_writer->feed_way(way);
				ways_with_postal_code_writer->feed_way(way);

				std::string road_id("");
				connection_line_preprocessor->process_way(way, road_id);
				nodes_with_addresses_writer->process_way(way, road_id);
			}
		} catch (osmium::geometry_error&) {
			std::cerr << "Ignoring illegal geometry for way " << way.id() << std::endl;
		} catch (osmium::invalid_location&) {
			std::cerr << "Ignoring dangling reference in way " << way.id() << std::endl;
		}
	}


private:
	node_set& mp_addr_interpolation_node_set;
	name2highways_type& mp_name2highways_area;
	name2highways_type& mp_name2highways_nonarea;
	node_map_type m_addr_interpolation_node_map;
	GeometryHelper m_geometry_helper;

	std::unique_ptr<InterpolationWriter> interpolation_writer;
	std::unique_ptr<BuildingsWriter> buildings_writer;
	std::unique_ptr<NodesWithAddressesWriter> nodes_with_addresses_writer;
	std::unique_ptr<WaysWithAddressesWriter> ways_with_addresses_writer;
	std::unique_ptr<WaysWithPostalCodeWriter> ways_with_postal_code_writer;
	std::unique_ptr<EntrancesWriter> entrances_writer;
	std::unique_ptr<ConnectionLinePreprocessor> connection_line_preprocessor;
};



#endif /* SECONDHANDLER_HPP_ */
