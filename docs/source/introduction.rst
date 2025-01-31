Overview
==================

IMAS-Core is low level library is the part of Integrated Modeling and Analysis Suite of ITER. 
It supports different backends like MDSPLUS, HDF5, ASCII, Memory, Flexbuffers and UDA.
IMAS Core low level library provides C++ APIs to be used by other high level languages like 
Java, Python, Fortran, Matlab etc.

IMAS-Core is part of the Integrated Modelling and Analysis Suite at ITER. It is low level library. It supports different backends like MDSPLUS, HDF5, ASCII, 
Memory, Flexbuffers and UDA.It provides a low-level library to read and write MDSplus ([Introduction](https://www.mdsplus.org/index.php/Introduction)) 
and HDF5 ([HDF5](https://www.hdfgroup.org/solutions/hdf5/)) formats. 
It also supports reading and writing [FlexBuffers](https://flatbuffers.dev/flexbuffers/). For debugging purposes, it allows writing ASCII files. 
Notably, for keeping data in-memory when exchanging data between physics codes, it supports writing into memory.

The IMAS-Core library includes Python bindings, making it useful for working with Python. It can be installed on different operating systems, 
including macOS and Windows. IMAS-Core is a versatile library that helps users write fusion data in a standard format used by various users 
and facilitates easy data exchange.

Most of the code is written in C++, with wrappers on existing libraries for HDF5, MDSplus, and FlexBuffers. These APIs are used by other 
high-level languages such as Java, Python, Fortran, and Matlab.


.. toctree::
   :maxdepth: 2
   :caption: Contents:

   backends
   uri