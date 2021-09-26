#ifndef CONNECTIONLINEWRITER_HPP_
#define CONNECTIONLINEWRITER_HPP_

class ConnectionLineWriter : public Writer {

public:

	ConnectionLineWriter(const std::string& dir_name) :
		Writer(dir_name, "osmi_addresses_connection_line", USE_TRANSACTIONS, wkbLineString) {

		std::vector<field_config> field_configurations;
		field_configurations.push_back({"node_id", OFTString, NO_WIDTH});
		field_configurations.push_back({"way_id",  OFTString, NO_WIDTH});

		create_fields(field_configurations);
	}

	void write_line(
			const OGRPoint&                  ogr_point,
			const std::unique_ptr<OGRPoint>& closest_point,
			const osmium::object_id_type&    objectid, // ID of the object (e.g. building) carrying addr:street, addr:place
			const object_type&               the_object_type) { // type of the object carrying addr:street, addr:place

		OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());
		OGRLineString connection_line;
		connection_line.addPoint(ogr_point.getX(), ogr_point.getY());
		connection_line.addPoint(closest_point->getX(), closest_point->getY());
		feature->SetGeometry(&connection_line);
		if (the_object_type == object_type::node_object) {
			feature->SetField("node_id", static_cast<double>(objectid)); //TODO: object.id() is of type int64_t. is this ok?
		} else if (the_object_type == object_type::way_object) {
			feature->SetField("way_id" , static_cast<double>(objectid)); //TODO: object.id() is of type int64_t. is this ok?
		}
		else if (the_object_type == object_type::interpolated_node_object){
			//there is no id to write
		}

		create_feature(feature);
	}

};

#endif /* CONNECTIONLINEWRITER_HPP_ */
