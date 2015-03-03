#ifndef ALTTAGLIST_HPP_
#define ALTTAGLIST_HPP_

class AltTagList {
public:
	AltTagList(){

	}


	AltTagList(const osmium::TagList* taglist) {
		for (auto it = taglist->cbegin(); it!=taglist->cend(); ++it) {
			m_list.insert(std::pair<std::string, std::string>(std::string(it->key()), std::string(it->value())));
		}
	}

	AltTagList(const osmium::TagList* taglist, const std::unordered_set<std::string>* desired_tags) {
		const char* value;
		for (auto it = desired_tags->cbegin(); it!=desired_tags->cend(); ++it) {
			value = taglist->get_value_by_key(it->c_str());
			if (value) {
				m_list.insert(std::pair<std::string, std::string>(*it,std::string(value)));
			}
		}
	}

	const char* get_value_by_key_as_c_str(const std::string& key) {
		if (m_list.find(key) == m_list.end()) {
			return nullptr;
		} else {
			return m_list.find(key)->second.c_str();
		}
	}

	const char* get_value_by_key_as_c_str(const char* key) {
		if (!key) {
			return nullptr;
		} else {
			return m_list.find(std::string(key))->second.c_str();
		}
	}

	std::string get_value_by_key(const std::string& key) {
		if (m_list.find(key) == m_list.end()) {
			return std::string("");
		} else {
			return m_list.find(key)->second;
		}
	}

	std::string get_value_by_key(char* key) {
		if (!key) {
			return std::string("");
		} else {
			return m_list.find(std::string(key))->second;
		}
	}

	bool operator==(const AltTagList& /* other */) const {
		return false; //TODO: define a meaningful equivalence operator; the operator==() is exclusively used in sparse_table.hpp in class SparseTable (in function get()); so for the moment this works.
	}

	bool operator!=(const AltTagList& /* other */) const {
		return true; //TODO: define a meaningful equivalence operator; the operator==() is exclusively used in sparse_table.hpp in class SparseTable (in function get()); so for the moment this works.
	}

private:
	std::map <std::string, std::string> m_list;
};

#endif /* ALTTAGLIST_HPP_ */
