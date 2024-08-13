Using the Access Layer
======================

Making the Access Layer available for use
-----------------------------------------

To use the Access Layer in your programs, you need to make it available. On the
ITER cluster (SDCC) this is achieved by loading the ``IMAS`` module from the
command line:

.. code-block:: bash

    module load IMAS

This will select the default IMAS module. You can also select a specific
version. To see which versions of the IMAS module are available, enter on the
command line:

.. code-block:: bash

    module avail IMAS

Please check with your HPC administrators which module you need to load when not working
on SDCC. When you're working with a local installation (see :ref:`Building and
installing the Access Layer`), you can source the installed environment file:

.. code-block:: bash
    :caption: Set environment variables (replace ``<install_dir>`` with the folder of your local install)

    source <install_dir>/bin/al_env.sh


..
    HLI-specific documentation should include the following items:

    - Code samples for loading the library (import, include, use, etc.)
    - Example program printing the Access Layer version used
    - Instructions for (compiling if relevant) and running the example program
