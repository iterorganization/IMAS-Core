Backends
========

.. _backends-guide:

Several backends exist for storing and loading IMAS data. Each backend uses a
different format for storing the data.

.. note::

    Depending on local install choices, some backends may be unavailable in
    your Access Layer installation.


Backend comparison
------------------

.. _load-entire-ids:
.. _store-entire-ids:
.. _load-single-time-slice:
.. _append-time-slice:

.. csv-table:: Comparison of backend functionality
    :header-rows: 1
    :stub-columns: 1

    , :ref:`HDF5 <hdf5-backend>`, :ref:`MDSplus <mdsplus-backend>`, :ref:`UDA <uda-backend>`, :ref:`Memory <memory-backend>`, :ref:`ASCII <ascii-backend>`, :ref:`Flexbuffers <flexbuffers-backend>`
    :ref:`get <load-entire-ids>`, Yes, Yes, Yes, Yes, Yes, Yes [#fb_get]_
    :ref:`get_slice <load-single-time-slice>`, Yes, Yes, Yes, Yes, \-, \-
    :ref:`put <store-entire-ids>`, Yes, Yes, \-, Yes, Yes, Yes [#fb_put]_
    :ref:`put_slice <append-time-slice>`, Yes, Yes, \-, Yes, \-, \-
    Persistent storage, Yes, Yes, Yes [#uda]_, \-, Yes [#ascii]_, \-

.. [#uda] The UDA backend is read-only.
.. [#ascii] Suitable for tests and small data, but not recommended for large
    datasets or long-term storage.
.. [#fb_get] Only when using ``OPEN_PULSE`` mode
.. [#fb_put] Only when not using ``OPEN_PULSE`` mode


HDF5 backend
~~~~~~~~~~~~

.. _hdf5-backend:

The HDF5 backend is identified by ``hdf5`` in the IMAS URI, and stores data in
the `hdf5 data format <https://en.wikipedia.org/wiki/Hierarchical_Data_Format>`_

HDF5 is the recommended backend for most users. It's efficient, widely supported, and platform-independent.

**Opening HDF5 Database:**

.. code-block:: python

    import imas
    
    uri = "imas:hdf5?path=/path/to/database"
    data_entry = imas.DBEntry(uri, "r")
    data_entry.open()

**Advantages:**
    - Self-contained file (all data in one file)
    - Efficient for large datasets
    - Supports compression
    - Cross-platform

**Disadvantages:**
    - Requires HDF5 libraries installed
    - Not ideal for remote access


MDSplus backend
~~~~~~~~~~~~~~~

.. _mdsplus-backend:

The MDSplus backend is identified by ``mdsplus`` in the IMAS URI. The data is
stored in the `MDSplus format <https://www.mdsplus.org/>`_.

This backend imposes some limitations on the data that can be stored, see
`maxoccur` in the |DD| documentation.

This backend has been around and stable for a longer time, so most older IMAS
data is stored in this format.

MDSplus is used for time-series data and is common in fusion experiments.

**Opening MDSplus Database:**

.. code-block:: python

    uri = "imas:mdsplus?path=/path/to/database"
    data_entry = imas.DBEntry(uri, "r")
    data_entry.open()

**Advantages:**
    - Native support for time series
    - Remote access capability
    - Used on ITER
    - Flexible tree structure

**Disadvantages:**
    - Requires MDSplus server
    - More complex setup


UDA backend
~~~~~~~~~~~

.. _uda-backend:

The UDA backend is used when a host is provided. `UDA (Universal Data Access)
<https://github.com/ukaea/UDA>`_ is the mechanism for contacting the server that
stores the data.

A number of UDA plugins already exist for these, but their availability depends
on how UDA has been installed on the local cluster. Therefore it's recommended
that you contact the IMAS support team when you want to use this functionality.

UDA (Unified Data Access) is designed for distributed data access.

**Opening UDA Database:**

.. code-block:: python

    uri = "imas:uda?path=imas://uda.iter.org/uda?path=/work/imas/shared/imasdb/ITER/3/134110/34&backend=hdf5"
    data_entry = imas.DBEntry(uri, "r")
    data_entry.open()

**Advantages:**
    - Remote data access
    - Efficient for distributed systems
    - Reduces local storage needs

**Disadvantages:**
    - Requires UDA infrastructure
    - Network dependency

.. note::

    Provide a sample URI string


Memory backend
~~~~~~~~~~~~~~

.. _memory-backend:

The memory backend is identified by ``memory`` in the IMAS URI. When storing or
loading IMAS data with this backend, the data is stored in-memory. This is
therefore not persistent.

The memory backend can still be useful to transfer data between languages in the
same program (for example, storing an IDS in C++ and then loading it with the
Fortran HLI) or to :ref:`store a number of time slices <append-time-slice>` and then :ref:`loading all time slices <load-entire-ids>`.

Store data in-memory. Useful for testing and temporary data handling.

**Using Memory Backend:**

.. code-block:: python

    uri = "imas:memory"
    data_entry = imas.DBEntry(uri, "w")
    data_entry.open()
    
    # Create and write data
    factory = imas.IDSFactory()
    equilibrium = factory.equilibrium()
    data_entry.put('equilibrium', equilibrium, occurrence=0)
    
    # Read it back
    data = data_entry.get('equilibrium', occurrence=0)

**Advantages:**
    - Fast in-memory access
    - No file I/O overhead
    - Useful for testing

**Disadvantages:**
    - Data lost when process ends
    - Limited by available RAM


ASCII backend
~~~~~~~~~~~~~

.. _ascii-backend:

The ASCII backend is identified by ``ascii`` in the IMAS URI. The ASCII backend
can be used to store IDS data in a plain-text human readable format. The
performance and size of the stored data is worse than the other backends, so
this is typically only used for debugging.

Human-readable text format, primarily for debugging.

**Using ASCII Backend:**

.. code-block:: python

    uri = "imas:ascii?path=/path/to/ascii/files"
    data_entry = imas.DBEntry(uri, "w")
    data_entry.open()

**Advantages:**
    - Human-readable
    - Good for debugging
    - Easy to inspect

**Disadvantages:**
    - Large file size
    - Slow read/write
    - Not for production use


Flexbuffers backend
~~~~~~~~~~~~~~~~~~~

.. _flexbuffers-backend:

The Flexbuffers backend is identified by ``flexbuffers`` in the IMAS URI. The
Flexbuffers backend is used when (de)serializing IDSs with the
``FLEXBUFFERS_SERIALIZER_PROTOCOL``. It is optimized for (de)serialization speed and
therefore has very limited functionality. It is not intended to be used outside of IDS
serialization.


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


Query keys specific for the HDF5 backend
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _hdf5-backend-query-keys:

The :ref:`HDF5 backend <hdf5-backend>` also recognizes these backend-specific query keys.

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
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The :ref:`ASCII backend <ascii-backend>` also recognizes these backend-specific query keys.

``filename``
    Specify the exact filename in which the IDS data will be stored, instead
    of the default `<idsname><occurrence>.ids`.


Query keys specific for the UDA backend
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The :ref:`UDA backend <uda-backend>` also recognizes these backend-specific query keys.

``verbose``
    UDA verbosity is disabled by default. Set ``verbose=1`` to obtain
    more information and ease debugging.

``cache_mode``
    UDA cache_mode is ``ids`` by default. Set ``cache_mode=none``or ``cache_mode=ids`` to specify the mode of caching.
    - ``none``: No caching is performed.
    - ``ids``: Caches the entire IDS (Interface Data Structure).

``fetch``
    UDA ``fetch`` is disabled by default. Set ``fetch=1`` to enable fetching
    and downloading IDS files to the local ``local_cache`` directory.

``local_cache``
    UDA ``local_cache`` is set to ``tmp/path_in_uri`` by default. This is used along with ``fetch=1`` in the query.
    Set ``local_cache=/path/to/local/cache/directory`` and the download directory will be ``local_cache/path_in_uri``.
    ``local_cache`` specifies the path to the local cache directory where IDSs will be downloaded.
