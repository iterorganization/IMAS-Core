#include "uda_xml.hpp"

#include <deque>
#include <boost/algorithm/string.hpp>

#include "uda_path.hpp"
#include "uda_utilities.hpp"

int imas::uda::get_rank(const std::string& data_type)
{
    std::vector<std::string> tokens;
    boost::split(tokens, data_type, boost::is_any_of("_"), boost::token_compress_on);
    if (tokens.size() == 2 && tokens.back() != "type") {
        std::string dim = tokens.back();
        if (dim.size() == 2 && dim[1] == 'D') {
            std::string num(1, dim[0]);
            int n = std::stoi(num);
            if (tokens[0] == "STR") {
                return n + 1;
            } else {
                return n;
            }
        } else {
            return 0;
        }
    } else if (tokens.size() == 3 && tokens.back() == "type") {
        std::string dim = tokens[1];
        if (dim.size() == 2 && dim[1] == 'd') {
            std::string num(1, dim[0]);
            return std::stoi(num);
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

pugi::xml_node imas::uda::find_node(const pugi::xml_node& root, const std::string& path, bool top_level)
{
    std::deque<std::string> tokens;
    boost::split(tokens, path, boost::is_any_of("/"), boost::token_compress_on);

    pugi::xml_node node = root;

    while (!tokens.empty()) {
        auto token = tokens.front();
        tokens.pop_front();
        if (std::string("IDS") == node.name() && imas::is_integer(token)) {
            // Skip occurrence number
            continue;
        }
        if (token.find('[') != std::string::npos) {
            token = token.substr(0, token.find('['));
        }
        if (token.find('(') != std::string::npos) {
            token = token.substr(0, token.find('('));
        }
        if (top_level) {
            node = node.find_child_by_attribute("IDS", "name", token.c_str());
        } else {
            node = node.find_child_by_attribute("field", "name", token.c_str());
        }
        if (node.empty()) {
            throw std::invalid_argument("cannot find node " + path + " in data dictionary (" + token + " not found)");
        }
        top_level = false;
    }

    return node;
}

std::shared_ptr<pugi::xml_document> imas::uda::load_xml()
{
    std::string dd_path;

    const char* env = getenv("IDSDEF_PATH");
    if (env == nullptr) {
        env = getenv("IMAS_PREFIX");
        if (env == nullptr) {
            throw std::runtime_error("neither IMAS_PREFIX or IDSDEF_PATH environmental variable is set");
        }
        dd_path = env;
        dd_path += "/include/IDSDef.xml";
    } else {
        dd_path = env;
    }

    auto doc = std::make_shared<pugi::xml_document>();
    pugi::xml_parse_result result = doc->load_file(dd_path.c_str());
    if (!result) {
        throw std::runtime_error("IDSDef.xml not found at either $IDSDEF_PATH or $IMAS_PREFIX/include/IDSDef.xml");
    }

    return doc;
}

void imas::uda::get_attributes(imas::uda::AttributeMap& attributes, std::string ids_path, const pugi::xml_node& node)
{
    std::string dtype = node.attribute("data_type").value();
    std::string timebase = node.attribute("timebasepath").value();

    if (dtype == "struct_array") {
        std::vector<std::string> tokens;
        boost::split(tokens, ids_path, boost::is_any_of("/"), boost::token_compress_on);

        auto type_attr = node.attribute("type");
        attributes[ids_path] = { (type_attr.empty() ? "static" : type_attr.value()), dtype, timebase, 0, 0 };

        if (imas::is_index(tokens.back())) {
            for (const auto& child : node.children("field")) {
                get_attributes(attributes, ids_path + "/" + child.attribute("name").value(), child);
            }
        } else {
            std::string path = ids_path + "[0]";
            for (const auto& child : node.children("field")) {
                get_attributes(attributes, path + "/" + child.attribute("name").value(), child);
            }
        }
    } else if (!dtype.empty() && dtype != "structure") {
        attributes[ids_path] = { node.attribute("type").value(), dtype, timebase, convert_xml_type_to_imas(dtype), get_rank(dtype) };

        for (const auto& child : node.children("field")) {
            get_attributes(attributes, ids_path + "/" + child.attribute("name").value(), child);
        }
    } else {
        for (const auto& child : node.children("field")) {
            get_attributes(attributes, ids_path + "/" + child.attribute("name").value(), child);
        }
    }
}

void imas::uda::get_requests(
        std::vector<std::string>& requests, imas::uda::AttributeMap& attributes,
        std::string ids_path, const pugi::xml_node& node, bool walk_arrays)
{
    std::string dtype = node.attribute("data_type").value();
    std::string timebase = node.attribute("timebasepath").value();

    if (dtype == "struct_array") {
        std::vector<std::string> tokens;
        boost::split(tokens, ids_path, boost::is_any_of("/"), boost::token_compress_on);

        auto type_attr = node.attribute("type");
        attributes[ids_path] = { (type_attr.empty() ? "static" : type_attr.value()), dtype, timebase, 0, 0 };

        if (walk_arrays) {
            if (imas::is_index(tokens.back())) {
                for (const auto& child : node.children("field")) {
                    get_requests(requests, attributes, ids_path + "/" + child.attribute("name").value(), child, walk_arrays);
                }
            } else {
                std::string path = ids_path + "[:]";
                for (const auto& child : node.children("field")) {
                    get_requests(requests, attributes, path + "/" + child.attribute("name").value(), child, walk_arrays);
                }
            }
        } else {
            requests.push_back(ids_path);
        }
    } else if (!dtype.empty() && dtype != "structure") {
        requests.push_back(ids_path);
        attributes[ids_path] = { node.attribute("type").value(), dtype, timebase, convert_xml_type_to_imas(dtype), get_rank(dtype) };

        for (const auto& child : node.children("field")) {
            get_requests(requests, attributes, ids_path + "/" + child.attribute("name").value(), child, walk_arrays);
        }
    } else {
        for (const auto& child : node.children("field")) {
            get_requests(requests, attributes, ids_path + "/" + child.attribute("name").value(), child, walk_arrays);
        }
    }
}

std::vector<std::string> imas::uda::generate_ids_paths(const std::string& path, pugi::xml_node nodes)
{
    std::deque<std::string> tokens;
    boost::split(tokens, path, boost::is_any_of("/"), boost::token_compress_on);

    if (tokens.empty()) {
        return {};
    }

    std::vector<std::string> ids_paths;

    ids_paths.emplace_back("");
    std::string delim;

    while (!tokens.empty()) {
        std::string token = tokens.front();

        if (imas::is_index(token)) {
            std::string name;
            imas::Range range{};
            std::tie(name, range) = imas::parse_index(token);

            nodes = nodes.find_child_by_attribute("name", name.c_str());
            for (auto& ids_path : ids_paths) {
                ids_path += delim + name;
            }

            std::vector<std::string> new_paths;
            for (const auto& ids_path : ids_paths) {
                for (long i = range.begin; i < range.end; i += range.stride) {
                    std::string new_path = ids_path + "[" + std::to_string(i) + "]";
                    new_paths.push_back(new_path);
                }
            }
            ids_paths = new_paths;
        } else {
            nodes = nodes.find_child_by_attribute("name", token.c_str());
            for (auto& ids_path : ids_paths) {
                ids_path += delim + token;
            }
        }

        delim = "/";
        tokens.pop_front();
    }

    return ids_paths;
}

std::string imas::uda::get_dynamic_flags(const AttributeMap& attributes, const std::string& path)
{
    std::deque<int> flags = {};

    auto is_dynamic = (attributes.at(path).type == "dynamic");
    flags.push_front(is_dynamic);

    std::vector<std::string> tokens;
    boost::split(tokens, path, boost::is_any_of("/"), boost::token_compress_on);

    while (!tokens.empty()) {
        while (!tokens.empty() && !boost::ends_with(tokens.back(), "[:]")) {
            tokens.pop_back();
        }

        if (!tokens.empty()) {
            std::string new_path = boost::join(tokens, "/");
            new_path = new_path.substr(0, new_path.size() - 3);

            is_dynamic = (attributes.at(new_path).type == "dynamic");
            flags.push_front(is_dynamic);

            tokens.pop_back();
        }
    }

    std::vector<std::string> string_flags;
    std::transform(flags.begin(), flags.end(), std::back_inserter(string_flags), [](bool flag){ return std::to_string(flag); });

    return boost::join(string_flags, ";");
}

int imas::uda::convert_xml_type_to_imas(const std::string& data_type)
{
    std::vector<std::string> tokens;
    boost::split(tokens, data_type, boost::is_any_of("_"), boost::token_compress_on);
    if (tokens[0] == "INT" || tokens[0] == "int") {
        return INTEGER_DATA;
    } else if (tokens[0] == "FLT" || tokens[0] == "flt") {
        return DOUBLE_DATA;
    } else if (tokens[0] == "STR") {
        return CHAR_DATA;
    }
    throw std::runtime_error("unknown type " + tokens[0]);
}
