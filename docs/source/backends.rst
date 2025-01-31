IMAS-Core supported backends
============================


Several backends exist for storing and loading IMAS data. Each backend uses a
different format for storing the data.

.. note::

    Depending on local install choices, some backends may be unavailable in
    your Access Layer installation.


Backend usage
'''''''''''''

:ref:`UDA backend` The UDA backend is read-only.
:ref:`ASCII backend` Suitable for tests and small data, but not recommended for large
datasets or long-term storage.
:ref:`Memory backend` Suitable for transferring data between languages in the same program.
:ref:`HDF5 backend` Suitable for large datasets and long-term storage.
:ref:`MDSplus backend` Suitable for storing data in the MDSplus format.

HDF5 backend
''''''''''''

The HDF5 backend is identified by ``hdf5`` in the IMAS URI, and stores data in
the `hdf5 data format <https://en.wikipedia.org/wiki/Hierarchical_Data_Format>`_
`hdf5 implementation <https://github.com/iterorganization/IMAS-Core/tree/main/src/hdf5>`_.



MDSplus backend
'''''''''''''''

The MDSplus backend is identified by ``mdsplus`` in the IMAS URI. The data is
stored in the `MDSplus format <https://www.mdsplus.org/>`_.

This backend imposes some limitations on the data that can be stored, see
`maxoccur` in the |DD| documentation.

This backend has been around and stable for a longer time, so most older IMAS
data is stored in this format.
`mdsplus implementation <https://github.com/iterorganization/IMAS-Core/tree/main/src/mdsplus>`_.

UDA backend
'''''''''''

The UDA backend is used when a host is provided. `UDA (Universal Data Access)
<https://github.com/ukaea/UDA>`_ is the mechanism for contacting the server that
stores the data.

A number of UDA plugins already exist for these, but their availability depends
on how UDA has been installed on the local cluster. Therefore it's recommended
that you contact the IMAS support team when you want to use this functionality.
`UDA implementation <https://github.com/iterorganization/IMAS-Core/tree/main/src/uda>`_.


Memory backend
''''''''''''''

The memory backend is identified by ``memory`` in the IMAS URI. When storing or
loading IMAS data with this backend, the data is stored in-memory. This is
therefore not persistent.

The memory backend can still be useful to transfer data between languages in the
same program (for example, storing an IDS in C++ and then loading it with the
Fortran HLI) or to store a number of time slices and then loading all time slices.
`Memory backend implementation <hhttps://git.iter.org/projects/IMAS/repos/al-core/browse/src/memory_backend.cpp>`_.


ASCII backend
'''''''''''''

The ASCII backend is identified by ``ascii`` in the IMAS URI. The ASCII backend
can be used to store IDS data in a plain-text human readable format. The
performance and size of the stored data is worse than the other backends, so
this is typically only used for debugging.
`ASCII backend implementation <https://git.iter.org/projects/IMAS/repos/al-core/browse/src/ascii_backend.cpp>`_.


Flexbuffers backend
'''''''''''''''''''

The Flexbuffers backend is identified by ``flexbuffers`` in the IMAS URI. The
Flexbuffers backend is used when (de)serializing IDSs with the
``FLEXBUFFERS_SERIALIZER_PROTOCOL``. It is optimized for (de)serialization speed and
therefore has very limited functionality. It is not intended to be used outside of IDS
serialization.
`Flexbuffers backend implementation <https://github.com/iterorganization/IMAS-Core/tree/main/src/flatbuffers>`_.


