#include "nbc_plugin.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

#define MAX_LOOPS 30

#define IDSDEF_FILE_PATH "IDSDef_file_path"
#define VERBOSE_PARAMETER "verbose"
#define SHOW_NBC_FIELDS_OPTION "show_nbc_fields"
#define DEBUG_OPTION "debug"

NBC_plugin::NBC_plugin()
:dataobjectname(), occurrence(-1), homogeneous_time(-1), doc(NULL), show_nbc_fields(false), nbc_debug(false)
{
}

NBC_plugin::~NBC_plugin()
{
}

std::string NBC_plugin::getName() {
    return "nbc";
}

std::string NBC_plugin::getDescription() {
    return "";
}

std::string NBC_plugin::getCommit() {
    return "d92bb2f30384bc618574508b56e9542d00f0e97a";
}

std::string NBC_plugin::getVersion() {
    return "1.0.0";
}

std::string NBC_plugin::getRepository() {
    return "ssh://git@git.iter.org/imas/access-layer.git";
}
std::string NBC_plugin::getParameters() {
    return "";
}

plugin::OPERATION NBC_plugin::node_operation(const std::string &path) {
    return plugin::OPERATION::GET_ONLY;
}

std::string NBC_plugin::getReadbackName(const std::string &path, int* index) {
    return "";
}

std::string NBC_plugin::getReadbackDescription(const std::string &path) {
    return "";
}

std::string NBC_plugin::getReadbackCommit(const std::string &path) {
    return "";
}

std::string NBC_plugin::getReadbackVersion(const std::string &path) {
    return "";
}

std::string NBC_plugin::getReadbackRepository(const std::string &path) {
    return "";
}

std::string NBC_plugin::getReadbackParameters(const std::string &path) {
    return "";
}
 
void NBC_plugin::setParameter(const char* parameter_name, int datatype, int dim, int *size, void *data) {
     if (std::string(parameter_name) == std::string(IDSDEF_FILE_PATH)) {
         assert(data != NULL);
         IDSDef_file_path = std::string((char*) data);
     }
     else if (std::string(parameter_name) == std::string(VERBOSE_PARAMETER)) {
         assert(data != NULL);
         std::string directive = std::string((char*) data);
         if (directive == SHOW_NBC_FIELDS_OPTION) {
             //LOG_DEBUG << "parameter detected :" << directive;
             show_nbc_fields = true;
         }
         else if (directive == DEBUG_OPTION){
             nbc_debug = true;
         }
     }
}

void NBC_plugin::begin_global_action(int pulseCtx, const char* dataobjectname, const char* datapath, int mode, int opCtx) {
    
     //Initializing plugin with IDS name, pulse and occurrence
    if (nbc_debug)
        LOG_DEBUG << "NBC_plugin::begin_global_action, opCtx= " << opCtx;
        
    std::size_t found = std::string(dataobjectname).find("/");
    if (found != std::string::npos) {
        this->dataobjectname = std::string(dataobjectname).substr(0, found);
        std::string occStr = std::string(dataobjectname).substr(found+1, std::string::npos);
        this->occurrence = std::stoi(occStr);
    }
    else {
        this->dataobjectname = std::string(dataobjectname);
        this->occurrence = 0;
    }
    
    if (IDSDef_file_path.empty()) 
        throw ALPluginException("Parameter 'IDSDef_file_path' nod defined for nbc_plugin.");
    
    //Creating/parsing the doc object (Data Dictionary)
    if (doc == NULL)
       create_xml_doc(IDSDef_file_path.c_str());
       
    setRootNode();
    
    visitedPaths.clear();

   if (data_dictionary_version.empty()) {
       char *data;
       int size;
       al_status_t status = al_plugin_read_data(opCtx, "ids_properties/version_put/data_dictionary", "", (void**) &data, CHAR_DATA, 1, &size);
       if (status.code != 0)
           throw ALPluginException(status.message);
       if (data != NULL) 
            data_dictionary_version = std::string(data);
       //LOG_DEBUG << "data_dictionary_version=" << data_dictionary_version;
   }

}

void NBC_plugin::begin_slice_action(int pulseCtx, const char* dataobjectname, int mode, double time, int interp, int opCtx) {
    //LOG_DEBUG << "begin_slice_action called for opCtx=" << opCtx; 
    this->begin_global_action(pulseCtx, dataobjectname, mode, opCtx); //same plugin initialization than begin_global_action, so we call it
}

