cdef extern from "al_lowlevel.h":

    ctypedef struct al_status_t:
        int code
        char message[10]

    al_status_t al_context_info(int ctx, char ** info)

    al_status_t al_get_backendID(int ctx, int * beid)

    al_status_t al_build_uri_from_legacy_parameters(const int backendID, const int pulse, const int run,  const char * user, const char * tokamak, const char * version, const char * options, char ** uri)

    al_status_t al_begin_dataentry_action(const char * uri, int mode, int* deCtx)

    al_status_t al_close_pulse(int pulseCtx, int mode)

    al_status_t al_begin_global_action(int ctx, const char * dataobjectname, const char * datapath, int rwmode, int * opctx)

    al_status_t al_begin_slice_action(int ctx, const char * dataobjectname, int rwmode, double time, int interpmode, int * opctx)

    al_status_t al_end_action(int ctx)

    al_status_t al_write_data(int ctx, const char * fieldpath, const char * timebasepath, void * data, int datatype, int dim, int * size)
    
    al_status_t al_read_data(int ctx, const char * fieldpath, const char * timebasepath, void ** data, int datatype, int dim, int * size)

    al_status_t al_delete_data(int ctx, const char * path)

    al_status_t al_begin_arraystruct_action(int ctx, const char * path, const char * timebase, int * size, int * aosctx)

    al_status_t al_iterate_over_arraystruct(int aosctx, int step)

    al_status_t al_register_plugin(const char * name)
    
    al_status_t al_unregister_plugin(const char * name)

    al_status_t al_bind_plugin(const char * path, const char * name)

    al_status_t al_unbind_plugin(const char * path, const char * name)
    
    al_status_t al_bind_readback_plugins(int ctx)

    al_status_t al_unbind_readback_plugins(int ctx)
    
    al_status_t al_write_plugins_metadata(int ctx)
    
    al_status_t al_setvalue_parameter_plugin(const char* parameter_name, int datatype, int dim, int *size, void *data, const char* pluginName)
    
    al_status_t al_setvalue_int_scalar_parameter_plugin(const char* parameter_name, int parameter_value, const char* pluginName)
    
    al_status_t al_setvalue_double_scalar_parameter_plugin(const char* parameter_name, double parameter_value, const char* pluginName)

    al_status_t al_get_occurrences(int ctx, const char* ids_name, int **occurrences_list, int *size)

    const char* getALVersion()
