#ifndef CONNECTIONLINEPREPROCESSOR_HPP_
#define CONNECTIONLINEPREPROCESSOR_HPP_

#include <math.h> 

#define DUMMY_ID 0
#define MAXDIST 0.06
#define PI 3.14159265358979323846
#define DEG2RAD(DEG) ((DEG)*((PI)/(180.0)))

#include "NearestPointsWriter.hpp"
#include "NearestRoadsWriter.hpp"
#include "NearestAreasWriter.hpp"
#include "ConnectionLineWriter.hpp"
#include "GeometryHelper.hpp"

class ConnectionLinePreprocessor {

public:
	ConnectionLinePreprocessor(OGRDataSource* data_source, name2highways_type& name2highways):
	mp_name2highways(name2highways),
	addrstreet(nullptr) {
		mp_nearest_points_writer  = new NearestPointsWriter (data_source);
		mp_nearest_roads_writer   = new NearestRoadsWriter  (data_source);
		mp_nearest_areas_writer   = new NearestAreasWriter  (data_source);
		mp_connection_line_writer = new ConnectionLineWriter(data_source);
	}

	~ConnectionLinePreprocessor() {
		// those need to be explicitly to be deleted to perform the commit operations in their deconstructor
		delete mp_nearest_points_writer;
		delete mp_nearest_roads_writer;
		delete mp_nearest_areas_writer;
		delete mp_connection_line_writer;
	}

	void process_interpolated_node(OGRPoint& ogr_point, std::string& road_id, const std::string& street) {
		addrstreet = street.c_str();
		if (addrstreet && has_entry_in_name2highways(street)) {
			handle_connection_lines(ogr_point, DUMMY_ID, object_type::interpolated_node_object, addrstreet, road_id);
		}
	}

	void process_node(const osmium::Node& node, std::string& road_id) {
		addrstreet = node.tags().get_value_by_key("addr:street");
		if (addrstreet && has_entry_in_name2highways(node)) {
			std::unique_ptr<OGRPoint> ogr_point = m_factory.create_point(node);
			handle_connection_lines(*ogr_point.get(), node.id(), object_type::node_object, addrstreet, road_id);
		}
	}

	void process_way(const osmium::Way& way, std::string& road_id) {
		if (way.is_closed()) {
			addrstreet = way.tags().get_value_by_key("addr:street");
			if (addrstreet && has_entry_in_name2highways(way)) {
				std::unique_ptr<OGRPoint> ogr_point = m_geometry_helper.centroid(way);
				handle_connection_lines(*ogr_point.get(), way.id(), object_type::way_object, addrstreet, road_id);
			}
		}
	}

private:

	void handle_connection_lines(
			OGRPoint&                     ogr_point,
			const osmium::object_id_type& objectid,
			const object_type&            the_object_type,
			const char*                   addrstreet,
			std::string&                  road_id) {

		std::unique_ptr<OGRPoint>       closest_node(new OGRPoint);
		std::unique_ptr<OGRPoint>       closest_point(new OGRPoint);    // TODO: check if new is necessary
		std::unique_ptr<OGRLineString>  closest_way(new OGRLineString); // TODO: check if new is necessary
		osmium::unsigned_object_id_type closest_way_id = 0; // wouldn't need an initialization, but gcc warns otherwise
		int                             ind_closest_node;
		std::string						lastchange;
		bool area;

		if(get_closest_way(ogr_point, closest_way, area, closest_way_id, lastchange)) {
			m_geometry_helper.wgs2mercator({&ogr_point, closest_way.get(), closest_point.get()});
			get_closest_node(ogr_point, closest_way, closest_node, ind_closest_node);
			get_closest_point_from_node_neighbourhood(ogr_point, closest_way, ind_closest_node, closest_point);
			m_geometry_helper.mercator2wgs({&ogr_point, closest_way.get(), closest_point.get()});

			// TODO: could this be parallelized?
			mp_nearest_points_writer->write_point(closest_point, closest_way_id);
			if (area) {
				mp_nearest_areas_writer->write_area(closest_way, closest_way_id, addrstreet, lastchange);
			} else {
				mp_nearest_roads_writer->write_road(closest_way, closest_way_id, addrstreet, lastchange);
			}

			mp_connection_line_writer->write_line(ogr_point, closest_point, objectid, the_object_type);

			road_id = "1"; // TODO: need to write the actual road_id
		}
	}