void NBC_plugin::begin_arraystruct_action(int ctx, int *aosctx, const char* fieldPath, const char* timeBasePath, int *arraySize) {
   
   LLenv lle = Lowlevel::getLLenv(ctx);
   if (nbc_debug) {
       LOG_DEBUG << "arraystruct path =" <<  getFullPath(lle.context) << ", fieldPath=" << fieldPath << ", aosctx=" << *aosctx << ", size=" << *arraySize; 
   }
   std::string patchedPath;
   bool is_patched = getPatchedField(ctx, fieldPath, patchedPath);
   
   std::string patchedTimedBasePath(timeBasePath);
   if (homogeneous_time == 0) {
       getPatchedField(ctx, timeBasePath, patchedTimedBasePath);
   }
   else {
      //nothing to do
   }

   //delete default AOS context if it exists
   if (*aosctx != 0) {
       LLenv lle_aos = Lowlevel::getLLenv(*aosctx);
       ArraystructContext* actx_default = dynamic_cast<ArraystructContext *>(lle_aos.context);
       lle_aos.backend->endAction(actx_default);
       delete(actx_default);
   }
   
   //Creating another AOS context with patched path
   ArraystructContext* actx = lle.create(patchedPath.c_str(), patchedTimedBasePath.c_str());
   *aosctx = Lowlevel::addLLenv(lle.backend, actx); 
   lle.backend->beginArraystructAction(actx, arraySize); //calling backend with patched field name
   
    if (nbc_debug) {
      LOG_DEBUG << "arraystruct path =" << getFullPath(lle.context) << ", patchedPath=" << patchedPath <<  ", is_patched=" <<  is_patched << ", aosctx=" << *aosctx << ", size=" << *arraySize; 
   }
   
   setXMLCurrentNode(fieldPath); //Setting the current node of the Xpath context
}

int NBC_plugin::read_data(int ctx, const char* fieldPath, const char* timeBasePath, 
                void **data, int datatype, int dim, int *size) {
                                      
   std::string patchedPath;
   getPatchedField(ctx, fieldPath, patchedPath);
   
   if (nbc_debug) {
      LLenv lle = Lowlevel::getLLenv(ctx);
      LOG_DEBUG << "read_data::full path=" <<  getFullPath(lle.context) + "/" + std::string(fieldPath);
   }
     
   std::string patchedTimedBasePath(timeBasePath);
   if (homogeneous_time == 0) {
       //we don't need to change the XML node of the current XML context
       getPatchedField(ctx, timeBasePath, patchedTimedBasePath);
   }
   else {
      //nothing to do, we assume that 'time' will never be renamed...
   }
   
   al_status_t status = al_plugin_read_data(ctx, patchedPath.c_str(), patchedTimedBasePath.c_str(), data, datatype, dim, size);
   
   if (status.code < 0)
       throw ALPluginException(status.message);
       
   if (homogeneous_time == -1 && patchedPath == "ids_properties/homogeneous_time") {
       int **h = (int**) data;
       homogeneous_time = *(*h);
       homogeneous_time = 0;
       //LOG_DEBUG << "homogeneous_time=" << homogeneous_time;
   }
       
   return 1;
                      
}

void NBC_plugin::write_data(int ctx, const char* fieldPath, const char* timeBasePath, void *data, int datatype, int dim, int *size) {
    LLenv lle = Lowlevel::getLLenv(ctx);
    if (nbc_debug) {
        LOG_DEBUG << "datatype = " << datatype;
        LOG_DEBUG << "write_data::full path (non patched)=" <<  getFullPath(lle.context) + "/" + std::string(fieldPath);
    }

    //data_dictionary_version = "3.25.0";   

    // LOG_DEBUG << "write_data::full path=" <<  getFullPath(lle.context) + "/" + std::string(fieldPath);
    std::string patchedPath;
    getPatchedField(ctx, fieldPath, patchedPath);
    
    std::string patchedTimedBasePath;
    if (homogeneous_time == 0) {
       //we don't need to change the XML node of the current XML context
       getPatchedField(ctx, timeBasePath, patchedTimedBasePath);
    }
    else {
        //nothing to do, we assume that 'time' will never be renamed...
    }
    
    al_status_t status;
    if (Lowlevel::data_has_non_zero_shape(datatype, data, dim, size)) {
            //LOG_DEBUG << "write_data::full patched path=" <<  getFullPath(lle.context) + "/" + std::string(patchedPath);
			status = al_plugin_write_data(ctx, patchedPath.c_str(), patchedTimedBasePath.c_str(), data, datatype, dim, size);
    }
    if (status.code < 0)
        throw ALPluginException(status.message);
}


