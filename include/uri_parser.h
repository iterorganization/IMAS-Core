/**
 * @file uri_parser.h
 * @brief A simple URI parser library.
 */

#pragma once

#ifndef SIMPLE_URI_PARSER_LIBRARY_H
#define SIMPLE_URI_PARSER_LIBRARY_H

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_set>

#ifndef simple_uri_CPLUSPLUS
# if defined(_MSVC_LANG ) && !defined(__clang__)
#  define simple_uri_CPLUSPLUS (_MSC_VER == 1900 ? 201103L : _MSVC_LANG )
# else
#  define simple_uri_CPLUSPLUS __cplusplus
# endif
#endif

#define simple_uri_CPP17_OR_GREATER  ( simple_uri_CPLUSPLUS >= 201703L )

namespace uri {

#if simple_uri_CPP17_OR_GREATER
  using string_view_type = std::string_view;
  using string_arg_type = std::string_view;
  constexpr auto npos = std::string_view::npos;
#else
  using string_view_type = std::string;
  using string_arg_type = const std::string&;
  constexpr auto npos = std::string::npos;
#endif

using query_type = std::unordered_map<std::string, std::vector<std::string>>;

/**
 * @enum Error
 * @brief Enumeration for URI parsing errors.
 */
enum class Error {
    None,           /**< No error */
    InvalidScheme,  /**< Invalid scheme error */
    InvalidPort,    /**< Invalid port error */
};

/**
 * @struct Authority
 * @brief Represents the authority component of a URI.
 */
struct Authority {
    std::string authority; /**< The authority string */
    std::string userinfo;  /**< The user info string */
    std::string host;      /**< The host string */
    long port = 0;         /**< The port number */

    /**
     * @brief Converts the authority to a string.
     * @return The authority as a string.
     */
    std::string to_string() const {
        std::ostringstream ss;
        if (!userinfo.empty()) {
            ss << userinfo << "@";
        }
        ss << authority;
        return ss.str();\
    }
};

/**
 * @class OptionalValue
 * @brief Represents an optional value for a URI query parameter.
 */
class OptionalValue {
public:
    /**
     * @brief Constructs an OptionalValue with a parameter name.
     * @param param The parameter name.
     */
    explicit OptionalValue(std::string param)
        : param_{std::move(param)}
        , value_{}
        , found_{false}
    {}

    /**
     * @brief Constructs an OptionalValue with a parameter name and value.
     * @param param The parameter name.
     * @param value The parameter value.
     */
    OptionalValue(std::string param, std::string value)
        : param_{std::move(param)}
        , value_{std::move(value)}
        , found_{true}
    {}

    /**
     * @brief Checks if the value is found.
     * @return True if the value is found, otherwise false.
     */
    explicit operator bool() const { return found_; }

    /**
     * @brief Gets the value.
     * @return The value.
     * @throws ALBackendException if the value is not found.
     */
    std::string value() const {
        if (!found_) {
            throw ALBackendException("URI query parameter " + param_ + " not found", LOG);
        }
        return value_;
    }
    /**
     * @brief Gets the value or a default value if not found.
     * @param other The default value.
     * @return The value or the default value.
     */
    std::string value_or(const std::string& other) const {
        return found_ ? value_ : other;
    }

private:
    std::string param_; /**< The parameter name */
    std::string value_; /**< The parameter value */
    bool found_;        /**< Whether the value is found */
};

/**
 * @brief Joins a set of strings with a semicolon delimiter.
 * @param set The set of strings.
 * @return The joined string.
 */
inline std::string join(const std::unordered_set<std::string>& set) {
    std::ostringstream ss;
    const char* delim = "";
    for (const auto& el : set) {
        ss << delim << el;
        delim = ";";
    }
    return ss.str();
}


/**
 * @class QueryDict
 * @brief Represents a dictionary of URI query parameters.
 */
class QueryDict {
public:
    /**
     * @brief Checks if the query dictionary is empty.
     * @return True if empty, otherwise false.
     */
    bool empty() const {
        return map_.empty();
    }

    /**
     * @brief Try and get the argument from the query dictionary.
     * @param name the argument to return
     * @return The value wrapped in an OptionalValue. if found, otherwise an empty OptionalValue
     */
    OptionalValue get(const std::string& name) const {
        auto got = map_.find(name);
        if (got != map_.end()) {
            return {name, join(got->second) };
        }
        else {
            return OptionalValue(name);
        }
    }