	bool get_closest_way(
			const OGRPoint& ogr_point,
			std::unique_ptr<OGRLineString>&  closest_way,
			bool& is_area,
			osmium::unsigned_object_id_type& closest_way_id,
			std::string&                     lastchange) {

		double min_dist = std::numeric_limits<double>::max();
		double dist;
		bool assigned = false;
		float corrected = MAXDIST / cos(DEG2RAD(ogr_point.getY()));

		std::pair<name2highways_type::iterator, name2highways_type::iterator> name2highw_it_pair;
		name2highw_it_pair = mp_name2highways.equal_range(std::string(addrstreet));

		for (name2highways_type::iterator it = name2highw_it_pair.first; it!=name2highw_it_pair.second; ++it) {
			if (fabs(static_cast<float>(it->second.lat - ogr_point.getY())) < MAXDIST &&
				fabs(static_cast<float>(it->second.lon - ogr_point.getX())) < corrected ) {

				OGRLineString linestring = *(static_cast<OGRLineString*>(it->second.compr_way.get()->uncompress().get()->clone()));
				dist = linestring.Distance(&ogr_point);
				if (dist < min_dist) {
					closest_way.reset(static_cast<OGRLineString*>(linestring.clone()));
					closest_way_id = it->second.way_id;
					lastchange     = it->second.lastchange;
               is_area = it->second.area;
					min_dist = dist;
					assigned = true;
				}
			}
		}

		return assigned;
	}

	void get_closest_node(
			const OGRPoint&                       ogr_point,
			const std::unique_ptr<OGRLineString>& closest_way,
			std::unique_ptr<OGRPoint>&            closest_node,
			int&                                  ind_closest_node) {

		double min_dist = std::numeric_limits<double>::max();
		double dist;
		OGRPoint closest_node_candidate;

		// iterate over all points of the closest way
		for (int i=0; i<closest_way->getNumPoints(); i++){
			closest_way->getPoint(i, &closest_node_candidate);

			dist = ogr_point.Distance(&closest_node_candidate);

			if (dist < min_dist) {
				min_dist = dist;
				ind_closest_node = i;
			}
		}

		closest_way->getPoint(ind_closest_node, closest_node.get());
	}

	void get_closest_point_from_node_neighbourhood(
			OGRPoint&                             ogr_point,
			const std::unique_ptr<OGRLineString>& closest_way,
			const int&                            ind_closest_node,
			std::unique_ptr<OGRPoint>&            closest_point) {

		OGRPoint neighbour_node;

		OGRPoint closest_point_candidate;

		OGRPoint closest_node;
		closest_way.get()->getPoint(ind_closest_node, &closest_node);
		closest_point.reset(static_cast<OGRPoint*>(closest_node.clone()));

		if (ind_closest_node > 0) {
			closest_way->getPoint(ind_closest_node-1, &neighbour_node);
			get_closest_point_from_segment(closest_node, neighbour_node, ogr_point, *closest_point.get());
			// no if condition necessary here, because get_closest_point_from_segment()
			// will return a point, that is at least as close as closest_node
		}
		if (ind_closest_node < closest_way->getNumPoints()-1) {
			closest_way->getPoint(ind_closest_node+1, &neighbour_node);
			get_closest_point_from_segment(closest_node, neighbour_node, ogr_point, closest_point_candidate);

			if (ogr_point.Distance(&closest_point_candidate) < ogr_point.Distance(closest_point.get())) {
				closest_point.reset(static_cast<OGRPoint*>(closest_point_candidate.clone()));
			}
		}
	}

	// based on: http://postgis.refractions.net/documentation/postgis-doxygen/da/de7/liblwgeom_8h_84b0e41df157ca1201ccae4da3e3ef7d.html#84b0e41df157ca1201ccae4da3e3ef7d
	// see also: http://femto.cs.illinois.edu/faqs/cga-faq.html#S1.02
	void get_closest_point_from_segment(
			OGRPoint& a,
			OGRPoint& b,
			OGRPoint& p,
			OGRPoint& ret) {

		double r;

		r = ((p.getX()-a.getX()) * (b.getX()-a.getX()) + (p.getY()-a.getY()) * (b.getY()-a.getY())) / (pow(b.getX()-a.getX(), 2)+pow(b.getY()-a.getY(),2));

		if (r<0) {
			ret = a;
		} else if (r>1) {
			ret = b;
		} else {
			OGRLineString linestring;
			linestring.addPoint(&a);
			linestring.addPoint(&b);
			linestring.Value(r*linestring.get_Length(), &ret);
		}

	}


	bool has_entry_in_name2highways(const osmium::OSMObject& object) {
		return has_entry_in_name2highways(std::string(object.tags().get_value_by_key("addr:street")));
	}


	bool has_entry_in_name2highways(const std::string& addrstreet) {
		if (mp_name2highways.find(std::string(addrstreet)) != mp_name2highways.end()) {
			return true;
		} else {
			return false;
		}
	}


	name2highways_type& mp_name2highways;
	const char* addrstreet;
	osmium::geom::OGRFactory<> m_factory {};
	NearestPointsWriter*  mp_nearest_points_writer;
	NearestRoadsWriter*   mp_nearest_roads_writer;
	NearestAreasWriter*   mp_nearest_areas_writer;
	ConnectionLineWriter* mp_connection_line_writer;
	GeometryHelper m_geometry_helper;



};



#endif /* CONNECTIONLINEPREPROCESSOR_HPP_ */