void NBC_plugin::end_action(int ctx) {
   LLenv lle = Lowlevel::getLLenv(ctx);
   //LOG_DEBUG << "end_action called for ctx=" << ctx;
   if (lle.context->getType() == CTX_ARRAYSTRUCT_TYPE) {
          //LOG_DEBUG << "end_action called for array struct";
          xpathCtx->node = xpathCtx->node->parent;
          int i = 0;
          while(true) {
              
              if (i > MAX_LOOPS) {
                  throw ALPluginException("NBC plugin: unexpected error!");
              }
             char* value = evaluateXpathExpression("./@data_type");
            
             if (value == NULL) {
                  break;
              }
             else if (strcmp("struct_array", value) == 0) {
                 free(value);
                 break;
             }
             else {
                xpathCtx->node = xpathCtx->node->parent;
                free(value);
             }
             
             i++;
          }
          
          //LOG_DEBUG << "xpathCtx->node node name after search = " << xpathCtx->node->name;
   }
   std::vector<nbc_key> keys;
   for (auto it = visitedPaths.begin(); it != visitedPaths.end(); ++it) {
       const nbc_key &key = it->first;
       if (key.ctx == ctx)
          keys.push_back(key);
   }
   for (size_t i = 0; i < keys.size(); i++) 
      visitedPaths.erase(keys[i]);
}

void NBC_plugin::setXMLCurrentNode(const char* fieldPath) {
     std::string xpathKey = std::string(fieldPath);
     buildXPathExpression(xpathKey, NULL);
     setCurrentNodePtr(xpath_request);
     xpathCtx->node = cur; //the XPath context current node has to be updated
}

bool NBC_plugin::getPatchedField(int ctx, const char* fieldPath, std::string &patchedPath) {

   bool is_patched = nbc_info(ctx, fieldPath, patchedPath);
   
   if (is_patched && show_nbc_fields) {
       LLenv lle = Lowlevel::getLLenv(ctx);
       LOG_DEBUG << "---------------------------------------";
       LOG_DEBUG << "field path=" << getFullPath(lle.context) << "/" << fieldPath << " --> patched to: " << getFullPath(lle.context) << "/" << patchedPath;
       LOG_DEBUG << "---------------------------------------";
       LOG_DEBUG << "\n";
   }
   
   return is_patched;
}

bool NBC_plugin::nbc_info(int ctx, const char* fieldPath, std::string &patchedPath) {
    
    bool is_patched = false;
    std::vector<nbc_infos> nbc;
    
    nbc_key nbc_k;
    nbc_k.ctx = ctx;
    strcpy(nbc_k.field_path, fieldPath); 
    std::map<nbc_key, std::string> ::iterator it = visitedPaths.find(nbc_k);
    
    if (it != visitedPaths.end()) {
        patchedPath = it->second; 
    }
    else {
        getNBC_infos(fieldPath, nbc);
        //displayNBC_infos(nbc_k, nbc);
        for (size_t i = 0; i < nbc.size(); i++) {
            nbc_infos &nbc_token = nbc[i];
            
            bool requirements_for_patching_fullfilled = false;

            if (data_dictionary_version.find("-") != std::string::npos)
                 requirements_for_patching_fullfilled = false;
             else if (data_dictionary_version.empty()) 
                 requirements_for_patching_fullfilled = true;
             else {
                 if (strcmp(nbc_token.change_nbc_version, "") != 0)
                    requirements_for_patching_fullfilled = strcmp(data_dictionary_version.c_str(), nbc_token.change_nbc_version) < 0; //allow for patching only if data_dictionary_version < change_nbc_version
             }

             //LOG_DEBUG << "requirements_for_patching_fullfilled=" << requirements_for_patching_fullfilled;
            
            if (requirements_for_patching_fullfilled && strcmp(nbc_token.change_nbc_version, "") != 0) {
                is_patched = true;
                if (i == 0) {
                    patchedPath = std::string(nbc_token.change_nbc_previous_name);
                }
                else {
                   patchedPath += "/" + std::string(nbc_token.change_nbc_previous_name);
                } 
            }
            else {
                if (i == 0) {
                    patchedPath = std::string(nbc_token.change_nbc_current_name);
                }
                else {
                   patchedPath += "/" + std::string(nbc_token.change_nbc_current_name);
                } 
            }
        }
        //LOG_DEBUG << "inserting into map with key=(" << nbc_k.ctx << "," << nbc_k.field_path << ")";
        visitedPaths.insert(std::pair<nbc_key, std::string>(nbc_k, patchedPath));
    }
    return is_patched;
}