    /**
     * @brief Set the value for the argument given, overwriting any existing values if they exist.
     * @param name the name of the argument
     * @param value the value to set for the argument
     */
    void set(const std::string& name, const std::string& value) {
        if (!value.empty()) {
            map_[name] = {value};
        } else {
            map_[name] = {};
        }
    }

    /**
     * @brief Insert an additional value for the given argument, creating a new argument if it didn't already exist.
     * @param name the name of the argument
     * @param value the value to insert for the argument
     */
    void insert(const std::string& name, const std::string& value) {
        auto got = map_.find(name);
        if (got != map_.end()) {
            if (!value.empty()) {
                got->second.insert(value);
            }
        } else if (!value.empty()) {
            map_[name] = {value};
        } else {
            map_[name] = {};
        }
    }

    /**
     * @brief Delete the argument from the query dictionary, or do nothing if the argument is not in the dictionary.
     * @param name the name of the argument to delete
     * @return true if the argument was found
     */
    bool remove(const std::string& name) {
        auto got = map_.find(name);
        if (got != map_.end()) {
            map_.erase(got);
            return true;
        }
        return false;
    }

    /**
     * @brief Return a string representation of the query dictionary, using & separator for the arguments.
     * @return the query dictionaries string representation
     */
    std::string to_string() const {
        std::ostringstream ss;
        const char* delim = "";
        for (const auto& el : map_) {
            if (el.second.empty()) {
                ss << delim << el.first;
            } else {
                ss << delim << el.first << "=" << join(el.second);
            }
            delim = "&";
        }
        return ss.str();
    }

private:
    std::unordered_map<std::string, std::unordered_set<std::string>> map_; /**< The query dictionary map */
};

/**
 * @struct Uri
 * @brief Represents a parsed URI.
 */
struct Uri {
    Error error; /**< The error status */
    std::string scheme; /**< The URI scheme */
    Authority authority = {}; /**< The URI authority */
    std::string path; /**< The URI path */
    QueryDict query = {}; /**< The URI query */
    std::string fragment; /**< The URI fragment */

    /**
     * @brief Constructs a Uri with an error.
     * @param error The error status.
     */
    explicit Uri(Error error) : error(error) {}

    /**
     * @brief Constructs a Uri with components.
     * @param scheme The URI scheme.
     * @param authority The URI authority.
     * @param path The URI path.
     * @param query The URI query.
     * @param fragment The URI fragment.
     */
    Uri(std::string scheme, Authority authority, std::string path, QueryDict query, std::string fragment)
        : error(Error::None)
        , scheme(std::move(scheme))
        , authority(std::move(authority))
        , path(std::move(path))
        , query(std::move(query))
        , fragment(std::move(fragment))
        {}

    /**
     * @brief Converts the URI to a string.
     * @return The URI as a string.
     */
    std::string to_string() const {
        std::ostringstream ss;
        if (error != Error::None) {
            ss << "invalid uri";
        } else {
            std::string authority_string = authority.to_string();
            if (!authority_string.empty()) {
                ss << scheme << "://" << authority.to_string() << path;
            } else {
                ss << scheme << ":" << path;
            }
            if (!query.empty()) {
                ss << "?" << query.to_string();
            }
            if (!fragment.empty()) {
                ss << "#" << fragment;
            }
        }
        return ss.str();
    }
};

}

