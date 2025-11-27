#ifndef NBC_PLUGIN_H
#define NBC_PLUGIN_H 1

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>

#include <libxml/xpath.h>
#include <libxml/parser.h>

#include "access_layer_plugin.h"

#include "simple_logger.h"

typedef struct 
      {
        char change_nbc_version[10];
        char change_nbc_description[50];
        char change_nbc_previous_name[100];
        char change_nbc_current_name[100];
      } nbc_infos;
      
class nbc_key {
     public:
       int ctx;
       char field_path[100];

     bool operator<(const nbc_key &k) const {
        if (ctx == k.ctx) {
            return strcmp(field_path, k.field_path) < 0;
        }
        else {
            return ctx < k.ctx;
        }
    }
};
      
class NBC_plugin: public access_layer_plugin
{

  private:
    std::string dataobjectname;
    int occurrence;
    int homogeneous_time;
    
    bool show_nbc_fields;
    bool nbc_debug;
    
    std::string IDSDef_file_path;
    std::string data_dictionary_version;
    xmlDocPtr doc;
    xmlXPathContextPtr xpathCtx;
    
    xmlNodePtr cur;
    xmlNodePtr root;
    
    char xpath_request[500];
    char temp_result[100];
    
    std::map<nbc_key, std::string> visitedPaths; //key=(ctx, field_path), value=full patched path
    void setXMLCurrentNode(const char* fieldPath);
    
    bool getPatchedField(int ctx, const char* fieldPath, std::string &patchedPath); 
    void getNBC_infos(const std::string &xpathKey, std::vector<nbc_infos> &nbc);
    bool nbc_info(int ctx, const char* fieldPath, std::string &patchedField);
    void displayNBC_infos(const nbc_key &nbc_k, const nbc_infos &nbc);
    void displayNBC_infos(const nbc_key &nbc_k, const std::vector<nbc_infos> &nbc_vector);
    void create_xml_doc(const char* ddFile);
    void create_xpath_context();
    void buildXPathExpression(const std::string &fullPath, const char* searchedAttribute);
    char* evaluateXpathExpression(const char* xpathKey);
    void close_xml_doc();
    
    void setRootNode(); 
    void setCurrentNodePtr(const char* xpathKey);
    void getTokens(const std::string &path, std::vector <std::string> &tokens);
    std::string getFullPath(Context *context);

  public:
    NBC_plugin();
    ~NBC_plugin();

     std::string getName() override;
     std::string getDescription() override;
     std::string getCommit() override;
     std::string getVersion() override;
     std::string getRepository() override;
     std::string getParameters() override;
     std::string getReadbackName(const std::string &path, int* index) override;
     std::string getReadbackDescription(const std::string &path) override;
     std::string getReadbackCommit(const std::string &path) override;
     std::string getReadbackVersion(const std::string &path) override;
     std::string getReadbackRepository(const std::string &path) override;
     std::string getReadbackParameters(const std::string &path) override;
     plugin::OPERATION node_operation(const std::string &path) override;
     void setParameter(const char* parameter_name, int datatype, int dim, int *size, void *data) override;
     void begin_arraystruct_action(int ctx, int *aosctx, const char* fieldPath, const char* timeBasePath, int *arraySize) override;
     void begin_global_action(int pulseCtx, const char* dataobjectname, const char* datapath, int mode, int opCtx) override;
     void begin_slice_action(int pulseCtx, const char* dataobjectname, int mode, double time, int interp, int opCtx) override;
     void end_action(int ctx) override;
     int read_data(int ctx, const char* fieldPath, const char* timeBasePath, void **data, int datatype, int dim, int *size) override;
     void write_data(int ctx, const char* fieldPath, const char* timeBasePath, void *data, int datatype, int dim, int *size) override;
};

extern "C" access_layer_plugin* create() {
  return new NBC_plugin;
}

extern "C" void destroy (access_layer_plugin* al_plugin) {
  delete al_plugin;
}

#endif
