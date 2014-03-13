#ifndef ENTRANCESWRITER_HPP_
#define ENTRANCESWRITER_HPP_


class EntrancesWriter : public Writer {

public:

	EntrancesWriter(OGRDataSource* data_source) :
		Writer(data_source, "osmi_addresses_entrances", USE_TRANSACTIONS, wkbPoint) {

		std::vector<field_config> field_configurations;
		field_configurations.push_back({"node_id",    OFTString,  NO_WIDTH});
		field_configurations.push_back({"lastchange", OFTString,  NO_WIDTH});
		field_configurations.push_back({"entrance",   OFTString,  NO_WIDTH});

		create_fields(field_configurations);
	}

	void feed_node(const osmium::Node& node) {
		const char* entrance = node.tags().get_value_by_key("entrance");
		const char* building = node.tags().get_value_by_key("building");
		if (entrance || (building && !strcmp(building, "entrance") )) {

			std::unique_ptr<OGRPoint> ogr_point = m_factory.create_point(node);
			OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());

			feature->SetGeometry(static_cast<OGRGeometry*>(ogr_point.get()));
			feature->SetField("node_id", static_cast<double>(node.id())); //TODO: way.id() is of type int64_t. is this ok?
			feature->SetField("lastchange", node.timestamp().to_iso().c_str());
			if (entrance) {
				feature->SetField("entrance", entrance);
			}

			create_feature(feature);
		}
	}

	void feed_way(const osmium::Way& /* way */) {

	}

	void feed_relation(const osmium::Relation& /* relation */) {

	}

};






#endif /* ENTRANCESWRITER_HPP_ */
