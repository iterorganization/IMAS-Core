Installation
=============

Quick Install
-------------

The easiest way to install IMAS-Core is via pip:

.. code-block:: bash

    pip install imas-core imas-python

That's it! No additional setup required.

Verify Installation
~~~~~~~~~~~~~~~~~~~

After installation, verify it works by trying to open hdf5 data entry:

.. code-block:: python

    import imas

    # Open an IMAS database
    data_entry = imas.DBEntry("imas:hdf5?path=/path/to/database", "r")
    data_entry.open()

    # Get data
    equilibrium = data_entry.get('equilibrium', occurrence=0)

    data_entry.close()

Requirements
~~~~~~~~~~~~

- **Python 3.8+** 
- **Linux** (fully supported)
- **macOS and Windows** (experimental - in testing)

.. note::
    macOS and Windows support is still being tested. Linux is the primary supported platform.
    Please report any issues on `GitHub <https://github.com/iterorganization/IMAS-Core/issues>`_.

Binary wheels are provided for all platforms, so you don't need to compile anything.



**Still not working?**

See :doc:`../troubleshooting` for common issues.

Next Steps
----------

For comprehensive examples and advanced usage patterns, see:

- `IMAS-Python Documentation <https://imas-python.readthedocs.io/en>`_ - High-level interface with detailed examples
- :doc:`backends_guide` - Learn about different data backends
- :doc:`index` - Start using IMAS-Core with Python
- :doc:`backends_guide` - Understand available data backends
