#ifndef WRITER_HPP_
#define WRITER_HPP_

#define NO_WIDTH -1

#define USE_TRANSACTIONS true
#define DONT_USE_TRANSACTIONS false

class Writer {

public:
	Writer(OGRDataSource* data_source, const std::string layer_name, const bool use_transaction, const OGRwkbGeometryType& geom_type)
	:m_use_transaction(use_transaction),
	 m_layer(nullptr) {

		this->create_layer(data_source, layer_name, geom_type);
	}

	virtual ~Writer() {
		if (m_use_transaction) {
			m_layer->CommitTransaction();
		}
	};

	virtual void feed_node(const osmium::Node&) = 0;

	virtual void feed_way(const osmium::Way&) = 0;

	virtual void feed_relation(const osmium::Relation&) = 0;

protected:
	osmium::geom::OGRFactory<> m_factory {};
	std::string m_layer_name;
	bool m_use_transaction;
	OGRLayer* m_layer;

	struct field_config {
		std::string  name;
		OGRFieldType type;
		int          width;
	};

	void create_fields(const std::vector<field_config>& field_configurations) {
		for (auto it = field_configurations.cbegin(); it!=field_configurations.cend(); ++it) {
			OGRFieldDefn field_defn(it->name.c_str(), it->type);
			if (it->width != NO_WIDTH) {
				field_defn.SetWidth(it->width);
			}
			if (m_layer->CreateField(&field_defn) != OGRERR_NONE) {
				std::cerr << "Creating field '" << it->name <<"' for layer '" << m_layer_name << "' failed." << std::endl;
				exit(1);
			}
		}
		if (m_use_transaction) {
			m_layer->StartTransaction();
		}
	}

	void create_feature(OGRFeature* feature) {
		OGRErr e = m_layer->CreateFeature(feature);
		if (e != OGRERR_NONE) {
			std::cerr << "Failed to create feature. e = " << e << std::endl;
			exit(1);
		}
		OGRFeature::DestroyFeature(feature);
		maybe_commit_transaction();
	}

	void catch_geometry_error(const osmium::geometry_error& e, const osmium::Way& way) {
		std::cerr << "Ignoring illegal geometry for way with id = " << way.id() << " e.what() = " << e.what() << std::endl;
	}

private:
	unsigned int num_features = 0;

	void create_layer(OGRDataSource* data_source, const std::string& layer_name, const OGRwkbGeometryType& geom_type) {
		OGRSpatialReference sparef;
		sparef.SetWellKnownGeogCS("WGS84");

		const char* layer_options[] = { "SPATIAL_INDEX=no", "COMPRESS_GEOM=yes", nullptr };

		this->m_layer = data_source->CreateLayer(layer_name.c_str(), &sparef, geom_type, const_cast<char**>(layer_options));
		if (!m_layer) {
			std::cerr << "Creation of layer '"<< layer_name << "' failed.\n";
			exit(1);
		}
	}

	void maybe_commit_transaction() {
		num_features++;
		if (m_use_transaction && num_features > 10000) {
			m_layer->CommitTransaction();
			m_layer->StartTransaction();
			num_features = 0;
		}
	}
};

#endif /* WRITER_HPP_ */