namespace {

/**
 * @brief Check if scheme is valid.
 * @param scheme scheme to check.
 * @return true if scheme is valid, otherwise false.
 */
bool valid_scheme(uri::string_arg_type scheme) {
    if (scheme.empty()) {
        return false;
    }
    auto pos = std::find_if_not(scheme.begin(), scheme.end(), [&](char c){
        return std::isalnum(c) || c == '+' || c == '.' || c == '-';
    });
    return pos == scheme.end();
}

/**
 * @brief Parses a URI scheme.
 * @param uri The URI string.
 * @return A tuple containing the scheme, error status, and remaining URI string.
 */
std::tuple<std::string, uri::Error, uri::string_view_type> parse_scheme(uri::string_arg_type uri) {
    auto pos = uri.find(':');
    if (pos == uri::npos) {
        return { "", uri::Error::InvalidScheme, uri };
    }

    auto scheme = uri.substr(0, pos);
    if (!::valid_scheme(scheme)) {
        return { "", uri::Error::InvalidScheme, uri };
    }
    std::string scheme_string{ scheme };
    std::transform(scheme_string.begin(), scheme_string.end(), scheme_string.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    return { scheme_string, uri::Error::None, uri.substr(pos + 1) };
}

/**
 * @brief Parses a URI authority.
 * @param uri The URI string.
 * @return A tuple containing the authority, error status, and remaining URI string.
 */
std::tuple<uri::Authority, uri::Error, uri::string_view_type> parse_authority(uri::string_arg_type uri) {
    uri::Authority authority;

    bool has_authority = uri.length() >= 2 && uri[0] == '/' && uri[1] == '/';
    if (!has_authority) {
        return { authority, uri::Error::None, uri };
    }

    auto pos = uri.substr(2).find('/');
    auto auth_string = uri.substr(2, pos);
    auto rem = uri.substr(pos + 2);
    authority.authority = auth_string;

    pos = auth_string.find('@');
    if (pos != uri::npos) {
        authority.userinfo = std::string(auth_string.substr(0, pos));
        auth_string = auth_string.substr(pos + 1);
    }

    char* end_ptr = nullptr;
    if (!auth_string.empty() && auth_string[0] != '[') {
        pos = auth_string.find(':');
        if (pos != uri::npos) {
            authority.port = std::strtol(&auth_string[pos + 1], &end_ptr, 10);
            if (end_ptr != &*auth_string.end()) {
                return { authority, uri::Error::InvalidPort, auth_string };
            }
        }
    }

    authority.host = auth_string.substr(0, pos);

    return { authority, uri::Error::None, rem };
}

/**
 * @brief Parses a URI path.
 * @param uri The URI string.
 * @return A tuple containing the path, error status, and remaining URI string.
 */
std::tuple<std::string, uri::Error, uri::string_view_type> parse_path(uri::string_arg_type uri) {
    auto pos = uri.find_first_of("#?");
    if (pos == uri::npos) {
        auto path = std::string(uri);
        return { path, uri::Error::None, "" };
    } else {
        auto path = std::string(uri.substr(0, pos));
        return { path, uri::Error::None, uri.substr(pos + 1) };
    }
}

/**
 * @brief Parses a URI query.
 * @param uri The URI string.
 * @return A tuple containing the query dictionary, error status, and remaining URI string.
 */
std::tuple<uri::QueryDict, uri::Error, uri::string_view_type> parse_query(uri::string_arg_type uri) {
    auto hash_pos = uri.find('#');
    auto query_substring = uri.substr(0, hash_pos);
    uri::QueryDict query;
    while (!query_substring.empty()) {
        auto delim_pos = query_substring.find_first_of("&;?", 0);
        auto arg = query_substring.substr(0, delim_pos);
        auto equals_pos = arg.find('=');
        std::string name;
        std::string value;
        if (equals_pos == uri::npos) {
            name = std::string(arg);
        } else {
            name = std::string(arg.substr(0, equals_pos));
            value = arg.substr(equals_pos + 1);
        }
        query.insert(name, value);
        if (delim_pos == uri::npos) {
            query_substring = "";
        } else {
            query_substring = query_substring.substr(delim_pos + 1);
        }
    }

    return {query, uri::Error::None, uri.substr(hash_pos + 1) };
}


/**
 * @brief Parses a URI fragment.
 * @param uri The URI string.
 * @return A tuple containing the fragment, error status, and remaining URI string.
 */
std::tuple<std::string, uri::Error, uri::string_view_type> parse_fragment(uri::string_arg_type uri) {
    return { std::string(uri), uri::Error::None, uri };
}

} // anon namespace

namespace uri {

/**
 * @brief Parses a URI string into a Uri object.
 * @param uri_in The URI string.
 * @return The parsed Uri object.
 */
inline Uri parse_uri(uri::string_arg_type uri_in) {
    Error error;

    string_view_type uri;
    std::string scheme;
    std::tie(scheme, error, uri) = ::parse_scheme(uri_in);
    if (error != Error::None) {
        return Uri(error);
    }

    Authority authority;
    std::tie(authority, error, uri) = ::parse_authority(uri);
    if (error != Error::None) {
        return Uri(error);
    }

    std::string path;
    std::tie(path, error, uri) = ::parse_path(uri);
    if (error != Error::None) {
        return Uri(error);
    }

    uri::QueryDict query;
    std::tie(query, error, uri) = ::parse_query(uri);
    if (error != Error::None) {
        return Uri(error);
    }

    std::string fragment;
    std::tie(fragment, error, uri) = ::parse_fragment(uri);
    if (error != Error::None) {
        return Uri(error);
    }

    return { scheme, authority, path, query, fragment };
}

} // namespace uri

#endif // SIMPLE_URI_PARSER_LIBRARY_H
