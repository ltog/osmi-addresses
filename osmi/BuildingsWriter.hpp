#ifndef BUILDINGSWRITER_HPP_
#define BUILDINGSWRITER_HPP_

class BuildingsWriter : public Writer {

public:

	BuildingsWriter(const std::string& dir_name) :
		Writer(dir_name, "osmi_addresses_buildings", USE_TRANSACTIONS, wkbPolygon) {

		std::vector<field_config> field_configurations;
		field_configurations.push_back({"way_id",     OFTString, NO_WIDTH});
		field_configurations.push_back({"lastchange", OFTString, NO_WIDTH});
		create_fields(field_configurations);
	}

	void feed_way(const osmium::Way& way) override {
		try {
			const char* building = way.tags().get_value_by_key("building");

			if (building && way.is_closed()) {
				std::unique_ptr<OGRPolygon> polygon = m_factory.create_polygon(way);
				OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());
				feature->SetGeometryDirectly(static_cast<OGRGeometry*>(polygon.release()));

				feature->SetField("way_id", static_cast<double>(way.id())); //TODO: node.id() is of type int64_t. is this ok?
				feature->SetField("lastchange", way.timestamp().to_iso().c_str());

				create_feature(feature);
			}

		} catch (osmium::geometry_error& e) {
			catch_geometry_error(e, way);
		}
	}

};

#endif /* BUILDINGSWRITER_HPP_ */
