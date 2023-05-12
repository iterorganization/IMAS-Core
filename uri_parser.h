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

enum class Error {
    None,
    InvalidScheme,
    InvalidPort,
};

struct Authority {
    std::string authority;
    std::string userinfo;
    std::string host;
    long port = 0;

    std::string to_string() const {
        std::ostringstream ss;
        if (!userinfo.empty()) {
            ss << userinfo << "@";
        }
        ss << authority;
        return ss.str();\
    }
};

class OptionalValue {
public:
    explicit OptionalValue(std::string param)
        : param_{std::move(param)}
        , value_{}
        , found_{false}
    {}
    OptionalValue(std::string param, std::string value)
        : param_{std::move(param)}
        , value_{std::move(value)}
        , found_{true}
    {}

    explicit operator bool() const { return found_; }
    std::string value() const {
        if (!found_) {
            throw UALBackendException("URI query parameter " + param_ + " not found", LOG);
        }
        return value_;
    }
    std::string value_or(const std::string& other) const {
        return found_ ? value_ : other;
    }
private:
    std::string param_;
    std::string value_;
    bool found_;
};

inline std::string join(const std::unordered_set<std::string>& set) {
    std::ostringstream ss;
    const char* delim = "";
    for (const auto& el : set) {
        ss << delim << el;
        delim = ";";
    }
    return ss.str();
}

class QueryDict {
public:
    bool empty() const {
        return map_.empty();
    }
    OptionalValue get(const std::string& name) const {
        auto got = map_.find(name);
        if (got != map_.end()) {
            return {name, join(got->second) };
        }
        else {
            return OptionalValue(name);
        }
    }
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
    bool remove(const std::string& name) {
        auto got = map_.find(name);
        if (got != map_.end()) {
            map_.erase(got);
            return true;
        }
        return false;
    }
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
    std::unordered_map<std::string, std::unordered_set<std::string>> map_;
};

struct Uri {
    Error error;
    std::string scheme;
    Authority authority = {};
    std::string path;
    QueryDict query = {};
    std::string fragment;

    explicit Uri(Error error) : error(error) {}
    Uri(std::string scheme, Authority authority, std::string path, QueryDict query, std::string fragment)
        : error(Error::None)
        , scheme(std::move(scheme))
        , authority(std::move(authority))
        , path(std::move(path))
        , query(std::move(query))
        , fragment(std::move(fragment))
        {}

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

bool valid_scheme(uri::string_arg_type scheme) {
    if (scheme.empty()) {
        return false;
    }
    auto pos = std::find_if_not(scheme.begin(), scheme.end(), [&](char c){
        return std::isalnum(c) || c == '+' || c == '.' || c == '-';
    });
    return pos == scheme.end();
}

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

std::tuple<std::string, uri::Error, uri::string_view_type> parse_fragment(uri::string_arg_type uri) {
    return { std::string(uri), uri::Error::None, uri };
}

} // anon namespace

namespace uri {

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
