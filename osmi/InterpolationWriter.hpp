#ifndef INTERPOLATIONWRITER_HPP_
#define INTERPOLATIONWRITER_HPP_

#include "NodesWithAddressesWriter.hpp"
#include "ConnectionLinePreprocessor.hpp"

/**
 * This class is responsible for handling address interpolations
 * (see http://wiki.openstreetmap.org/wiki/Addresses#Using_interpolation)
 */
class InterpolationWriter : public Writer {

	node_map_type* m_addr_interpolation_node_map;

public:

	InterpolationWriter(
			const std::string& dir_name,
			node_map_type* node_map_type_p,
			NodesWithAddressesWriter& nwa_writer,
			ConnectionLinePreprocessor& clpp) :
		Writer(dir_name, "osmi_addresses_interpolation", USE_TRANSACTIONS, wkbLineString),
		m_addr_interpolation_node_map(node_map_type_p),
		m_nwa_writer(nwa_writer),
		m_clpp(clpp){

		std::vector<field_config> field_configurations;
		field_configurations.push_back({"way_id",     OFTString, NO_WIDTH});
		field_configurations.push_back({"typename",   OFTString, NO_WIDTH});
		field_configurations.push_back({"firstid",    OFTString, NO_WIDTH});
		field_configurations.push_back({"lastid",     OFTString, NO_WIDTH});
		field_configurations.push_back({"firstno",    OFTString, NO_WIDTH});
		field_configurations.push_back({"lastno",     OFTString, NO_WIDTH});
		field_configurations.push_back({"error",      OFTString, NO_WIDTH});
		field_configurations.push_back({"lastchange", OFTString, NO_WIDTH});
		create_fields(field_configurations);
	}

