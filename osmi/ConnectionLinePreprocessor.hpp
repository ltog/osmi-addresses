#ifndef CONNECTIONLINEPREPROCESSOR_HPP_
#define CONNECTIONLINEPREPROCESSOR_HPP_

#include <math.h> 

constexpr osmium::object_id_type DUMMY_ID = 0;
constexpr double MAXDIST = 0.06;
constexpr bool IS_ADDRSTREET = true;
constexpr bool IS_ADDRPLACE  = false;

#include "NearestPointsWriter.hpp"
#include "NearestRoadsWriter.hpp"
#include "NearestAreasWriter.hpp"
#include "ConnectionLineWriter.hpp"
#include "GeometryHelper.hpp"

class ConnectionLinePreprocessor {

public:
	ConnectionLinePreprocessor(
			const std::string& dir_name,
			name2highways_type& name2highways_area,
			name2highways_type& name2highways_nonarea,
			name2place_type& name2place_nody,
			name2place_type& name2place_wayy)
  :	mp_name2highways_area(name2highways_area),
	mp_name2highways_nonarea(name2highways_nonarea),
	m_name2place_nody(name2place_nody),
	m_name2place_wayy(name2place_wayy),
	addrstreet(nullptr) {
		mp_nearest_points_writer  = new NearestPointsWriter (dir_name);
		mp_nearest_roads_writer   = new NearestRoadsWriter  (dir_name);
		mp_nearest_areas_writer   = new NearestAreasWriter  (dir_name);
		mp_connection_line_writer = new ConnectionLineWriter(dir_name);
	}

	~ConnectionLinePreprocessor() {
		// those need to be explicitly to be deleted to perform the commit operations in their deconstructor
		delete mp_nearest_points_writer;
		delete mp_nearest_roads_writer;
		delete mp_nearest_areas_writer;
		delete mp_connection_line_writer;
	}

	void process_interpolated_node(
			OGRPoint& ogr_point,
			std::string& road_id,       // out
			std::string& nody_place_id, // out
			std::string& wayy_place_id, // out
			const std::string& street)
	{
		addrstreet = street.c_str();
		if (addrstreet && has_entry_in_name2highways(street)) {
			handle_connection_line(ogr_point, DUMMY_ID,
					object_type::interpolated_node_object, addrstreet, road_id,
					nody_place_id, wayy_place_id, IS_ADDRSTREET);
		}
	}

	void process_node(
			const osmium::Node& node,
			std::string& road_id,       // out
			std::string& nody_place_id, // out
			std::string& wayy_place_id) // out
	{
		addrstreet = node.tags().get_value_by_key("addr:street");
		if (addrstreet && has_entry_in_name2highways(node)) {
			std::unique_ptr<OGRPoint> ogr_point = m_factory.create_point(node);
			handle_connection_line(*ogr_point.get(), node.id(),
					object_type::node_object, addrstreet, road_id,
					nody_place_id, wayy_place_id, IS_ADDRSTREET);
		} else {
			// road_id shall not be written by handle_connection_line
		}

		addrplace = node.tags().get_value_by_key("addr:place");
		if (addrplace && has_entry_in_name2place(node)) {
			std::unique_ptr<OGRPoint> ogr_point = m_factory.create_point(node);
			handle_connection_line(*ogr_point.get(), node.id(),
					object_type::node_object, addrplace, road_id,
					nody_place_id, wayy_place_id, IS_ADDRPLACE);
		} else {
			// nody_place_id, wayy_place_id shall not be written by handle_connection_line
		}
	}

	void process_way(
			const osmium::Way& way,
			std::string& road_id,       // out
			std::string& nody_place_id, // out
			std::string& wayy_place_id) // out
	{
		if (way.is_closed()) {
			addrstreet = way.tags().get_value_by_key("addr:street");
			if (addrstreet && has_entry_in_name2highways(way)) {
				std::unique_ptr<OGRPoint> ogr_point = m_geometry_helper.centroid(way);
				handle_connection_line(*ogr_point.get(), way.id(),
						object_type::way_object, addrstreet, road_id,
						nody_place_id, wayy_place_id, IS_ADDRSTREET);
			} else {
				// road_id shall not be written by handle_connection_line
			}

			addrplace = way.tags().get_value_by_key("addr:place");
			if (addrplace && has_entry_in_name2place(way)) {
				std::unique_ptr<OGRPoint> ogr_point = m_geometry_helper.centroid(way);
				handle_connection_line(*ogr_point.get(), way.id(),
						object_type::way_object, addrplace, road_id,
						nody_place_id, wayy_place_id, IS_ADDRPLACE);
			} else {
				// nody_place_id, wayy_place_id shall not be written by handle_connection_line
			}
		}
	}

private:

	void handle_connection_line(
			OGRPoint&                     ogr_point, // TODO: can we make this const ?
			const osmium::object_id_type& objectid,
			const object_type&            the_object_type,
			const char*                   addrstreet,
			std::string&                  road_id,         // out
			std::string&                  nody_place_id,   // out
			std::string&                  wayy_place_id,   // out
			const bool&                   is_addrstreet) {

		std::unique_ptr<OGRPoint>       closest_node(new OGRPoint);
		std::unique_ptr<OGRPoint>       closest_point(new OGRPoint);    // TODO: check if new is necessary
		std::unique_ptr<OGRLineString>  closest_way(new OGRLineString); // TODO: check if new is necessary
		osmium::unsigned_object_id_type closest_obj_id = 0; // gets written later; wouldn't need an initialization, but gcc warns otherwise
		osmium::unsigned_object_id_type closest_way_id = 0; // gets written later; wouldn't need an initialization, but gcc warns otherwise
		int                             ind_closest_node;
		std::string                     lastchange;
		bool is_area;
		bool is_nody;

		// handle addr:place here
		if (is_addrstreet == IS_ADDRPLACE &&
				get_closest_place(ogr_point, closest_point, is_nody, closest_obj_id, lastchange)) {

			if (is_nody) {
				nody_place_id = "1";
			} else {
				wayy_place_id = "1";
			}

			mp_connection_line_writer->write_line(ogr_point, closest_point, closest_obj_id, the_object_type);
		}

		// handle addr:street here
		if (is_addrstreet == IS_ADDRSTREET &&
				get_closest_way(ogr_point, closest_way, is_area, closest_way_id, lastchange)) {

			m_geometry_helper.wgs2mercator({&ogr_point, closest_way.get(), closest_point.get()});
			get_closest_node(ogr_point, closest_way, closest_node, ind_closest_node);
			get_closest_point_from_node_neighbourhood(ogr_point, closest_way, ind_closest_node, closest_point);
			m_geometry_helper.mercator2wgs({&ogr_point, closest_way.get(), closest_point.get()});

			// TODO: could this be parallelized?
			mp_nearest_points_writer->write_point(closest_point, closest_way_id);
			if (is_area) {
				mp_nearest_areas_writer->write_area(closest_way, closest_way_id, addrstreet, lastchange);
			} else {
				mp_nearest_roads_writer->write_road(closest_way, closest_way_id, addrstreet, lastchange);
			}

			mp_connection_line_writer->write_line(ogr_point, closest_point, objectid, the_object_type);

			road_id = "1"; // TODO: need to write the actual road_id
		}
	}

	// return value: was a closest place found/written
	bool get_closest_place(
			const OGRPoint&                  ogr_point,
			std::unique_ptr<OGRPoint>&       closest_point,  // out
			bool&                            is_nody,        // out
			osmium::unsigned_object_id_type& closest_obj_id,
			std::string&                     lastchange) {

		double best_dist = std::numeric_limits<double>::max();
		double cur_dist = std::numeric_limits<double>::max();
		bool is_assigned = false;

		std::pair<name2place_type::iterator, name2place_type::iterator> name2place_it_pair_nody;
		std::pair<name2place_type::iterator, name2place_type::iterator> name2place_it_pair_wayy;
		name2place_it_pair_nody = m_name2place_nody.equal_range(std::string(addrplace));
		name2place_it_pair_wayy = m_name2place_wayy.equal_range(std::string(addrplace));

		for (name2place_type::iterator it = name2place_it_pair_nody.first; it!=name2place_it_pair_nody.second; ++it) {
			cur_dist = it->second.ogrpoint->Distance(&ogr_point);

			if (cur_dist < best_dist) { // TODO: add check for minimum distance to prevent long connection lines
				closest_point.reset(static_cast<OGRPoint*>(it->second.ogrpoint.get()->clone())); // TODO: check for memory leak
				is_nody     = true;
				is_assigned = true;
				// TODO: extract more info from struct
			}
		}

		for (name2place_type::iterator it = name2place_it_pair_wayy.first; it!=name2place_it_pair_wayy.second; ++it) {
			cur_dist = it->second.ogrpoint->Distance(&ogr_point);

			if (cur_dist < best_dist) { // TODO: add check for minimum distance to prevent long connection lines
				closest_point.reset(static_cast<OGRPoint*>(it->second.ogrpoint.get()->clone())); // TODO: check for memory leak
				is_nody     = false;
				is_assigned = true;
				// TODO: extract more info from struct
			}
		}

		return is_assigned;
	}


	/* look up the closest way with the given name in the name2highway structs for ways and areas */
	/* return: was a way found/assigned to closest_way */
	bool get_closest_way(
			const OGRPoint&                  ogr_point,      // in
			std::unique_ptr<OGRLineString>&  closest_way,    // out
			bool&                            is_area,        // out
			osmium::unsigned_object_id_type& closest_way_id, // out
			std::string&                     lastchange)     // out
	{
		double best_dist = std::numeric_limits<double>::max();
		bool is_assigned = false;

		std::pair<name2highways_type::iterator, name2highways_type::iterator> name2highw_it_pair;

		name2highw_it_pair = mp_name2highways_area.equal_range(std::string(addrstreet));
		if (get_closest_way_from_argument(ogr_point, best_dist, closest_way, closest_way_id, lastchange, name2highw_it_pair)) {
			is_area     = true;
			is_assigned = true;
		}

		name2highw_it_pair = mp_name2highways_nonarea.equal_range(std::string(addrstreet));
		if (get_closest_way_from_argument(ogr_point, best_dist, closest_way, closest_way_id, lastchange, name2highw_it_pair)) {
			is_area     = false;
			is_assigned = true;
		}

		return is_assigned;
	}

