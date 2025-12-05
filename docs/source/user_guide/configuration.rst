.. _configuration options:

Configuration
=============

Some functionality of the Access Layer can be configured through enironment
variables. This page provides an overview of the available options.


Environment variables controlling IMAS-Core behaviour
-----------------------------------------------------

``IMAS_AL_DEFAULT_BACKEND`` [#backend_env]_
    Specify which backend to use by default with open/create methods that do not
    pass this information as an argument. Values for this environment
    variable correspond to the targeted backend ID, see below table. If not
    specified, the MDS+ backend is the default.

    .. csv-table:: Backend IDs
        :header-rows: 1

        Backend, Backend ID
        :ref:`ASCII <ascii backend>`, 11
        :ref:`MDSplus <mdsplus backend>`, 12
        :ref:`HDF5 <hdf5 backend>`, 13
        :ref:`Memory <memory backend>`, 14
        :ref:`UDA <uda backend>`, 15
    

``IMAS_AL_FALLBACK_BACKEND`` [#backend_env]_
    Specify a fallback backend to be tried if opening the given data-entry was
    not successful with the primary/default backend. Values for this environment
    variable correspond to the targeted backend ID, see above table. If not
    specified, no secondary attempt will be made. This does not have any effect
    on calls to create new dataentries.


``IMAS_AL_DISABLE_OBSOLESCENT_WARNING``
    Since version 4.10.0, all interfaces print warnings when putting an IDS that
    contains data in fields marked as `obsolescent` in the DD. Setting this
    variable to ``1`` disables these printouts.


.. [#backend_env] These environment variables are not applicable when using
    Data entry URIs (see :doc:`../user_guide/uris_guide`), which explicitly specify the backend. Also not applicable
    in the Python HLI.


Environment variables controlling access layer plugins
------------------------------------------------------

``IMAS_AL_ENABLE_PLUGINS``
    Execution of C++ plugins in AL5 is a new feature which can be tested by
    users who are interested in. It's currently an experimental feature which is
    disabled by default.

    When the plugins framework is disabled:

    - Low level plugins registering/search functions are disabled.
    - The behavior of writing data for nodes with default values is the same
      that AL4. HLI write requests for these `empty` nodes are not sent to the
      LL.  

    When the plugins framework is enabled:

    - Low level plugins registering/search functions are enabled.
    - The behavior of writing data for nodes with default values differs from
      AL4. HLI write requests for these `empty` nodes are sent to the LL
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
    Specify the size of the write cache in MB (default is 5). It may improve
    writing performance at the cost of increased memory consumption. Obtained
    performance and best size of cache is heavily depending on the data.


..  [#uri_precedence] These settings can also be configured in the IMAS URI, see
    :doc:`../user_guide/backends_guide`. The URI provided settings
    will be used if both are present.


``IMAS_AL_SERIALIZER_TMP_DIR``
    Specify the path to storing temporary data. If it is not set, the default
    location `/dev/shm/` or the current working directory will be chosen.



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
