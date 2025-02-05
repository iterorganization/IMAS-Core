.. _support-for-data-entry-uris:

Support for Data entry URIs
===========================

Data entry URIs specify where and how IMAS data is stored (or should be stored
to). When you , you need to provide a data entry URI.

This page documents the URI structure and the options that are supported.


Data entry URI structure
------------------------

The general structure of an IMAS URI is the following, with optional elements
indicated with square brackets:

.. code-block:: text

    imas:[//host/]backend?query

Let's break down each of the components:

1. ``imas:`` this part indicates that this is an IMAS URI
2. ``host`` when the data is located at another machine, you use this
   section to indicate the address of that machine. See :ref:`UDA backend` for
   further details.
3. ``backend`` select the Access Layer backend. See :ref:`IMAS-Core supported backends <imas-core-supported-backends>` for the
   options.
4. ``query`` the query consists of ``key=value`` pairs, separated by a
   semicolon ``;``. See :ref:`Query keys <query-keys>` for further details.

.. 
   Commenting this out, as no backend currently supports URI fragments

   5. ``fragment`` In order to identify a subset from a given data-entry a
      ``fragment`` can be added to the URI. Such ``fragment``, which starts with a
      hash ``#``, is optional and allows to identify a specific IDS, or a part of
      an IDS. See :ref:`URI fragment` for further details.

.. _query-keys:

Query keys
----------

You can use query keys to indicate to the backend where the data is stored and
(optionally) set backend-specific configuration options. The following query
keys are currently recognized.

.. note::
    
    Query keys are case-sensitive and unknown query keys are silently ignored. 

``path`` [#mandatory]_
    Provide the path to the folder where the IMAS data is (or will be) stored.
    Paths can be absolute (starting with a ``/`` on UNIX, or with a drive letter
    on Windows) or relative to the current working directory.

    The backend manages how your IMAS data is stored within the folder.

    .. code-block:: text
        :caption: URI examples using path

        imas:hdf5?path=/absolute/path/to/data
        imas:hdf5?path=relative_path

``user``, ``database``, ``version``, ``pulse``, ``run`` [#mandatory]_
    Use `legacy` (Access Layer version 4 and earlier) way to indicate where the
    IMAS data is (or will be) stored.

    .. code-block:: text
        :caption: URI example using legacy data identifiers

        imas:mdsplus?user=public;pulse=131024;run=41;database=ITER;version=3

    In Access Layer version 5.0.0 and earlier use key ``shot`` instead of ``pulse``.

    .. code-block:: text
        :caption: URI example using legacy data identifiers in Access Layer 5.0.0 and earlier.

        imas:mdsplus?user=public;shot=131024;run=41;database=ITER;version=3



.. [#mandatory] Either ``path`` or `all` of the legacy query keys must be
    provided.

.. _query_keys_specific_for_the_hdf5_backend:

Query keys specific for the HDF5 backend
''''''''''''''''''''''''''''''''''''''''

The :ref:`HDF5 backend` also recognizes these backend-specific query keys.

``hdf5_compression``
    Data compression is enabled by default. Set ``hdf5_compression=no`` or
    ``hdf5_compression=n`` to disable data compression.

``hdf5_write_buffering``
    During a `put` operation, 0D and 1D buffers are first
    stored in memory. Buffers are flushed at the end of the put.
    
    This feature is enabled by default. Set ``hdf5_write_buffering=no`` or
    ``hdf5_write_buffering=n`` to disable write buffering.

``write_cache_option``
    Set the size of the HDF5 chunk cache used during chunked datasets write
    operations. Default to 100x1024x1024 bytes (100 MiB).

``read_cache_option``
    Set the size of the HDF5 chunk cache used during chunked datasets read
    operations. Default to 5x1024x1024 bytes (5 MiB).

``open_read_only``
    Open master file and IDSs files in read only if ``open_read_only=yes`` or
    ``open_read_only=y``, overwriting the files access modes default behavior (see IMAS-5274 for an example use-case).

``hdf5_debug``
    HDF5 debug output is disabled by default. Set ``hdf5_debug=yes`` or
    ``hdf5_debug=y`` to enable HDF5 debug output.


Query keys specific for the ASCII backend
'''''''''''''''''''''''''''''''''''''''''

.. _ascii-backend:

The :ref:`ASCII backend <ascii-backend>` also recognizes these backend-specific query keys.

``filename``
    Specify the exact filename in which the IDS data will be stored, instead
    of the default `<idsname><occurrence>.ids`.


Query keys specific for the UDA backend
'''''''''''''''''''''''''''''''''''''''

The :ref:`UDA backend` also recognizes these backend-specific query keys.

``verbose``
    UDA verbosity is disabled by default. Set ``verbose=1`` to obtain
    more information and ease debugging.
