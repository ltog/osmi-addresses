#ifndef WAYSWITHPOSTALCODEWRITER_HPP_
#define WAYSWITHPOSTALCODEWRITER_HPP_

class WaysWithPostalCodeWriter : public Writer {

public:

	WaysWithPostalCodeWriter(OGRDataSource* data_source) :
		Writer(data_source, "osmi_addresses_ways_with_postal_code", USE_TRANSACTIONS, wkbLineString) {

		std::vector<field_config> field_configurations;
		field_configurations.push_back({"way_id",     OFTString,  NO_WIDTH});
		field_configurations.push_back({"lastchange", OFTString,  NO_WIDTH});
		field_configurations.push_back({"postalcode", OFTString,  NO_WIDTH});

		create_fields(field_configurations);
	}

	void feed_node(const osmium::Node& /* node */) {

	}

	void feed_way(const osmium::Way& way) {
		try {
			const char* postalcode = way.tags().get_value_by_key("postal_code");
			if (postalcode) {

				std::unique_ptr<OGRLineString> ogr_linestring = m_factory.create_linestring(way);
				OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());

				feature->SetGeometry(static_cast<OGRGeometry*>(ogr_linestring.get()));
				feature->SetField("way_id", static_cast<double>(way.id())); //TODO: way.id() is of type int64_t. is this ok?
				feature->SetField("lastchange", way.timestamp().to_iso().c_str());
				feature->SetField("postalcode", postalcode);

				create_feature(feature);
			}
		}
		catch (osmium::geom::geometry_error& e) {
			catch_geometry_error(e, way);
		}
	}

	void feed_relation(const osmium::Relation& /* relation */) {

	}

};





#endif /* WAYSWITHPOSTALCODEWRITER_HPP_ */