void NBC_plugin::displayNBC_infos(const nbc_key &nbc_k, const nbc_infos &nbc) {
    if (!nbc_debug)
        return;
    LLenv lle = Lowlevel::getLLenv(nbc_k.ctx);
    LOG_DEBUG << "read_data::full path=" <<  getFullPath(lle.context) + "/" + std::string(nbc_k.field_path);
    LOG_DEBUG << "nbc.change_nbc_version=" << nbc.change_nbc_version;
    LOG_DEBUG << "nbc.change_nbc_description=" << nbc.change_nbc_description;
    LOG_DEBUG << "nbc.change_nbc_previous_name=" << nbc.change_nbc_previous_name;
    LOG_DEBUG << "nbc.change_nbc_current_name=" << nbc.change_nbc_current_name;
}

void NBC_plugin::displayNBC_infos(const nbc_key &nbc_k, const std::vector<nbc_infos> &nbc_vector) {
    if (!nbc_debug)
        return;
    //LOG_DEBUG << "nbc_vector size=" <<  nbc_vector.size();
    for (int i = 0; i < nbc_vector.size(); i++) {
        displayNBC_infos(nbc_k, nbc_vector[i]);
    }
}

void NBC_plugin::getNBC_infos(const std::string &xpathKey, std::vector<nbc_infos> &nbc_vector)
{
   std::vector <std::string> tokens;
   getTokens(xpathKey, tokens);
   
   std::string key;
   char* value= NULL;
   for (int i = 0; i < tokens.size(); i++) {
       
       nbc_infos nbc;
       
       if (i == 0) {
           key = tokens[i];
       }
       else {
           key += "/" +tokens[i];
       }
          
       strcpy(nbc.change_nbc_version, "");
       strcpy(nbc.change_nbc_description, "");
       strcpy(nbc.change_nbc_previous_name, "");
       //LOG_DEBUG << "set nbc.change_nbc_current_name to " << xpathKey;
       strcpy(nbc.change_nbc_current_name, tokens[i].c_str());
       
       buildXPathExpression(key, "change_nbc_version");
       value= evaluateXpathExpression(xpath_request);
       if (value != NULL) {
           //LOG_DEBUG << "nbc.change_nbc_version found for expression: " << change_nbc_version_expr << "\n";
           strcpy(nbc.change_nbc_version, value);
           free(value);
       }
       else {
          //LOG_DEBUG << "nbc.change_nbc_version not found for expression: " << change_nbc_version_expr << "\n";
          nbc_vector.push_back(nbc);
          continue; //not necessary to try other fields, we continue
       }
       
       buildXPathExpression(key, "change_nbc_description");
       value= evaluateXpathExpression(xpath_request);
       if (value != NULL) {
           strcpy(nbc.change_nbc_description, value);
           free(value);
       }
       
       buildXPathExpression(key, "change_nbc_previous_name");
       value= evaluateXpathExpression(xpath_request);
       if (value != NULL) {
           strcpy(nbc.change_nbc_previous_name, value);
           free(value);
       }

       nbc_vector.push_back(nbc);    
       
   }

}

void NBC_plugin::close_xml_doc() {
     //LOG_DEBUG << "close_xml_doc =" << "\n";
    if (doc != NULL) {
       xmlFreeDoc(doc);
       doc = NULL;
       if (xpathCtx != NULL) {
            xmlXPathFreeContext(xpathCtx);
            xpathCtx = NULL;
        }
   }
}

void NBC_plugin::create_xml_doc(const char* ddFile) {
    assert(ddFile);
    close_xml_doc();
    //LOG_DEBUG << "creating xml doc for =" << ddFile << "\n";
    doc = xmlParseFile(ddFile); //Load and parse the XML DD
    if (doc == NULL) {
        char message[200];
        sprintf(message, "unable to parse Data Dictionnary %s. Check if the file exists.", ddFile);
        throw ALPluginException(message);
    }
    create_xpath_context(); 
}

void NBC_plugin::create_xpath_context() {
    //LOG_DEBUG << "creating create_xpath_context"  << "\n";
    //Create xpath evaluation context
    xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx == NULL) {
        char message[200];
        sprintf(message, "unexpected error: unable to create new XPath context");
        throw ALPluginException(message);
    }
}

