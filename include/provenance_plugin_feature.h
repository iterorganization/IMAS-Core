#ifndef PROVENANCE_PLUGIN_FEATURE_H
#define PROVENANCE_PLUGIN_FEATURE_H 1

#include <string>

class provenance_plugin_feature {

    public:

    provenance_plugin_feature() {}
    virtual ~provenance_plugin_feature() {}

    virtual std::string getName() = 0;
    virtual std::string getDescription() = 0;
    virtual std::string getCommit() = 0;
    virtual std::string getVersion() = 0;
    virtual std::string getRepository() = 0;
    virtual std::string getParameters() = 0;

};

#endif
