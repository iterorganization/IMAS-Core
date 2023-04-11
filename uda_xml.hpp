#pragma once

#ifndef IMAS_UDA_XML_HPP
#define IMAS_UDA_XML_HPP

/**
 * Functionality for processing the IMAS data dictionary XML and for generating all sub-requests required to populate
 * the sub-structures of the IDS, given a node in the tree.
 */

#include <map>
#include <memory>
#include <vector>

#include "pugixml.hpp"

namespace imas {
namespace uda {

/**
 * A structure for representing different attribute values stored on the IDS nodes in the XML.
 */
struct Attribute
{
    std::string type;
    std::string dtype;
    int data_type;
    int rank;
};

/**
 * Type alias for the map going from the node name to the XML attributes found for that node.
 */
using AttributeMap = std::map<std::string, Attribute>;

/**
 * Find the node in the XML document with the specified path.
 *
 * @param root the root node of the XML document
 * @param path the path to find
 * @param top_level whether the provided node is root, used in future recursive calls
 * @return the found node
 * @throw std::invalid_argument if the node cannot be found
 */
pugi::xml_node find_node(const pugi::xml_node& root, const std::string& path, bool top_level=true);

/**
 * Load the IMAS data dictionary XML.
 *
 * The path of the file is given by the IDSDEF_PATH environmental variable, or constructed from the IMAS_PREFIX
 * environmental variable as follows:
 *  `$IMAS_PREFIX/include/IDSDef.xml`
 *
 * @return the loaded XML document
 * @throw std::runtime_error if the XML file cannot be loaded
 */
std::shared_ptr<pugi::xml_document> load_xml();

/**
 * Generate all the IDS paths required to populate the IDS data tree at and below the specified ids_path.
 *
 * @param requests [OUT] the generated IDS paths
 * @param attributes [OUT] the attribute values read from the XML for the found paths
 * @param ids_path the path in the IDS we start generating from
 * @param node the node corresponding to the given ids_path
 * @param walk_arrays whether to recurse down into struct_arrays
 */
void get_requests(
        std::vector<std::string>& requests, imas::uda::AttributeMap& attributes,
        std::string ids_path, const pugi::xml_node& node, bool walk_arrays=true);

/**
 * Find the attributes for all the IDS paths at and below the specified ids_path.
 *
 * @param attributes [OUT] the found attributes
 * @param ids_path the path in the IDS we start the search from
 * @param node the node corresponding to the given ids_path
 */
void get_attributes(imas::uda::AttributeMap& attributes, std::string ids_path, const pugi::xml_node& node);

/**
 * Extract the data rank from the data dictionary XML type.
 *
 * @param data_type the data dictionary XML type
 * @return the rank
 */
int get_rank(const std::string& data_type);

/**
 * Expand IDS path to resolve any specified range requests.
 *
 * I.e. the path `magnetics/flux_loop[1:3]/flux/data` would be expanded into the following paths:
 *  - `magnetics/flux_loop[1]/flux/data`
 *  - `magnetics/flux_loop[2]/flux/data`
 *  - `magnetics/flux_loop[3]/flux/data`
 *
 * @param path the path to expand
 * @param nodes the node corresponding to the given path
 * @param size_requests
 * @return
 */
std::vector<std::string> generate_ids_paths(const std::string& path, pugi::xml_node nodes);

/**
 * Generate the list of flags specifying whether each part of the given path is marked as dynamic in the data dictionary
 * XML. This list is returned as a comma separated string, i.e. "0;0;1".
 *
 * @param attributes the attribute map to look up the dynamic attribute for the different paths
 * @param path the path to get the dynamic flags for
 * @return a string representing the comma separated list of flags
 */
std::string get_dynamic_flags(const imas::uda::AttributeMap& attributes, const std::string& path);

/**
 * Convert the type string read from the XML into IMAS data type constant.
 *
 * @param data_type the string data type read from the XML node
 * @return the integer IMAS constant equivalent of the data type
 */
int convert_xml_type_to_imas(const std::string& data_type);

}} // namespace imas::uda

#endif // IMAS_UDA_XML_HPP
