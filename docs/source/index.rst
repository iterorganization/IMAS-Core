IMAS-Core Documentation
=======================

.. image:: https://github.com/iterorganization/IMAS-Core/actions/workflows/wheels.yml/badge.svg?branch=develop
    :target: https://github.com/iterorganization/IMAS-Core/actions/workflows/wheels.yml
    :alt: Build Status

.. image:: https://img.shields.io/github/license/iterorganization/IMAS-Core
    :target: https://github.com/iterorganization/IMAS-Core/blob/main/LICENSE.txt
    :alt: License


What is IMAS?
-------------

**IMAS** (Integrated Modeling and Analysis Suite) is ITER's comprehensive suite for storing and analyzing fusion experiment data.

IMAS consists of:

1. **Standardized Data Structures** - Defined in the `IMAS Data Dictionary <https://imas-data-dictionary.readthedocs.io/en/latest/>`_, these structures store experimental and simulation data
2. **Infrastructure Components** - Tools and libraries for reading, writing, and accessing data
3. **Physics Components** - Domain-specific tools and models


What is IMAS-Core?
------------------

IMAS-Core is the **lowlevel infrastructure library** for reading and writing IMAS data structures. It consists of:

- **Core Library** - C++ access layer with Python bindings
- **Multiple Backends** - Support for HDF5, MDSplus, UDA, and other storage formats
- **MDSplus Models** - Data model definitions for MDSplus backend
- **Python Bindings** - Easy-to-use Python interface via Cython

Key capabilities:

- Provide support for loading and store IMAS data structures (IDSs) to/from disk
- Work with multiple data backends transparently
- Access data from any supported programming language with bindings
- Cross-platform support (Linux, macOS, Windows)



Overview
--------

IMAS-Core provides:

- **High Performance** - Efficient C++ backend for fast data access
- **Multiple Backends** - HDF5, MDSplus, UDA, FlexBuffers, and in-memory storage
- **Language Support** - Python, C++, Fortran, Java, MATLAB bindings in end-user languages
- **Standardized Format** - IMAS data structure ensures interoperability
- **Easy Installation** - pip install for Python users

The library is built using modern C++ and provides a unified interface across all supported backends.

.. grid:: 1 1 2 2
    :gutter: 3

    .. grid-item-card:: 👤 **For End Users**
        :text-align: center

        Install and use IMAS-Core as a Python package

        +++

        .. button-ref:: user_guide/index
            :ref-type: doc
            :color: primary
            :outline:

            User Guide

    .. grid-item-card:: 🔧 **For Developers**
        :text-align: center

        Build from source, contribute, and extend IMAS-Core

        +++

        .. button-ref:: developers/index
            :ref-type: doc
            :color: primary
            :outline:

            Developer Guide


Getting Started
---------------


**Quick Start with pip:**

.. code-block:: bash

    pip install imas-core
    python -c "import imas_core"


Documentation Structure
-----------------------

.. toctree::
    :maxdepth: 2

    user_guide/index
    developers/index
    troubleshooting
    faq


Important Links
---------------

- **Homepage**: Coming soon
- **GitHub Repository**: https://github.com/iterorganization/IMAS-Core
- **IMAS Data Dictionary**: https://imas-data-dictionary.readthedocs.io/
- **Issue Tracker**: https://github.com/iterorganization/IMAS-Core/issues


.. note::
    This documentation covers IMAS-Core as an independent, installable library.
    For information on other IMAS components (like language bindings for C++, Fortran),
    refer to their respective repositories.
