#ifndef GEOMETRYHELPER_HPP_
#define GEOMETRYHELPER_HPP_

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#include <ogr_api.h>
#pragma GCC diagnostic pop

class GeometryHelper{

public:
	GeometryHelper() {
		m_wgs.SetWellKnownGeogCS("WGS84");
		m_mercator.importFromEPSG(3857);

		m_wgs2mercator = OGRCreateCoordinateTransformation(&m_wgs, &m_mercator);
		if (m_wgs2mercator == NULL) {
			std::cerr << "ERROR: m_wgs2mercator is null" << std::endl;
		}

		m_mercator2wgs = OGRCreateCoordinateTransformation(&m_mercator, &m_wgs);
		if (m_mercator2wgs == NULL) {
			std::cerr << "ERROR: m_mercator2wgs is null" << std::endl;
		}
	}

	std::unique_ptr<OGRPoint> centroid(const osmium::Way& way) {

		std::unique_ptr<OGRLineString> ogr_linestring = m_factory.create_linestring(way);

		OGRPolygon polygon;
		polygon.addRing(static_cast<OGRLinearRing*>(ogr_linestring.get()));

		std::unique_ptr<OGRPoint> centroid(new OGRPoint);
		int ret = polygon.Centroid(centroid.get());
		if (ret != OGRERR_NONE) {
			std::cerr << "Couldn't calculate centroid of way = " << way.id() << ".\n";
			osmium::geometry_error e(std::string("Couldn't calculate centroid of way = ") + std::to_string(way.id()) + std::string(".\n"));
			throw e;
			return nullptr;
		} else {
			return centroid;
		}

		return nullptr;
	}

	void mercator2wgs(std::initializer_list<OGRGeometry*> geometries) {
		for (auto& geometry : geometries) {
			geometry->transform(m_mercator2wgs);
		}
	}

	void mercator2wgs(OGRGeometry* geometry) {
			geometry->transform(m_mercator2wgs);
	}

	void wgs2mercator(std::initializer_list<OGRGeometry*> geometries) {
		for (auto& geometry : geometries) {
			geometry->transform(m_wgs2mercator);
		}
	}

	void wgs2mercator(OGRGeometry* geometry) {
			geometry->transform(m_wgs2mercator);
	}

	osmium::unsigned_object_id_type get_first_node_id(const osmium::Way& way) {
		return way.nodes()[0].ref();
	}

	osmium::unsigned_object_id_type get_last_node_id(const osmium::Way& way) {
		return way.nodes()[way.nodes().size()-1].ref();
	}

	float get_lat_estimate(const osmium::Way& way) {
		// return middle point (lat) between first and last node's lat
		return (way.nodes()[0].location().lat() + way.nodes()[way.nodes().size()-1].location().lat())/2;
	}

	float get_lon_estimate(const osmium::Way& way) {
		// return middle point (lon) between first and last node's lon
		return (way.nodes()[0].location().lon() + way.nodes()[way.nodes().size()-1].location().lon())/2;
	}

	bool is_way_with_nonzero_length(const osmium::Way& way) {
		if (way.nodes().size() < 2) {
			return false;
		}
		for (unsigned int i=1; i<way.nodes().size(); i++) {
			if (way.nodes()[0].location().lat() != way.nodes()[i].location().lat()) {
				return true;
			}
			if (way.nodes()[0].location().lon() != way.nodes()[i].location().lon()) {
				return true;
			}
		}
		return false;
	}



private:
	OGRSpatialReference m_wgs;
	OGRSpatialReference m_mercator;
	OGRCoordinateTransformation* m_wgs2mercator;
	OGRCoordinateTransformation* m_mercator2wgs;

	osmium::geom::OGRFactory<> m_factory;
};

#endif /* GEOMETRYHELPER_HPP_ */
