#ifndef WAYSWITHADDRESSESWRITER_HPP_
#define WAYSWITHADDRESSESWRITER_HPP_

class WaysWithAddressesWriter : public Writer {

public:

	WaysWithAddressesWriter(const std::string& dir_name) :
		Writer(dir_name, "osmi_addresses_ways_with_addresses", USE_TRANSACTIONS, wkbPolygon) {

		std::vector<field_config> field_configurations;
		field_configurations.push_back({"way_id",     OFTString,  NO_WIDTH});
		field_configurations.push_back({"street",     OFTString,  NO_WIDTH});
		field_configurations.push_back({"houseno",    OFTString,  NO_WIDTH});
		field_configurations.push_back({"postcode",   OFTString,  NO_WIDTH});
		field_configurations.push_back({"city",       OFTString,  NO_WIDTH});
		field_configurations.push_back({"country",    OFTString,  NO_WIDTH});
		field_configurations.push_back({"fulladdr",   OFTString,  NO_WIDTH});
		field_configurations.push_back({"place",      OFTString,  NO_WIDTH});
		field_configurations.push_back({"lastchange", OFTString,  NO_WIDTH});
		create_fields(field_configurations);
	}

	void feed_way(const osmium::Way& way) override {
		try {
			const char* building = way.tags().get_value_by_key("building");
			if (building && way.is_closed()) {
				const char* street   = way.tags().get_value_by_key("addr:street");
				const char* houseno  = way.tags().get_value_by_key("addr:housenumber");
				const char* place    = way.tags().get_value_by_key("addr:place");
				
				if (street || houseno || place) {
					std::unique_ptr<OGRPolygon> polygon = m_factory.create_polygon(way);
					OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());
					feature->SetGeometryDirectly(static_cast<OGRGeometry*>(polygon.release()));
					feature->SetField("way_id", static_cast<double>(way.id())); //TODO: node.id() is of type int64_t. is this ok?
					feature->SetField("lastchange", way.timestamp().to_iso().c_str());

					const char* postcode = way.tags().get_value_by_key("addr:postcode");
					const char* city     = way.tags().get_value_by_key("addr:city");
					const char* country  = way.tags().get_value_by_key("addr:country");
					const char* fulladdr = way.tags().get_value_by_key("addr:full");

					if (street)   { feature->SetField("street"  , street);   }
					if (houseno)  { feature->SetField("houseno" , houseno);  }
					if (postcode) { feature->SetField("postcode", postcode); }
					if (city)     { feature->SetField("city",     city);     }
					if (country)  { feature->SetField("country",  country);  }
					if (fulladdr) { feature->SetField("fulladdr", fulladdr); }
					if (place)    { feature->SetField("place",    place);    }

					create_feature(feature);

				}
			}
		}
		catch (const osmium::geometry_error& e) {
			catch_geometry_error(e, way);
		}
	}

};




#endif /* WAYSWITHADDRESSESWRITER_HPP_ */
