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
            std::unique_ptr<OGRPoint> closest_point, // cannot be const because in GDAL 1 OGRFeature::SetGeometry() does not accept const OGRGeometry*.
			const osmium::unsigned_object_id_type closest_way_id) {

		OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());
		feature->SetGeometryDirectly(closest_point.release());
		feature->SetField("way_id", static_cast<double>(closest_way_id)); //TODO: closest_way_id is of type int64_t. is this ok?

		create_feature(feature);
	}
};

#endif /* NEARESTPOINTSWRITER_HPP_ */
