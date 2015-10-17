#include <sys/stat.h>
#include <boost/filesystem.hpp>

#ifndef WRITER_HPP_
#define WRITER_HPP_

#define NO_WIDTH -1

#define USE_TRANSACTIONS true
#define DONT_USE_TRANSACTIONS false

namespace bfs = boost::filesystem;

class Writer {

public:
	Writer(
			const std::string& dirname,
			const std::string& layer_name,
			const bool& use_transaction,
			const OGRwkbGeometryType& geom_type)

	:m_layer_name(layer_name),
	 m_use_transaction(use_transaction),
	 m_layer(nullptr) {

		m_data_source = get_data_source(dirname);

		if (!m_data_source) {
			std::cerr << "Creation of data source for layer '" << m_layer_name
					<< "' failed." << std::endl;
			exit(1);
		}

		OGRSpatialReference spatialref;
		spatialref.SetWellKnownGeogCS("WGS84");

		this->create_layer(m_data_source, geom_type);
	}

	virtual ~Writer() {
		if (m_use_transaction) {
			m_layer->CommitTransaction();
		}
		OGRDataSource::DestroyDataSource(m_data_source);
	}

	virtual void feed_node(const osmium::Node&) = 0;

	virtual void feed_way(const osmium::Way&) = 0;

	virtual void feed_relation(const osmium::Relation&) = 0;

protected:
	osmium::geom::OGRFactory<> m_factory {};
	const std::string m_layer_name;
	const bool m_use_transaction;
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
			std::cout << "m_layer_name=" << m_layer_name << std::endl;
			if (m_layer->CreateField(&field_defn) != OGRERR_NONE) {
				std::cerr << "Creating field '" << it->name <<"' for layer '"
						<< m_layer_name << "' failed." << std::endl;
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
	OGRDataSource* m_data_source;

	unsigned int num_features = 0;

	static bool is_output_dir_written;

	void create_layer(OGRDataSource* data_source, const OGRwkbGeometryType& geom_type) {
		OGRSpatialReference sparef;
		sparef.SetWellKnownGeogCS("WGS84");

		const char* layer_options[] = { "SPATIAL_INDEX=no", "COMPRESS_GEOM=yes", nullptr };

		this->m_layer = data_source->CreateLayer(m_layer_name.c_str(), &sparef,
				geom_type, const_cast<char**>(layer_options));
		if (!m_layer) {
			std::cerr << "Creation of layer '"<< m_layer_name << "' failed.\n";
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

	OGRDataSource* get_data_source(const std::string& dir_name) {
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

		bfs::path full_dir;
		if (is_absolute_path(dir_name)) {
			full_dir = bfs::path(dir_name);
		} else {
			full_dir = bfs::current_path() / bfs::path(dir_name);
		}

		maybe_create_dir(full_dir);
		bfs::path layer_path = full_dir / bfs::path(m_layer_name + ".sqlite");
		return driver->CreateDataSource(layer_path.c_str(), const_cast<char**>(options));
	}

	void maybe_create_dir(const bfs::path& dir) { // TODO: not thread-safe
		if (!is_output_dir_written) {
			is_output_dir_written = true;
			bfs::create_directories(dir);
		}
	}

	bool is_absolute_path(const std::string& path) {
		if (path.substr(0, 1) == "/") {
			return true;
		} else {
			return false;
		}
	}
};

bool Writer::is_output_dir_written = false;

#endif /* WRITER_HPP_ */
