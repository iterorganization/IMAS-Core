#ifndef READBACK_PLUGIN_FEATURE_H
#define READBACK_PLUGIN_FEATURE_H 1

#include <string>

class readback_plugin_feature {

    public:

    readback_plugin_feature() {}
    virtual ~readback_plugin_feature() {}

    virtual std::string getReadbackName(const std::string &path, int *application_index) = 0;
    virtual std::string getReadbackDescription(const std::string &path) = 0;
    virtual std::string getReadbackCommit(const std::string &path) = 0;
    virtual std::string getReadbackVersion(const std::string &path) = 0;
    virtual std::string getReadbackRepository(const std::string &path) = 0;
    virtual std::string getReadbackParameters(const std::string &path) = 0;

};

#endif
