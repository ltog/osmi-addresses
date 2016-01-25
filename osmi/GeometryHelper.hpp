#ifndef GEOMETRYHELPER_HPP_
#define GEOMETRYHELPER_HPP_

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#include <ogr_api.h>
#pragma GCC diagnostic pop

constexpr double PI = 3.141592653589793238462;

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

	// PRE: way.is_closed() == true
	std::unique_ptr<OGRPoint> centroid(const osmium::Way& way) {

		std::unique_ptr<OGRLineString> ogr_linestring = m_factory.create_linestring(way);

		OGRPolygon polygon;
		polygon.addRing(static_cast<OGRLinearRing*>(ogr_linestring.get()));

		std::unique_ptr<OGRPoint> centroid(new OGRPoint);
		int ret = polygon.Centroid(centroid.get());
		if (ret == OGRERR_NONE) {
			return centroid;
		} else {
			std::cerr << "Couldn't calculate centroid of way = " << way.id() << ".\n";
			osmium::geometry_error e(std::string("Couldn't calculate centroid of way = ") + std::to_string(way.id()) + std::string(".\n"));
			throw e;
			return nullptr;
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

	double_bbox get_bbox(const osmium::Way& way) {
		double_bbox bbox;

		bbox.north = way.nodes()[0].location().lat();
		bbox.south = way.nodes()[0].location().lat();
		bbox.east  = way.nodes()[0].location().lon();
		bbox.west  = way.nodes()[0].location().lon();

		for (unsigned int i=1; i<way.nodes().size(); i++) {
			if (way.nodes()[i].location().lat() > bbox.north) {
				bbox.north = way.nodes()[i].location().lat();
			} else if (way.nodes()[i].location().lat() < bbox.south) {
				bbox.south = way.nodes()[i].location().lat();
			}

			if (way.nodes()[i].location().lon() > bbox.east) {
				bbox.east = way.nodes()[i].location().lon();
			} else if (way.nodes()[i].location().lon() < bbox.west) {
				bbox.west = way.nodes()[i].location().lon();
			}
		}

		return bbox;
	}

	int16_t lat2int16(const double lat, const bool increment_when_rounding) {
		double result = lat*INT16_MAX/90;
		if (increment_when_rounding) {
			return ceil(result);
		} else {
			return floor(result);
		}
	}

	int16_t lon2int16(const double lon, const bool increment_when_rounding) {
		double result = lon*INT16_MAX/180;
		if (increment_when_rounding) {
			return ceil(result);
		} else {
			return floor(result);
		}
	}

	float int162lat(const int16_t lat) {
		return static_cast<float>(lat)*90/INT16_MAX;
	}

	float int162lon(const int16_t lon) {
		return static_cast<float>(lon)*180/INT16_MAX;
	}

	/**
	 * Returns true if all of the following conditions are met:
	 * - latitude of the point is less than tolerance degrees outside the bbox
	 * - longitude of the point is less than tolerance/cos(lat(point)) degrees outside the bbox
	 */
	bool is_point_near_bbox(
			const int16_t& bbox_n,
			const int16_t& bbox_e,
			const int16_t& bbox_s,
			const int16_t& bbox_w,
			const OGRPoint& point,
			const float& tolerance) {

		const float lat = point.getY();
		const float lon = point.getX();
		const float n = int162lat(bbox_n);
		const float e = int162lon(bbox_e);
		const float s = int162lat(bbox_s);
		const float w = int162lon(bbox_w);

		if (
				lat < n + tolerance &&
				lat > s - tolerance &&
				lon < e + tolerance/cos(deg2rad(lat)) &&
				lon > w - tolerance/cos(deg2rad(lat))) {

			return true;
		} else {
			return false;
		}
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

	/*
	 * Convert degrees to radians
	 */
	template <typename T>
	const inline T deg2rad(T x) {
		return x*PI/180;
	}

private:
	OGRSpatialReference m_wgs;
	OGRSpatialReference m_mercator;
	OGRCoordinateTransformation* m_wgs2mercator;
	OGRCoordinateTransformation* m_mercator2wgs;

	osmium::geom::OGRFactory<> m_factory;
};

#endif /* GEOMETRYHELPER_HPP_ */
