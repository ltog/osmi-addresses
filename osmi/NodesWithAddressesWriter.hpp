#ifndef NODESWITHADDRESSESWRITER_HPP_
#define NODESWITHADDRESSESWRITER_HPP_

#include "GeometryHelper.hpp"

#define IS_INTERPOLATION     true
#define IS_NOT_INTERPOLATION false

class NodesWithAddressesWriter : public Writer {

public:
	NodesWithAddressesWriter(const std::string& dir_name) :
		Writer(dir_name, "osmi_addresses_nodes_with_addresses", USE_TRANSACTIONS, wkbPoint) {

		std::vector<field_config> field_configurations;
		field_configurations.push_back({"node_id",    OFTString,  NO_WIDTH});
		field_configurations.push_back({"way_id",     OFTString,  NO_WIDTH});
		field_configurations.push_back({"ip_id",      OFTString,  NO_WIDTH});
		field_configurations.push_back({"is_ip",      OFTInteger, NO_WIDTH});
		field_configurations.push_back({"street",     OFTString,  NO_WIDTH});
		field_configurations.push_back({"houseno",    OFTString,  NO_WIDTH});
		field_configurations.push_back({"postcode",   OFTString,  NO_WIDTH});
		field_configurations.push_back({"city",       OFTString,  NO_WIDTH});
		field_configurations.push_back({"country",    OFTString,  NO_WIDTH});
		field_configurations.push_back({"fulladdr",   OFTString,  NO_WIDTH});
		field_configurations.push_back({"place",      OFTString,  NO_WIDTH});
		field_configurations.push_back({"road_id",    OFTString,  NO_WIDTH});
		field_configurations.push_back({"lastchange", OFTString,  NO_WIDTH});
		create_fields(field_configurations);
	}

	void feed_node(const osmium::Node& /* node */) {

	}

	void process_node(const osmium::Node& node, const std::string& road_id) {
		const char* addrstreet = node.tags().get_value_by_key("addr:street");
		const char* houseno    = node.tags().get_value_by_key("addr:housenumber");
		const char* postcode   = node.tags().get_value_by_key("addr:postcode");
		const char* city       = node.tags().get_value_by_key("addr:city");
		const char* country    = node.tags().get_value_by_key("addr:country");
		const char* fulladdr   = node.tags().get_value_by_key("addr:full");
		const char* place      = node.tags().get_value_by_key("addr:place");
		
		if (addrstreet || houseno || postcode || city || country || fulladdr || place) {
			OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());
			std::unique_ptr<OGRPoint> ogr_point = m_factory.create_point(node);
			feature->SetGeometry(ogr_point.get());
			feature->SetField("node_id", static_cast<double>(node.id())); //TODO: node.id() is of type int64_t. is this ok?
			feature->SetField("is_ip", IS_NOT_INTERPOLATION);
			feature->SetField("lastchange", node.timestamp().to_iso().c_str());
			if (addrstreet) { feature->SetField("street"  , addrstreet); }
			if (houseno)    { feature->SetField("houseno" , houseno);    }
			if (postcode)   { feature->SetField("postcode", postcode);   }
			if (city)       { feature->SetField("city",     city);       }
			if (country)    { feature->SetField("country",  country);    } // TODO: make caps
			if (fulladdr)   { feature->SetField("fulladdr", fulladdr);   }
			if (place)      { feature->SetField("place",    place);      }

			if (road_id != "") { // TODO: road_id is always "" ?
				feature->SetField("road_id", road_id.c_str());
			}

			create_feature(feature);
		}
	}

	void feed_way(const osmium::Way& /* way */) {

	}

	// process OSM-ways with tag building=...
	void process_way(const osmium::Way& way, const std::string& road_id) {
		try {
			const char* building = way.tags().get_value_by_key("building");
			if (building && way.is_closed()) {
				const char* street   = way.tags().get_value_by_key("addr:street");
				const char* houseno  = way.tags().get_value_by_key("addr:housenumber");
				if (street || houseno) {

					std::unique_ptr<OGRPoint> centroid = m_geometry_helper.centroid(way);

					const char* postcode = way.tags().get_value_by_key("addr:postcode");
					const char* city     = way.tags().get_value_by_key("addr:city");
					const char* country  = way.tags().get_value_by_key("addr:country");
					const char* fulladdr = way.tags().get_value_by_key("addr:full");
					const char* place    = way.tags().get_value_by_key("addr:place");

					OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());
					feature->SetGeometry(centroid.get());
					feature->SetField("way_id", static_cast<double>(way.id())); //TODO: node.id() is of type int64_t. is this ok?
					feature->SetField("is_ip", IS_NOT_INTERPOLATION);
					feature->SetField("lastchange", way.timestamp().to_iso().c_str());
					if (street)   { feature->SetField("street"  , street);   }
					if (houseno)  { feature->SetField("houseno" , houseno);  }
					if (postcode) { feature->SetField("postcode", postcode); }
					if (city)     { feature->SetField("city",     city);     }
					if (country)  { feature->SetField("country",  country);  }
					if (fulladdr) { feature->SetField("fulladdr", fulladdr); }
					if (place)    { feature->SetField("place",    place);    }

					if (road_id != "") { // TODO: road_id is always "" ?
						feature->SetField("road_id", road_id.c_str());
					}

					create_feature(feature);
				}
			}
		}
		catch (osmium::geometry_error& e) {
			catch_geometry_error(e, way);
		}
	}

	void feed_relation(const osmium::Relation& /* relation */) {

	}

	void process_interpolated_node(
			OGRPoint& point,
			//const unsigned int houseno,
			const std::string& houseno,
			const std::string& street,
			const std::string& postcode,
			const std::string& city,
			const std::string& country,
			const std::string& full,
			const std::string& place,
			const std::string& road_id
	) {
		OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());
		feature->SetGeometry(static_cast<OGRGeometry*>(&point));
		//feature->SetField("houseno" , static_cast<int>(houseno));
		if (houseno.c_str())   { feature->SetField("houseno",   houseno.c_str());   }
		feature->SetField("is_ip", IS_INTERPOLATION);
		if (street.c_str())   { feature->SetField("street",   street.c_str());   }
		if (postcode.c_str()) { feature->SetField("postcode", postcode.c_str()); }
		if (city.c_str())     { feature->SetField("city",     city.c_str());     }
		if (country.c_str())  { feature->SetField("country",  country.c_str());  }
		if (full.c_str())     { feature->SetField("fulladdr", full.c_str()); }
		if (place.c_str())    { feature->SetField("place",    place.c_str());    }

		if (road_id != "") { // TODO: road_id is always "" ?
			feature->SetField("road_id", road_id.c_str());
		}

		create_feature(feature);
	}

private:
	GeometryHelper m_geometry_helper;

};


#endif /* NODESWITHADDRESSESWRITER_HPP_ */