	/* look up the closest way in the given name2highway struct that is closer than best_dist using bbox
	 * return true if found */
	bool get_closest_way_from_argument(
			const OGRPoint&                  ogr_point,      // in
			double&                          best_dist,      // in,out
			std::unique_ptr<OGRLineString>&  closest_way,    // out
			osmium::unsigned_object_id_type& closest_way_id, // out
			std::string&                     lastchange,     // out
			const std::pair<name2highways_type::iterator, name2highways_type::iterator> name2highw_it_pair) { // in

		double cur_dist;
		bool assigned = false;

		for (name2highways_type::iterator it = name2highw_it_pair.first; it!=name2highw_it_pair.second; ++it) {
			if (m_geometry_helper.is_point_near_bbox(
					it->second.bbox_n,
					it->second.bbox_e,
					it->second.bbox_s,
					it->second.bbox_w,
					ogr_point,
					MAXDIST)) {

				std::unique_ptr<OGRLineString> linestring = it->second.compr_way.get()->uncompress();

				cur_dist = linestring->Distance(&ogr_point);
				// note: distance calculation involves nodes, but not the points between the nodes on the line segments

				if (cur_dist < best_dist) {
					closest_way.reset(linestring.release());
					closest_way_id = it->second.way_id;
					lastchange     = it->second.lastchange;
					best_dist = cur_dist;
					assigned = true;
				}
			}
		}

		return assigned;
	}

	/* get the node of closest_way that is most close ogr_point */
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

	/* given the linestring closest_way, return the point on it that is closest to ogr_point */
	void get_closest_point_from_node_neighbourhood(
			const OGRPoint&                       ogr_point,
			const std::unique_ptr<OGRLineString>& closest_way,
			const int&                            ind_closest_node,
			std::unique_ptr<OGRPoint>&            closest_point)    // out
	{

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
	/* given a single line segment from a to b, return the point on it that is closest to p */
	void get_closest_point_from_segment(
			const OGRPoint& a,
			const OGRPoint& b,
			const OGRPoint& p,
			OGRPoint& ret) {

		double r;

		r = ((p.getX()-a.getX()) * (b.getX()-a.getX()) + (p.getY()-a.getY()) * (b.getY()-a.getY())) / (pow(b.getX()-a.getX(),2)+pow(b.getY()-a.getY(),2));

		if (r<0) {
			ret = a;
		} else if (r>1) {
			ret = b;
		} else {
			OGRLineString linestring;
			linestring.addPoint(a.getX(), a.getY());
			linestring.addPoint(b.getX(), b.getY());
			linestring.Value(r*linestring.get_Length(), &ret);
		}

	}


	bool has_entry_in_name2highways(const osmium::OSMObject& object) {
		return has_entry_in_name2highways(std::string(object.tags().get_value_by_key("addr:street")));
	}


	bool has_entry_in_name2highways(const std::string& addrstreet) {
		if (mp_name2highways_nonarea.find(std::string(addrstreet)) != mp_name2highways_nonarea.end() || // TODO: use result directly
				(mp_name2highways_area.find(std::string(addrstreet)) != mp_name2highways_area.end())) {
			return true;
		} else {
			return false;
		}
	}

	bool has_entry_in_name2place(const osmium::OSMObject& object) {
		return has_entry_in_name2place(std::string(object.tags().get_value_by_key("addr:place")));
	}

	bool has_entry_in_name2place(const std::string& addrplace) {
		if (m_name2place_nody.find(std::string(addrplace)) != m_name2place_nody.end() || // TODO: use result directly
				(m_name2place_wayy.find(std::string(addrplace)) != m_name2place_wayy.end())) {
			return true;
		} else {
			return false;
		}
	}

	name2highways_type& mp_name2highways_area;
	name2highways_type& mp_name2highways_nonarea;
	name2place_type& m_name2place_nody;
	name2place_type& m_name2place_wayy;
	const char* addrstreet;
	const char* addrplace;
	osmium::geom::OGRFactory<> m_factory {};
	NearestPointsWriter*  mp_nearest_points_writer;
	NearestRoadsWriter*   mp_nearest_roads_writer;
	NearestAreasWriter*   mp_nearest_areas_writer;
	ConnectionLineWriter* mp_connection_line_writer;
	GeometryHelper m_geometry_helper;



};



#endif /* CONNECTIONLINEPREPROCESSOR_HPP_ */
