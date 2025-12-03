Installation
=============

Quick Install
-------------

The easiest way to install IMAS-Core is via pip:

.. code-block:: bash

    pip install imas-core
    python -c "import imas_core"

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


Build and Install from Source
==============================

This page describes how to build and install the IMAS-Core from source code.

.. _`build prerequisites`:

IMAS-Core
---------

To build the IMAS-Core you need:

-   Git
-   A C++11 compiler (tested with GCC and Intel compilers)
-   CMake (3.16 or newer)
-   Saxon-HE XSLT processor
-   Boost C++ libraries (1.66 or newer)
-   PkgConfig

The following dependencies are only required for some of the components:

-   Backends

    -   **HDF5 backend**: HDF5 C/C++ libraries (1.8.12 or newer)
    -   **MDSplus backend**: MDSplus libraries (7.84.8 or newer)
    -   **UDA backend**: `UDA <https://github.com/ukaea/UDA/>`__ libraries
        (2.7.5 or newer) [#uda_install]_

..  [#uda_install] When installing UDA, make sure you have 
    `Cap'n'Proto <https://github.com/capnproto/capnproto>`__ installed in your system
    and add its support by adding the CMake switch `-DENABLE_CAPNP=ON` when configuring UDA. 


Standard environments:

.. md-tab-set::

    .. md-tab-item:: Ubuntu 22.04

        The following packages provide most requirements when using Ubuntu 22.04:

        .. code-block:: bash

            apt install git build-essential cmake libsaxonhe-java libboost-all-dev \
                pkg-config libhdf5-dev xsltproc libblitz0-dev gfortran \
                default-jdk-headless python3-dev python3-venv python3-pip

        The following dependencies are not available from the package repository,
        you will need to install them yourself:

        -   MDSplus: see their `GitHub repository
            <https://github.com/MDSplus/mdsplus>`__ or `home page
            <https://mdsplus.org/>`__ for installation instructions.
        -   UDA: see their `GitHub repository <https://github.com/ukaea/UDA>`__ for more
            details.
        -   MATLAB, which is not freely available.


Building and installing IMAS-Core
---------------------------------

Please make sure you have the :ref:`build prerequisites` installed.


Clone the repository
````````````````````

First you need to clone the repository of the IMAS-Core you want to build:

.. code-block:: bash

    # For the IMAS-Core use:
    git clone git@github.com:iterorganization/IMAS-Core.git


cmake configuration
```````````````````

Once you have cloned the repository, navigate your shell to the folder and run cmake.
You can pass configuration options with ``-D OPTION=VALUE``. See below list for an
overview of configuration options.

.. code-block:: bash

    cd IMAS-Core
    cmake -B build -D CMAKE_INSTALL_PREFIX=$HOME/install -D OPTION1=VALUE1 -D OPTION2=VALUE2 [...]

.. note:: 

    CMake will automatically fetch dependencies from required repositories
    for you. 

    -   `data-dictionary (git@github.com:iterorganization/IMAS-Data-Dictionary.git)
        <https://github.com/iterorganization/IMAS-Data-Dictionary>`__

    If you need to change the git repositories, for example to point to a mirror of the
    repository or to use a HTTPS URL instead of the default SSH URLs, you can update the
    :ref:`configuration options`. For example, add the following options to your
    ``cmake`` command to download the repositories over HTTPS instead of SSH:
    
    .. code-block:: text
        :caption: Use explicit options to download dependent repositories over HTTPS

        cmake -B build \
            -D AL_CORE_GIT_REPOSITORY=https://github.com/iterorganization/IMAS-Core.git \
            -D DD_GIT_REPOSITORY=https://github.com/iterorganization/IMAS-Data-Dictionary.git

    If you use CMake 3.21 or newer, you can also use the ``https`` preset:

    .. code-block:: text
        :caption: Use CMake preset to set to download dependent repositories over HTTPS

        cmake -B build --preset=https


Choosing the compilers
''''''''''''''''''''''

You can instruct CMake to use compilers with the following environment variables:

-   ``CC``: C compiler, for example ``gcc`` or ``icc``.
-   ``CXX``: C++ compiler, for example ``g++`` or ``icpc``.
-   ``FC``: Fortran compiler, for example ``gfortran``, ``ifort`` or ``nagfor``.

If you don't specify a compiler, CMake will take a default (usually from the Gnu
Compiler Collection).

.. importafixnt::

    These environment variables must be set before the first time you configure
    ``cmake``!

    If you have an existing ``build`` folder and want to use a different compiler, you
    should delete the ``build`` folder first, or use a differently named folder for the
    build tree.


Configuration options
'''''''''''''''''''''

-   **Backend configuration options**

    -   ``AL_BACKEND_HDF5``, allowed values ``ON`` *(default)* or ``OFF``.
        Enable/disable the HDF5 backend.
    -   ``AL_BACKEND_MDSPLUS``, allowed values ``ON`` or ``OFF`` *(default)*.
        Enable/disable the MDSplus backend.

        -   ``AL_BUILD_MDSPLUS_MODELS``, allowed values ``ON`` *(default)* or ``OFF``,
            only available when the MDSplus backend is enabled. Enable building MDSplus
            models for the selected Data Dictionary version.

    -   ``AL_BACKEND_UDA``, allowed values ``ON`` or ``OFF`` *(default)*. Enable/disable
        the UDA backend.
    -   ``AL_BACKEND_UDAFAT``, allowed values ``ON`` or ``OFF`` *(default)*.
        Enable/disable the UDA backend and use FAT UDA instead of the client/server
        model. See the `UDA documentation <https://ukaea.github.io/UDA/>`__ for more
        information.

-   **Control what to build**

    -   ``AL_TESTS``, allowed values ``ON`` or ``OFF`` *(default)*. Enable/disable
        building the test programs in the ``tests`` folder.
    -   ``AL_PLUGINS``, allowed values ``ON`` *(default)* or ``OFF``. Enable/disable
        building the plugins from the ``al-plugins`` repository.
    -   ``AL_DOCS_ONLY``, allowed values ``ON`` or ``OFF`` *(default)*. When enabled,
        ONLY the documentation will be built (needs ``AL_HLI_DOCS=ON``). Regardless of
        other configuration options, nothing else will be built.
    -   ``AL_PYTHON_BINDINGS``, allowed values ``ON`` *(default when building the Python
        HLI)* or ``OFF`` *(default when not building the Python HLI)*. When enabled, this
        builds the Access Layer Python lowlevel bindings.

-   **Dependency configuration options**

    -   ``AL_DOWNLOAD_DEPENDENCIES``, allowed values ``ON`` *(default)* or ``OFF``.
        Enable or disable the automatic downloading of dependencies. Should be disabled
        when using a development environment

    .. important::

        The following environment variables must be set before the first time you
        configure ``cmake``!

        If you have an existing ``build`` folder and want to use a different compiler,
        you should delete the ``build`` folder first, or use a differently named folder
        for the build tree.

    When ``AL_DOWNLOAD_DEPENDENCIES`` is enabled, the following settings can be used to
    configure the location and/or version of the dependencies that should be used.
    
    -   ``AL_CORE_GIT_REPOSITORY``,
        , ``DD_GIT_REPOSITORY``. Configure the git URLs
        where the ``IMAS-Core`` c.q.
        ``data-dictionary`` repositories should be fetched from.
    -   ``AL_CORE_VERSION``, ``AL_PLUGINS_VERSION``,
        ``DD_VERSION``. Configure the version of the repository that should be used.
        This can point to any valid branch name, tag or commit hash.

        This setting can be used to control which version of the Data Dictionary you
        want to use. For example: ``-D DD_VERSION=3.38.1`` will use DD version 3.38.1
        instead of the default.

    .. code-block:: text
        :caption: Default values for ``*_GIT_REPOSITORY`` and ``*_VERSION`` options

        AL_CORE_GIT_REPOSITORY:     git@github.com:iterorganization/IMAS-Core.git
        AL_CORE_VERSION:            main


        DD_GIT_REPOSITORY:          git@github.com:iterorganization/IMAS-Data-Dictionary.git
        DD_VERSION:                 main

-   **Useful CMake options**

    -   ``CMAKE_INSTALL_PREFIX``. Configure the path where the Access Layer will be
        installed, for example ``-D CMAKE_INSTALL_PREFIX=$HOME/install`` will install
        the IMAS-Core inside the ``install`` folder in your home directory.
    -   ``CMAKE_BUILD_TYPE``. Configure the build type for compiled languages.
        Supported values (case sensitive):

        -   ``Debug``: build with debug symbols and minimal optimizations.
        -   ``Release``: build with optimizations enabled, without debug symbols.
        -   ``RelWithDebInfo`` *(default)*: build with optimizations and debug symbols
            enabled.
        -   ``MinSizeRel``: build optimized for minimizing the size of the resulting
            binaries.

More advanced options are available as well, these can be used to configure where CMake
searches for the prerequisite dependencies (such as the Boost libraries). To show all
available configuration options, use the command-line tool ``ccmake`` or the gui tool
``cmake-gui``:

.. code-block:: bash

    # for the CLI tool
    ccmake -B build -S .
    # for the GUI tool
    cmake-gui -B build -S .


Build the IMAS-Core
```````````````````

Use ``cmake`` to build as shown below. 

.. code-block:: bash

    # Instruct make to build "all" in the "build" folder, using at most "8" parallel
    # processes:
    CMAKE_ARGS=(${CMAKE_ARGS[@]}
        -D"CMAKE_INSTALL_PREFIX=$(pwd)/test-install/"
        # Enable all backends
        -DAL_BACKEND_HDF5=ON
        -DAL_BACKEND_MDSPLUS=ON
        -DAL_BACKEND_UDA=ON
        # Build MDSplus models
        -DAL_BUILD_MDSPLUS_MODELS=ON
        # Build Python bindings
        -DAL_PYTHON_BINDINGS=no-build-isolation
        # Download dependencies from HTTPS (using an access token):
        -DAL_DOWNLOAD_DEPENDENCIES=ON
        -DDD_GIT_REPOSITORY=https://github.com/iterorganization/IMAS-Data-Dictionary.git
        # DD version:
        -DDD_VERSION=$DD_VERSION
        # Work around Boost linker issues on 2020b toolchain
        -DBoost_NO_BOOST_CMAKE=ON
    )
    echo "CMake args:"
    echo ${CMAKE_ARGS[@]} | tr ' ' '\n'

    # Note: we don't set CC or CXX compiler, so CMake will pick the default (GCC) compilers
    cmake -Bbuild -GNinja "${CMAKE_ARGS[@]}"

    # Build and install
    cmake --build build --target install

.. note::

    By default CMake on Linux will create ``Unix Makefiles`` for actually building
    everything, as assumed in this section.

    You can select different generators (such as Ninja) if you prefer, but these are not
    tested. See the `CMake documentation
    <https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html>`__ for more
    details.


Optional: Test the IMAS-Core
````````````````````````````
Following snippet shows how to run the test with pytest. Just ensure that you have 
AL_PYTHON_BINDINGS-ON in cmake configuration.
If you set either of the options ``AL_EXAMPLES`` or ``AL_TESTS`` to ``ON``, you can run
the corresponding test programs as follows:

.. code-block:: bash

    python3 -m venv build/pip_install
    source build/pip_install/bin/activate
    pip install --upgrade pip wheel
    pip install numpy 
    set -x
    pip install --find-links=build/dist imas-core[test,cov]
    pytest --junitxml results.xml --cov imas_core --cov-report xml --cov-report html
    coverage2clover -i coverage.xml -o clover.xml


Use the IMAS-Core
`````````````````

After installing the IMAS-Core, you need to ensure that your code can find the installed
Access Layer. To help you with this, a file ``al_env.sh`` is installed. You can
``source`` this file to set all required environment variables:

.. code-block:: bash
    :caption: Set environment variables (replace ``<install_dir>`` with your install folder)

    source <install_dir>/bin/al_env.sh

You may want to add this to your ``$HOME/.bashrc`` file to automatically make the IMAS-Core
 installation available for you.

.. note:: 

    To use a ``public`` dataset, you also need to set the ``IMAS_HOME`` environment
    variable. For example, on SDCC, this would be ``export IMAS_HOME=/work/imas``.

    Some programs may rely on an environment variable ``IMAS_VERSION`` to detect which
    version of the data dictionary is used in the current IMAS environment. You may set
    it manually with the DD version you've build the HLI with, for example: ``export
    IMAS_VERSION=3.41.0``.

Once you have set the required environment variables, you may continue Using the
IMAS-Core.

If IMAS-Core is built with Python bindings, you can also use the Python bindings with imas-python. 
`imas_python <https://pypi.org/project/imas-python/>`_. Just install with `pip install imas-python`
and you can use the Python bindings. more information is here `imas-python <https://imas-python.readthedocs.io/en/latest/>`_.


Next Steps
----------

For comprehensive examples and advanced usage patterns, see:

- `IMAS-Python Documentation <https://imas-python.readthedocs.io/en>`_ - High-level interface with detailed examples
- :doc:`backends_guide` - Learn about different data backends
- :doc:`index` - Start using IMAS-Core with Python
