FAQ
===

Frequently Asked Questions about IMAS-Core.

Installation
------------

**Q: How do I install IMAS-Core?**

A: Use pip:

.. code-block:: bash

    pip install imas-core

**Q: What Python versions are supported?**

A: We recommend Python 3.8+.

**Q: Can I build from source?**

A: Yes, see :doc:`user_guide/installation`.

Usage
-----

**Q: What is an IDS?**

A: An Integrated Data Structure (IDS) is a standardized data container defined
by the IMAS Data Dictionary. Examples include 'equilibrium', 'magnetics', etc.

**Q: What is a backend?**

A: A backend is a storage format (HDF5, MDSplus, etc.). The same code works
with different backends by changing the URI.

Backends
--------

**Q: What is the difference between backends?**

A: See comparison in :doc:`user_guide/backends_guide`.

Development
-----------

**Q: How do I contribute to IMAS-Core?**

A: See the GitHub repository for guidelines: https://github.com/iterorganization/IMAS-Core

**Q: How do I build from source?**

A: See :doc:`user_guide/installation`.

**Q: How do I report bugs?**

A: Open an issue on GitHub: https://github.com/iterorganization/IMAS-Core/issues


Miscellaneous
-------------


**Q: What is the IMAS Data Dictionary?**

A: Defines the structure of all IDS. See https://imas-data-dictionary.readthedocs.io/

Still Have Questions?
---------------------

- :doc:`troubleshooting` - Common issues and solutions
- Email: imas-support@iter.org
- GitHub Issues: https://github.com/iterorganization/IMAS-Core/issues
