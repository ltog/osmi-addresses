#ifndef ADDRXONNONCLOSEDWAYWRITER_HPP_
#define ADDRXONNONCLOSEDWAYWRITER_HPP_

class AddrXOnNonClosedWayWriter : public Writer {

public:

	AddrXOnNonClosedWayWriter(const std::string& dir_name) :
		Writer(dir_name, "osmi_addresses_addrx_on_nonclosed_way", USE_TRANSACTIONS, wkbLineString) {

		std::vector<field_config> field_configurations;
		field_configurations.push_back({"way_id",     OFTString, NO_WIDTH});
		field_configurations.push_back({"lastchange", OFTString, NO_WIDTH});
		create_fields(field_configurations);
	}

	void feed_way(const osmium::Way& way) {
		try {
			const char* street             = way.tags().get_value_by_key("addr:street");
			const char* housenumber        = way.tags().get_value_by_key("addr:housenumber");
			const char* full               = way.tags().get_value_by_key("addr:full");
			const char* conscriptionnumber = way.tags().get_value_by_key("addr:conscriptionnumber");
			const char* housename          = way.tags().get_value_by_key("addr:housename");
			const char* place              = way.tags().get_value_by_key("addr:place");
			const char* postcode           = way.tags().get_value_by_key("addr:postcode");
			const char* flats              = way.tags().get_value_by_key("addr:flats");
			const char* door               = way.tags().get_value_by_key("addr:door");
			const char* unit               = way.tags().get_value_by_key("addr:unit");
			const char* floor              = way.tags().get_value_by_key("addr:floor");
			const char* city               = way.tags().get_value_by_key("addr:city");
			const char* country            = way.tags().get_value_by_key("addr:country");
			const char* hamlet             = way.tags().get_value_by_key("addr:hamlet");
			const char* suburb             = way.tags().get_value_by_key("addr:suburb");
			const char* district           = way.tags().get_value_by_key("addr:district");
			const char* subdistrict        = way.tags().get_value_by_key("addr:subdistrict");
			const char* province           = way.tags().get_value_by_key("addr:province");
			const char* region             = way.tags().get_value_by_key("addr:region");
			const char* state              = way.tags().get_value_by_key("addr:state");

			if (!way.is_closed() && (street || housenumber || full ||
					conscriptionnumber || housename || place || postcode ||
					flats || door || unit || floor || city || country ||
					hamlet || suburb || district || subdistrict || province ||
					region || state)) {

				std::unique_ptr<OGRLineString> ogr_linestring = m_factory.create_linestring(way);
				OGRFeature* const feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());

				feature->SetGeometryDirectly(static_cast<OGRGeometry*>(ogr_linestring.release()));
				feature->SetField("way_id", static_cast<double>(way.id())); //TODO: node.id() is of type int64_t. is this ok?
				feature->SetField("lastchange", way.timestamp().to_iso().c_str());

				create_feature(feature);
			}

		} catch (osmium::geometry_error& e) {
			catch_geometry_error(e, way);
		}
	}

	void feed_node(const osmium::Node& /* node */) {

	}

	void feed_relation(const osmium::Relation& /* relation */) {

	}
};

#endif /* ADDRXONNONCLOSEDWAYWRITER_HPP_ */
