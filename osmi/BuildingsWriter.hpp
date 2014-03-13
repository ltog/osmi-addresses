#ifndef BUILDINGSWRITER_HPP_
#define BUILDINGSWRITER_HPP_

class BuildingsWriter : public Writer {

public:

	BuildingsWriter(OGRDataSource* data_source) :
		Writer(data_source, "osmi_addresses_buildings", USE_TRANSACTIONS, wkbPolygon) {

		std::vector<field_config> field_configurations;
		field_configurations.push_back({"way_id",     OFTString, NO_WIDTH});
		field_configurations.push_back({"lastchange", OFTString, NO_WIDTH});
		create_fields(field_configurations);
	}

	void feed_way(const osmium::Way& way) {
		try {
			const char* building = way.tags().get_value_by_key("building");

			if (building && way.is_closed()) {
				std::unique_ptr<OGRLineString> ogr_linestring = m_factory.create_linestring(way);
				OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());
				OGRPolygon polygon;
				polygon.addRing(static_cast<OGRLinearRing*>(ogr_linestring.get()));
				feature->SetGeometry(static_cast<OGRGeometry*>(&polygon));

				feature->SetField("way_id", static_cast<double>(way.id())); //TODO: node.id() is of type int64_t. is this ok?
				feature->SetField("lastchange", way.timestamp().to_iso().c_str());

				create_feature(feature);
			}

		} catch (osmium::geom::geometry_error& e) {
			catch_geometry_error(e, way);
		}
	}

	void feed_node(const osmium::Node& /* node */) {

	}

	void feed_relation(const osmium::Relation& /* relation */) {

	}
};


#endif /* BUILDINGSWRITER_HPP_ */
