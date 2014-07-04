#ifndef INTERPOLATIONWRITER_HPP_
#define INTERPOLATIONWRITER_HPP_

#include "NodesWithAddressesWriter.hpp"
#include "ConnectionLinePreprocessor.hpp"

class InterpolationWriter : public Writer {

	node_map_type* m_addr_interpolation_node_map;

public:

	InterpolationWriter(
			OGRDataSource* data_source,
			node_map_type* node_map_type_p,
			NodesWithAddressesWriter& nwa_writer,
			ConnectionLinePreprocessor& clpp) :
		Writer(data_source, "osmi_addresses_interpolation", USE_TRANSACTIONS, wkbLineString),
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

	void feed_node(const osmium::Node& /* node */) {

	}

	void feed_way(const osmium::Way& way) {
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

				AltTagList first_taglist = m_addr_interpolation_node_map->get(first_node_id);
				AltTagList last_taglist  = m_addr_interpolation_node_map->get(last_node_id);

				std::string first_node_housenumber = first_taglist.get_value_by_key(std::string("addr:housenumber"));
				std::string last_node_housenumber  = last_taglist.get_value_by_key(std::string("addr:housenumber"));

				unsigned int first;
				unsigned int last;
				std::string first_number;
				std::string last_number;
				bool correct_alphanumeric = false;

				if (first_node_housenumber != "") {
					feature->SetField("firstno", first_node_housenumber.c_str());
					first = atoi(first_node_housenumber.c_str());
				} else {
					first = 0;
				}

				if (last_node_housenumber != "") {
					feature->SetField("lastno",  last_node_housenumber.c_str());
					last  = atoi(last_node_housenumber.c_str());
				} else {
					last = 0;
				}

				if (!strcmp(interpolation, "alphabetic"))
				{
					if(!isalpha(first_node_housenumber[first_node_housenumber.length()-2]) && !isalpha(last_node_housenumber[last_node_housenumber.length()-2])){
						if (isalpha(first_node_housenumber[first_node_housenumber.length()-1]) && isalpha(last_node_housenumber[last_node_housenumber.length()-1])) {
							std::string firstnumber (first_node_housenumber.begin(), first_node_housenumber.end() - 1);
							std::string lastnumber (last_node_housenumber.begin(), last_node_housenumber.end() - 1);
							first_number = firstnumber;
							last_number = lastnumber;
							if (firstnumber == lastnumber) {
								first = first_node_housenumber[first_node_housenumber.length() - 1];
								last = last_node_housenumber[last_node_housenumber.length() - 1];
								correct_alphanumeric = true;
							} else {
								correct_alphanumeric = false;
								feature->SetField("error", "is alphanumeric but housenumber is not the same");
							}
						} else {
							correct_alphanumeric = false;
							feature->SetField("error", "is not alphanumeric");
						}
					}
				}


				if (!(!strcmp(interpolation,"all") || !strcmp(interpolation,"even") || !strcmp(interpolation,"odd") || !strcmp(interpolation,"alphabetic"))) { // TODO: add support for 'alphabetic'
					feature->SetField("error", "unknown interpolation type");
				} else if (
						(first == 0 ||
						last  == 0 ||
						first_node_housenumber.length() != floor(log10(first))+1 || // make sure 123%& is not recognized as 123
						 last_node_housenumber.length() != floor(log10(last) )+1    //
					) && correct_alphanumeric != true) {
					feature->SetField("error", "endpoint has wrong format");
				} else 	if (abs(first-last) > 1000) {
					feature->SetField("error", "range too large");
				} else if (((!strcmp(interpolation,"even") || !strcmp(interpolation,"odd")) && abs(first-last)==2) ||
							(!strcmp(interpolation,"all")                                   && abs(first-last)==1) ) {
					feature->SetField("error", "needless interpolation");
				} else if (!strcmp(interpolation,"even") && ( first%2==1 || last%2==1 )) {
					feature->SetField("error", "interpolation even but number odd");
				} else if (!strcmp(interpolation,"odd") && ( first%2==0 || last%2==0 )) {
					feature->SetField("error", "interpolation odd but number even");
				} else if (
					(first_taglist.get_value_by_key(std::string("addr:street"))   != last_taglist.get_value_by_key(std::string("addr:street")))   ||
					(first_taglist.get_value_by_key(std::string("addr:postcode")) != last_taglist.get_value_by_key(std::string("addr:postcode"))) ||
					(first_taglist.get_value_by_key(std::string("addr:city"))     != last_taglist.get_value_by_key(std::string("addr:city")))     ||
					(first_taglist.get_value_by_key(std::string("addr:country"))  != last_taglist.get_value_by_key(std::string("addr:country")))  ||
					(first_taglist.get_value_by_key(std::string("addr:full"))     != last_taglist.get_value_by_key(std::string("addr:full")))     ||
					(first_taglist.get_value_by_key(std::string("addr:place"))    != last_taglist.get_value_by_key(std::string("addr:place"))) ) {

					feature->SetField("error", "different tags on endpoints");
				} else if ( // no interpolation error
						(!strcmp(interpolation, "all")) ||
						(!strcmp(interpolation, "odd")) ||
						(!strcmp(interpolation, "even")) ||
						(correct_alphanumeric == 1)) {
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
							nrstr = first_number + static_cast<char>(nr);
						}

						m_clpp.process_interpolated_node( // osmi_addresses_connection_line 
								*(point.get()),
								road_id,
								first_taglist.get_value_by_key(std::string("addr:street"))
						);
						m_nwa_writer.process_interpolated_node( //osmi_addresses_nodes_with_addresses
							*(point.get()),
								nrstr,
								//nr,
								//std::to_string(nr),
								first_taglist.get_value_by_key(std::string("addr:street")),
								first_taglist.get_value_by_key(std::string("addr:postcode")),
								first_taglist.get_value_by_key(std::string("addr:city")),
								first_taglist.get_value_by_key(std::string("addr:country")),
								first_taglist.get_value_by_key(std::string("addr:full")),
								first_taglist.get_value_by_key(std::string("addr:place")),
								road_id
						);
					}
				}

				create_feature(feature);

			}


		} catch (osmium::geom::geometry_error& e) {
			catch_geometry_error(e, way);
		}
	}

	void feed_relation(const osmium::Relation& /* relation */) {

	}

private:
	GeometryHelper m_geometry_helper;
	NodesWithAddressesWriter& m_nwa_writer; //osmi_addresses_nodes_with_addresses
	ConnectionLinePreprocessor& m_clpp; //osmi_addresses_connection_line 

};

#endif /* INTERPOLATIONWRITER_HPP_ */
