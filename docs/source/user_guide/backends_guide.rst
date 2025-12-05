.. _Backends:

Backends
========




Data entry URIs
===============

Data entry URIs specify where and how IMAS data is stored (or should be stored
to). When you load or store IMAS data, you
need to provide a data entry URI.

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
3. ``backend`` select the Access Layer backend. See the :ref:`Backends` section below for the
   options.
4. ``query`` the query consists of ``key=value`` pairs, separated by a
   semicolon ``;``. See :ref:`Query keys` for further details.

.. 
   Commenting this out, as no backend currently supports URI fragments

   5. ``fragment`` In order to identify a subset from a given data-entry a
      ``fragment`` can be added to the URI. Such ``fragment``, which starts with a
      hash ``#``, is optional and allows to identify a specific IDS, or a part of
      an IDS. See :ref:`URI fragment` for further details.


Backends
--------

Several backends exist for storing and loading IMAS data. Each backend uses a
different format for storing the data.

.. note::

    Depending on local install choices, some backends may be unavailable in
    your Access Layer installation.


Backend comparison
''''''''''''''''''

.. csv-table:: Comparison of backend functionality
    :header-rows: 1
    :stub-columns: 1

    , :ref:`HDF5 <hdf5 backend>`, :ref:`MDSplus <mdsplus backend>`, :ref:`UDA <uda backend>`, :ref:`Memory <memory backend>`, :ref:`ASCII <ascii backend>`, :ref:`Flexbuffers <flexbuffers backend>`
    ``get``, Yes, Yes, Yes, Yes, Yes, Yes [#fb_get]_
    ``get_slice``, Yes, Yes, Yes, Yes, \-, \-
    ``put``, Yes, Yes, \-, Yes, Yes, Yes [#fb_put]_
    ``put_slice``, Yes, Yes, \-, Yes, \-, \-
    Persistent storage, Yes, Yes, Yes [#uda]_, \-, Yes [#ascii]_, \-

.. [#uda] The UDA backend is read-only.
.. [#ascii] Suitable for tests and small data, but not recommended for large
    datasets or long-term storage.
.. [#fb_get] Only when using ``OPEN_PULSE`` mode
.. [#fb_put] Only when not using ``OPEN_PULSE`` mode


.. _hdf5 backend:

HDF5 backend
''''''''''''

The HDF5 backend is identified by ``hdf5`` in the IMAS URI, and stores data in
the `hdf5 data format <https://en.wikipedia.org/wiki/Hierarchical_Data_Format>`_


.. _mdsplus backend:

MDSplus backend
'''''''''''''''

The MDSplus backend is identified by ``mdsplus`` in the IMAS URI. The data is
stored in the `MDSplus format <https://www.mdsplus.org/>`_.

This backend imposes some limitations on the data that can be stored, see
`maxoccur` in the |DD| documentation.

    This backend has been around and stable for a longer time, so most older IMAS
    data is stored in this format.


.. _uda backend:

UDA backend
'''''''''''

The UDA backend is used when a host is provided. `UDA (Universal Data Access)
<https://github.com/ukaea/UDA>`_ is the mechanism for contacting the server that
stores the data.

A number of UDA plugins already exist for these, but their availability depends
on how UDA has been installed on the local cluster. Therefore it's recommended
that you contact the IMAS support team when you want to use this functionality.

simple URI example: imas://uda.iter.org:56565/uda?path=/work/imas/shared/imasdb/ITER/3/131062/4;backend=hdf5;verbose=1


.. _memory backend:

Memory backend
''''''''''''''

The memory backend is identified by ``memory`` in the IMAS URI. When storing or
loading IMAS data with this backend, the data is stored in-memory. This is
therefore not persistent.

The memory backend can still be useful to transfer data between languages in the
same program (for example, storing an IDS in C++ and then loading it with the
Fortranens-user API) or to store a number of time slices and then loading all time slices.

simple URI example: imas:memory?path=/path/to/data

.. _ascii backend:

ASCII backend
'''''''''''''

The ASCII backend is identified by ``ascii`` in the IMAS URI. The ASCII backend
can be used to store IDS data in a plain-text human readable format. The
performance and size of the stored data is worse than the other backends, so
this is typically only used for debugging.


.. _flexbuffers backend:

Flexbuffers backend
'''''''''''''''''''

The Flexbuffers backend is identified by ``flexbuffers`` in the IMAS URI. The
Flexbuffers backend is used when (de)serializing IDSs with the
``FLEXBUFFERS_SERIALIZER_PROTOCOL``. It is optimized for (de)serialization speed and
therefore has very limited functionality. It is not intended to be used outside of IDS
serialization.


.. _Query keys:

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



.. [#mandatory] Either ``path`` or all of the legacy query keys must be
    provided.


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

The :ref:`ASCII backend` also recognizes these backend-specific query keys.

``filename``
    Specify the exact filename in which the IDS data will be stored, instead
    of the default `<idsname><occurrence>.ids`.


Query keys specific for the UDA backend
'''''''''''''''''''''''''''''''''''''''

The :ref:`UDA backend` also recognizes these backend-specific query keys.

``verbose``
    UDA verbosity is disabled by default. Set ``verbose=1`` to obtain
    more information and ease debugging.

``cache_mode``
    UDA cache_mode is ``ids`` by default. Set ``cache_mode=none`` or ``cache_mode=ids`` to specify the mode of caching.
    - ``none``: No caching is performed.
    - ``ids``: Caches the entire IDS (Interface Data Structure).

``fetch``
    UDA ``fetch`` is disabled by default. Set ``fetch=1`` to enable fetching
    and downloading IDS files to the local ``local_cache`` directory.

``local_cache``
    UDA ``local_cache`` is set to ``tmp/path_in_uri`` by default. This is used along with ``fetch=1`` in the query.
    Set ``local_cache=/path/to/local/cache/directory`` and the download directory will be ``local_cache/path_in_uri``.
    ``local_cache`` specifies the path to the local cache directory where IDSs will be downloaded.
