#pragma once

#ifndef IMAS_UDA_PREFETCH_UDA_XML_HPP
#define IMAS_UDA_PREFETCH_UDA_XML_HPP

#include <map>

#include "pugixml.hpp"

namespace imas {
namespace uda {

struct Attribute
{
    std::string type;
    std::string dtype;
    int data_type;
    int rank;
};

using AttributeMap = std::map<std::string, Attribute>;

pugi::xml_node find_node(const pugi::xml_node& root, const std::string& path, bool top_level);
std::shared_ptr<pugi::xml_document> load_xml();
void get_requests(std::vector<std::string>& requests, imas::uda::AttributeMap& attributes, std::string ids_path,
                  const pugi::xml_node& node, bool walk_arrays=true);
void get_attributes(imas::uda::AttributeMap& attributes, std::string ids_path, const pugi::xml_node& node);
int get_rank(const std::string& data_type);
std::vector<std::string> generate_ids_paths(const std::string& path, pugi::xml_node& nodes,
                                            std::vector<std::string>& size_requests);
std::string get_dynamic_flags(const imas::uda::AttributeMap& attributes, const std::string& path);
int convert_xml_type_to_imas(const std::string& data_type);

}
}

#endif // IMAS_UDA_PREFETCH_UDA_XML_HPP