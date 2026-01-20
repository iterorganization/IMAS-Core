Troubleshooting
===============

Common issues and solutions when using IMAS-Core.

Installation Issues
-------------------

**Could not import 'imas_core': No module named 'imas_core'. Some functionality is not available.**

*Problem:* IMAS-Core not installed correctly

*Solutions:*

1. Verify installation:

   .. code-block:: bash

       pip list | grep imas-core

2. Reinstall:

   .. code-block:: bash

       pip install --force-reinstall imas-core

3. Check Python version:

   .. code-block:: bash

       python --version


Getting Help
------------

If you can't find the solution:

1. **Check Documentation:**
   - :doc:`user_guide/index` - User guide
   - :doc:`developers/index` - Developer guide
   - :doc:`user_guide/backends_guide` - Backend documentation

2. **Search Issues:**
   - https://github.com/iterorganization/IMAS-Core/issues

3. **Report Bug:**
   - Include: OS, Python version, IMAS-Core version
   - Include: Full error message and stack trace
   - Include: Minimal code to reproduce

4. **Contact Support:**
   - Email: imas-support@iter.org
