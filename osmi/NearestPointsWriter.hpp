#ifndef NEARESTPOINTSWRITER_HPP_
#define NEARESTPOINTSWRITER_HPP_

class NearestPointsWriter : public Writer {

public:

	NearestPointsWriter(const std::string& dir_name) :
		Writer(dir_name, "osmi_addresses_nearest_points", USE_TRANSACTIONS, wkbPoint)  {

		std::vector<field_config> field_configurations;
		field_configurations.push_back({"way_id", OFTString, NO_WIDTH});

		create_fields(field_configurations);
	}

	void write_point(
			const std::unique_ptr<OGRPoint>&       closest_point,
			const osmium::unsigned_object_id_type& closest_way_id) {

		OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());
		feature->SetGeometry(closest_point.get());
		feature->SetField("way_id", static_cast<double>(closest_way_id)); //TODO: closest_way_id is of type int64_t. is this ok?

		create_feature(feature);
	}

	void feed_node(const osmium::Node& /* node */) {

	}

	void feed_way(const osmium::Way& /* way */) {

	}

	void feed_relation(const osmium::Relation& /* relation */) {

	}
};

#endif /* NEARESTPOINTSWRITER_HPP_ */
