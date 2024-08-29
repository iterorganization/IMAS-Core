IMAS overview
=============

IMAS is the Integrated Modeling and Analysis Suite of ITER. It consists of
numerous infrastructure components, physics components and tools. An up-to-date
overview of these can be found at `<https://imas.iter.org/>`_ (ITER
Organization account required).

The IMAS core consists of:

1. Standardized data structures for storing experimental and simulation data.
2. Infrastructure for storing and loading these data structures.

The standardized data structures are defined in the |DD|.
The documentation for the Data Dictionary (DD) can be found there as well, for
example:

- Which data structures (IDSs) exist
- What data is contained in these structures
- What units a data field has
- What are the coordinates belonging to a data field

.. todo::

    Add links to the HLI documentation pages.

The Access Layer, of which you are currently reading the documentation,
provides the libraries for working with these data structures, for example:

- Loading an IDS from disk
- Storing an IDS to disk
- Using and manipulating IDSs in your program
