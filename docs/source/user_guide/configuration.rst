.. _configuration options:

Configuration
=============

Some functionality of the Access Layer can be configured through enironment
variables. This page provides an overview of the available options.


Environment variables controlling IMAS-Core behaviour
-----------------------------------------------------

``IMAS_AL_DISABLE_OBSOLESCENT_WARNING``
    Since version 4.10.0, all interfaces print warnings when putting an IDS that
    contains data in fields marked as `obsolescent` in the DD. Setting this
    variable to ``1`` disables these printouts.


``IMAS_LOCAL_HOSTS``
    If you have a UDA server on a site where users have direct access to the IMAS data files,
    the IMAS-Core backend can decide to use a direct file access (via the relevant backend, e.g. HDF5)
    instead of going via the UDA server (which has performance overheads). If you want to use this
    feature, you need to set a list of UDA server hostname (cf. ``UDA_HOST`` [#uda_uri]_ ) in the
    ``IMAS_LOCAL_HOSTS`` environment variable. You can specify several servers separated with a
    semi-colon ``;``.

    
Environment variables controlling access layer plugins
------------------------------------------------------

``IMAS_AL_ENABLE_PLUGINS``
    Execution of C++ plugins in AL5 is a new feature which can be tested by
    users who are interested in. It's currently an experimental feature which is
    disabled by default.

    When the plugins framework is disabled:

    - Low level plugins registering/search functions are disabled.
    - The behavior of writing data for nodes with default values is the same
      that AL4.ens-user API write requests for these `empty` nodes are not sent to the
      LL.  

    When the plugins framework is enabled:

    - Low level plugins registering/search functions are enabled.
    - The behavior of writing data for nodes with default values differs from
      AL4.ens-user API write requests for these `empty` nodes are sent to the LL
      allowing eventually to execute low level C++ plugins bound to these nodes
      whose content can be handled by these plugins.    
    
    To enable the plugins framework, set the global environment variable
    ``IMAS_AL_ENABLE_PLUGINS`` before executing the access layer:

    .. code-block:: bash

        export IMAS_AL_ENABLE_PLUGINS=TRUE


Backend specific environment variables
--------------------------------------


``HDF5_BACKEND_READ_CACHE`` [#uri_precedence]_
    Specify the size of the read cache in MB (default is 5). It may improve
    reading performance at the cost of increased memory consumption. Obtained
    performance and best size of cache is heavily depending on the data.


``HDF5_BACKEND_WRITE_CACHE`` [#uri_precedence]_
    Specify the size of the write cache in MB (default is 100). It may improve
    writing performance at the cost of increased memory consumption. Obtained
    performance and best size of cache is heavily depending on the data.


..  [#uri_precedence] These settings can also be configured in the IMAS URI, see
    :doc:`../user_guide/backends_guide`. The URI provided settings
    will be used if both are present.


``IMAS_AL_SERIALIZER_TMP_DIR``
    Specify the path to storing temporary data. If it is not set, the default
    location `/dev/shm/` or the current working directory will be chosen.

``MDSPLUS_MODELS_PATH``
    Specify the path where the MDSplus models files are stored for a given
    version of the Data Dictionary (previously set by the `ids_path`, which
    is now internally handled by the backend).


UDA client configuration to reach the server at ITER
----------------------------------------------------

``UDA_HOST=uda.iter.org`` [#uda_uri]_
    If set, all queries with the UDA backend will be directed at the ITER UDA
    server `uda.iter.org`, unless directly specified in the Access Layer's URI.

``UDA_PORT=56565`` [#uda_uri]_
    If set, all queries will be directed to the port `56565` of the selected UDA
    server, unless directly specified in the Access Layer's URI.


    The ITER UDA server uses SSL authentication through a Personal Key Infrastructure
    (PKI). You can download your PKI certificate at `pkiuda.iter.org`. Extract the
    obtained `bundle.zip` in a folder in which only you have read permission (e.g.
    `$HOME/.uda`). Then set the following environment variables:

    .. code-block:: bash

	export UDA_CLIENT_SSL_KEY=$HOME/.uda/private.key
	export UDA_CLIENT_CA_SSL_CERT=$HOME/.uda/ca-server-certificate.pem
	export UDA_CLIENT_SSL_CERT=$HOME/.uda/certificate.pem
	export UDA_CLIENT_SSL_AUTHENTICATE=1

    Do the same on all the systems from which you want to access ITER's UDA server.


..  [#uda_uri] An Access Layer URI of the form
    `imas:uda?path=<path_on_server>;backend=<backend_on_server>` will only work if
    `UDA_HOST` and `UDA_PORT` environment variables are set. If not, the information
    needs to be directly passed in the URI, as
    `imas://<UDA_HOST>:<UDA_PORT>/uda?path=<path_on_server>;backend=<backend_on_server>`.