void NBC_plugin::getTokens(const std::string &path, std::vector <std::string> &tokens) {
    std::stringstream check1(path);
    std::string intermediate;
    while(getline(check1, intermediate, '/')) {
        tokens.push_back(intermediate);
    }
}

void NBC_plugin::buildXPathExpression(const std::string &path, 
            const char* searchedAttribute) {

    std::vector <std::string> tokens;
    getTokens(path, tokens);
  
    std::string xpath_exp = "."; //current node (XPath)
    std::string field_str("/field[@name='%s']");
    
    strcpy(xpath_request, xpath_exp.c_str());
    
    for(int i = 0; i < tokens.size(); i++) {
        sprintf(temp_result, field_str.c_str(), tokens[i].c_str());
        strcat(xpath_request, temp_result);
    }
    if (searchedAttribute != NULL) {
        strcpy(temp_result, "");
        strcat(temp_result, "/@");
        strcat(temp_result, searchedAttribute);
        strcat(xpath_request, temp_result);
    }
    //LOG_DEBUG << "Xpath expression=" << xpath_request << "\n";
}

/**
 * Return the value of a XML node, NULL if the node does not exist 
 * */
char* NBC_plugin::evaluateXpathExpression(const char* xpathKey)
{
    assert(xpathKey);

    xmlChar* xPathExpr = xmlCharStrdup(xpathKey);
    //if (xpathCtx->node != NULL)
    //    LOG_DEBUG << "before xpath evalutation, xpathCtx->node name = " << xpathCtx->node->name;
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(xPathExpr, xpathCtx);
    if (xpathObj == NULL) {
        char message[200];
        sprintf(message, "unable to evaluate xpath expression %s", xpathKey);
        free(xPathExpr);
        throw ALPluginException(message);
    }

    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    char* value = NULL;

    xmlNodePtr cur;
    int err = 0;
    if (size != 0) {
        //LOG_DEBUG << "SIZE different of 0 for " << xPathExpr;
        cur = nodes->nodeTab[0];
        cur = cur->children;
        if (cur->content != NULL)
            value = strdup((char*)cur->content);
    } 
    xmlXPathFreeObject(xpathObj);
    free(xPathExpr);
    return value;
}

/**
 * Set the XML DOC root node to the IDS root and set the current node 
 * of the XPath context to this node 
 * */
void NBC_plugin::setRootNode() {
    //LOG_DEBUG << "setRootNode called...";
    char xpathKey[100];
    sprintf(xpathKey, "//IDS[@name='%s']", dataobjectname.c_str());
    setCurrentNodePtr(xpathKey);
    this->root = cur;
    xpathCtx->node = this->root;
}

/**
 * Set the XML current node to the client node 
 * */
void NBC_plugin::setCurrentNodePtr(const char* xpathKey)
{
    xmlChar* xPathExpr = xmlCharStrdup(xpathKey);
    
   /*if (xpathCtx->node != NULL)
        LOG_DEBUG << "before setCurrentNodePtr, xpathCtx->node name = " << xpathCtx->node->name;
    else {
        LOG_DEBUG << "before setCurrentNodePtr, xpathCtx-> node is NULL";
    }*/
    
    //Evaluate xpath expression
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(xPathExpr, xpathCtx);
    if (xpathObj == NULL) {
        char message[200];
        sprintf(message, "unable to evaluate xpath expression %s", xpathKey);
        free(xPathExpr);
        throw ALPluginException(message);
    }

    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;

    if (size != 0) {
        cur = nodes->nodeTab[0];
    } 
    else {
        char message[200];
        sprintf(message, "unable to set the current node %s", xpathKey);
        xmlXPathFreeObject(xpathObj);
        free(xPathExpr);
        throw ALPluginException(message);
    }
    xmlXPathFreeObject(xpathObj);
    free(xPathExpr);
}

std::string NBC_plugin::getFullPath(Context *context) {
    std::string path = "";
    if (context->getType() == CTX_ARRAYSTRUCT_TYPE) {
      OperationContext *opctx = (static_cast<ArraystructContext*> (context))->getOperationContext();
      path = (static_cast<ArraystructContext*> (context))->getPath();
      std::string path_prefix = opctx->getDataobjectName();
      size_t found_sep = path_prefix.find("/");
      if (found_sep == std::string::npos) 
         path_prefix += "/0";
      path = path_prefix + "/" + path;
  }
  else {
      OperationContext *opctx = static_cast<OperationContext*> (context);
      path = opctx->getDataobjectName();
      size_t found_sep = path.find("/");
      if (found_sep == std::string::npos) 
         path += "/0";
  }
}