	void feed_way(const osmium::Way& way) override {
		try {
			const char* interpolation = way.tags().get_value_by_key("addr:interpolation");

			if (interpolation) {

				std::unique_ptr<OGRLineString> ogr_linestring = m_factory.create_linestring(way);

				OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());

				const osmium::object_id_type first_node_id = m_geometry_helper.get_first_node_id(way);
				const osmium::object_id_type last_node_id  = m_geometry_helper.get_last_node_id(way);

				feature->SetGeometry(ogr_linestring.get());
				feature->SetField("way_id", static_cast<double>(way.id())); //TODO: node.id() is of type int64_t. is this ok?
				feature->SetField("typename", interpolation);
				feature->SetField("firstid",  static_cast<double>(first_node_id)); //TODO: node.id() is of type int64_t. is cast do double ok?
				feature->SetField("lastid",   static_cast<double>(last_node_id)); //TODO: node.id() is of type int64_t. is cast do double ok?
				feature->SetField("lastchange", way.timestamp().to_iso().c_str());

				AltTagList first_taglist =
						m_addr_interpolation_node_map->get(first_node_id);
				AltTagList last_taglist  =
						m_addr_interpolation_node_map->get(last_node_id);

				// the raw expression given in the first addr:housenumber= entry
				std::string first_raw =
						first_taglist.get_value_by_key("addr:housenumber");

				// the raw expression given in the last addr:housenumber= entry
				std::string last_raw  =
						last_taglist.get_value_by_key("addr:housenumber");

				unsigned int first;
				unsigned int last;
				std::string first_numeric; // the numeric part of the first housenumber
				std::string last_numeric;  // the numeric part of the last housenumber

				bool is_alphabetic_ip_correct = false;

				if (first_raw != "") {
					feature->SetField("firstno", first_raw.c_str());
					first = atoi(first_raw.c_str());
				} else {
					first = 0;
				}

				if (last_raw != "") {
					feature->SetField("lastno",  last_raw.c_str());
					last  = atoi(last_raw.c_str());
				} else {
					last = 0;
				}

				if (!strcmp(interpolation, "alphabetic") &&
						// second last characters are numbers?
						!isalpha(first_raw[first_raw.length()-2]) &&
						!isalpha(last_raw[last_raw.length()-2])){

					// last characters are letters?
					if (isalpha(first_raw[first_raw.length()-1]) &&
							isalpha(last_raw[last_raw.length()-1])) {

						first_numeric = std::string(first_raw.begin(), first_raw.end() - 1);
						last_numeric  = std::string(last_raw.begin(),  last_raw.end()  - 1);

						if (first_numeric == last_numeric) {
							first = first_raw[first_raw.length() - 1];
							last = last_raw[last_raw.length() - 1];
							is_alphabetic_ip_correct = true;
						} else {
							is_alphabetic_ip_correct = false;
							feature->SetField("error", "numeric parts of housenumbers not identical");
						}

					} else {
						is_alphabetic_ip_correct = false;
						feature->SetField("error", "no alphabetic part in addr:housenumber");
					}
				}

				if (!(
						!strcmp(interpolation, "all")  ||
						!strcmp(interpolation, "even") ||
						!strcmp(interpolation, "odd")  ||
						!strcmp(interpolation, "alphabetic"))) {

					feature->SetField("error", "unknown interpolation type");

				} else if (
						strcmp(interpolation, "alphabetic") && // don't overwrite more precise error messages set before
						(first == 0 ||
						 last  == 0 ||
						 first_raw.length() != floor(log10(first))+1 || // make sure 123%& is not recognized as 123
						  last_raw.length() != floor(log10(last) )+1    //
						)) {

					feature->SetField("error", "endpoint has wrong format");

				} else if (abs_diff(first,last) > 1000) {
					feature->SetField("error", "range too large");

				} else if (((!strcmp(interpolation,"even") || !strcmp(interpolation,"odd")) && abs_diff(first,last)==2) ||
							(!strcmp(interpolation,"all")                                   && abs_diff(first,last)==1) ) {
					feature->SetField("error", "needless interpolation");

				} else if (!strcmp(interpolation,"even") && ( first%2==1 || last%2==1 )) {
					feature->SetField("error", "interpolation even but number odd");

				} else if (!strcmp(interpolation,"odd") && ( first%2==0 || last%2==0 )) {
					feature->SetField("error", "interpolation odd but number even");

				} else if (
					(first_taglist.get_value_by_key("addr:street")   != last_taglist.get_value_by_key("addr:street"))   ||
					(first_taglist.get_value_by_key("addr:postcode") != last_taglist.get_value_by_key("addr:postcode")) ||
					(first_taglist.get_value_by_key("addr:city")     != last_taglist.get_value_by_key("addr:city"))     ||
					(first_taglist.get_value_by_key("addr:country")  != last_taglist.get_value_by_key("addr:country"))  ||
					(first_taglist.get_value_by_key("addr:full")     != last_taglist.get_value_by_key("addr:full"))     ||
					(first_taglist.get_value_by_key("addr:place")    != last_taglist.get_value_by_key("addr:place")) ) {
					feature->SetField("error", "different tags on endpoints");
				} else if (way.is_closed()) {
				    feature->SetField("error", "interpolation is a closed way");
				} else if ( // no interpolation error
						(!strcmp(interpolation, "all")) ||
						(!strcmp(interpolation, "odd")) ||
						(!strcmp(interpolation, "even")) ||
						(is_alphabetic_ip_correct == true)) {
					double length = ogr_linestring.get()->get_Length();
					int increment;

					if (strcmp(interpolation, "all") && strcmp(interpolation, "alphabetic")) {
						increment = 2; // even , odd
					} else {
						increment = 1; //all , alphabetic
					}

					double fraction;
					unsigned int lower, upper;

					
					if (first < last) {
						fraction = 1/static_cast<double>(last-first);
						lower = first;
						upper = last;
					} else {
						fraction = 1/static_cast<double>(first-last);
						increment *= -1;
						lower = last;
						upper = first;
					}
					
					for (unsigned int nr=first+increment; nr<upper && nr>lower; nr+=increment) {
						std::unique_ptr<OGRPoint> point (new OGRPoint);
						if (increment > 0) {
							ogr_linestring.get()->Value((nr-lower)*fraction*length, point.get());
						} else {
							ogr_linestring.get()->Value((1-((nr-lower)*fraction))*length, point.get());
						}

						std::string road_id("");
						std::string nrstr;
						
						if(strcmp(interpolation, "alphabetic")) {
							nrstr = std::to_string(nr);
						} else { // is alphabetic
							// std::string strend = printf("%d", nr);
							nrstr = first_numeric + static_cast<char>(nr);
						}

						m_clpp.process_interpolated_node( // osmi_addresses_connection_line 
								*(point.get()),
								road_id,
								first_taglist.get_value_by_key("addr:street")
						);
						m_nwa_writer.process_interpolated_node( //osmi_addresses_nodes_with_addresses
							*(point.get()),
								nrstr,
								//nr,
								//std::to_string(nr),
								first_taglist.get_value_by_key("addr:street"),
								first_taglist.get_value_by_key("addr:postcode"),
								first_taglist.get_value_by_key("addr:city"),
								first_taglist.get_value_by_key("addr:country"),
								first_taglist.get_value_by_key("addr:full"),
								first_taglist.get_value_by_key("addr:place"),
								road_id
						);
					}
				}

				create_feature(feature);

			}


		} catch (osmium::geometry_error& e) {
			catch_geometry_error(e, way);
		}
	}

private:
	GeometryHelper m_geometry_helper;
	NodesWithAddressesWriter& m_nwa_writer; //osmi_addresses_nodes_with_addresses
	ConnectionLinePreprocessor& m_clpp; //osmi_addresses_connection_line 

	inline unsigned int abs_diff(const unsigned int& arg1, const unsigned int& arg2) {
		return (arg1 > arg2 ? arg1-arg2 : arg2-arg1);
	}
};

#endif /* INTERPOLATIONWRITER_HPP_ */
