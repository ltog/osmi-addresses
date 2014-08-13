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

	osmium::geom::OGRFactory<> m_factory {};
	OGRDataSource* m_data_source;

public:
	SecondHandler(
			const std::string& filename,
			node_set& addr_interpolation_node_set,
			name2highways_type& name2highways)
	: mp_addr_interpolation_node_set(addr_interpolation_node_set),
	  mp_name2highways(name2highways)
	{
		node_map_type m_addr_interpolation_node_map;

		OGRRegisterAll();

		const std::string driver_name = std::string("SQLite");
		OGRSFDriver* driver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(driver_name.c_str());
		if (!driver) {
			std::cerr << driver_name << " driver not available." << std::endl;
			exit(1);
		}

		CPLSetConfigOption("OGR_SQLITE_SYNCHRONOUS", "OFF");
		CPLSetConfigOption("OGR_SQLITE_SYNCHRONOUS", "FALSE");
		CPLSetConfigOption("OGR_SQLITE_CACHE", "1024"); // size in MB; see http://gdal.org/ogr/drv_sqlite.html
		const char* options[] = { "SPATIALITE=TRUE", nullptr };
		m_data_source = driver->CreateDataSource(filename.c_str(), const_cast<char**>(options));
		if (!m_data_source) {
			std::cerr << "Creation of output file '" << filename << "' failed." << std::endl;
			exit(1);
		}

		OGRSpatialReference sparef;
		sparef.SetWellKnownGeogCS("WGS84");

	}


	~SecondHandler(){
		OGRDataSource::DestroyDataSource(m_data_source);
		OGRCleanupAll();
	}

	void before_nodes() {
		nodes_with_addresses_writer  = std::unique_ptr<NodesWithAddressesWriter>  (new NodesWithAddressesWriter(m_data_source));
		connection_line_preprocessor = std::unique_ptr<ConnectionLinePreprocessor>(new ConnectionLinePreprocessor(m_data_source, mp_name2highways));
		entrances_writer             = std::unique_ptr<EntrancesWriter>           (new EntrancesWriter(m_data_source));
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


	void after_nodes() {
		// call destructors to commit sqlite transactions
		entrances_writer.reset();

		std::cout << "node processing in SecondHandler finished" << std::endl;
	}


	void before_ways() {
		interpolation_writer         = std::unique_ptr<InterpolationWriter>     (new InterpolationWriter     (m_data_source, &m_addr_interpolation_node_map, *(nodes_with_addresses_writer.get()), *(connection_line_preprocessor.get()) ));
		buildings_writer             = std::unique_ptr<BuildingsWriter>         (new BuildingsWriter         (m_data_source));
		ways_with_addresses_writer   = std::unique_ptr<WaysWithAddressesWriter> (new WaysWithAddressesWriter (m_data_source));
		ways_with_postal_code_writer = std::unique_ptr<WaysWithPostalCodeWriter>(new WaysWithPostalCodeWriter(m_data_source));
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
		}
	}

	void after_ways() {
		// call destructors to commit sqlite transactions
		interpolation_writer.reset();
		buildings_writer.reset();
		ways_with_addresses_writer.reset();
		ways_with_postal_code_writer.reset();
		nodes_with_addresses_writer.reset();
		connection_line_preprocessor.reset();

		std::cout << "way processing in SecondHandler finished" << std::endl;
	}



private:
	node_set& mp_addr_interpolation_node_set;
	name2highways_type& mp_name2highways;
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
