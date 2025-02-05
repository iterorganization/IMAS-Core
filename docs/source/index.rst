.. IMAS-Core documentation master file, created by
   sphinx-quickstart on Fri Jan 24 08:52:26 2025.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

IMAS-Core documentation
=======================

IMAS-Core is low level access library for reading writing IMAS datasets. It supports mdsplus, hdf5, memory backends
and provides a common interface to access IMAS data.
It includes:
- C lowlevel interface used by the various High Level Interfaces like Java Fortran , Matlab etc.
- Python bindings to the lowlevel interface
- Backends for reading and writing IMAS data
- Logic for creating the models required by the MDS+ backend

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   introduction
   installation
   configuration
   development
   usage
   troubleshooting

.. toctree::
   :maxdepth: 2
   :caption: Plugins in IMAS-Core:

   plugins

.. toctree:: 
   :maxdepth: 2
   :caption: API documentation

   imas_core/html/index

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
