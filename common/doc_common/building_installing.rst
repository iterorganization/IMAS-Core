Building and installing the Access Layer
========================================

This page describes how to build and install the Access Layer.

Documentation for developers wishing to contribute to the Access Layer can be found in
the :ref:`Access Layer development guide`. Please refer to that guide if you wish to set
up a development environment.


.. _`build prerequisites`:

Prerequisites
-------------

To build the Access Layer you need:

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


-   High Level Interfaces

    -   All HLIs require the ``xsltproc`` program
    -   **C++ High Level Interface**: Blitz++ libraries
    -   **Fortran High Level Interface**: A fortran compiler, ideally with F2008 support
        (tested with ``gfortran``, ``ifort`` and ``nagfor``)
    -   **Java High Level Interface**: Java Development Kit and Java Native Interface
        (tested with Java versions 11, 17 and 21)
    -   **MATLAB High Level Interface**: A working MATLAB installation (tested with
        version 2020b)
    -   **Python High Level Interface**: Python (version 3.8 or newer) with the pip
        packages ``build``, ``cython`` and ``numpy`` installed.




Standard environments:

.. md-tab-set::

    .. md-tab-item:: SDCC ``intel-2020b``

        The following modules provide all the requirements when using the
        ``intel-2020b`` toolchain:

        .. code-block:: bash

            module load intel/2020b CMake/3.24.3-GCCcore-10.2.0 Saxon-HE/11.4-Java-11 \
                Boost/1.74.0-GCC-10.2.0 HDF5/1.10.7-iimpi-2020b \
                MDSplus/7.96.17-GCCcore-10.2.0 MDSplus-Java/7.96.17-GCCcore-10.2.0-Java-11 \
                UDA/2.7.4-GCCcore-10.2.0 Blitz++/1.0.2-GCCcore-10.2.0 \
                MATLAB/2020b-GCCcore-10.2.0-Java-11 SciPy-bundle/2020.11-intel-2020b

    .. md-tab-item:: SDCC ``foss-2023b``

        The following modules provide all the requirements when using the
        ``foss-2023b`` toolchain:

        .. code-block:: bash

            module load CMake/3.27.6-GCCcore-13.2.0 Saxon-HE/12.4-Java-21 \
                Boost/1.83.0-GCC-13.2.0 HDF5/1.14.3-gompi-2023b \
                MDSplus/7.132.0-GCCcore-13.2.0 \
                UDA/2.8.0-GCC-13.2.0 Blitz++/1.0.2-GCCcore-13.2.0 \
                MATLAB/2023b-r5 SciPy-bundle/2023.11-gfbf-2023b \
                build/1.0.3-foss-2023b

        .. admonition:: The MATLAB/2023b-r5 installation is lightly tweaked

            The installation at ITER uses `EB PR#20508 
            <https://github.com/easybuilders/easybuild-easyconfigs/pull/20508>`__
            and its tweak resolves `IMAS-5162 <https://jira.iter.org/browse/IMAS-5162>`__ 
            by removing ``libstdc++.so.6`` from the MATLAB installation. It also adds 
            ``extern/bin/glnxa64`` to ``LD_LIBRARY_PATH`` to address the
            ``MatlabEngine not found`` issue.

        .. caution::

            When using the HDF5 backend within MATLAB, depending on the HDF5 library being used
            you may need to add ``LD_PRELOAD=<hdf5_install_dir>/lib/libhdf5_hl.so`` when starting
            MATLAB.

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


Building and installing a single High Level Interface
-----------------------------------------------------

This section explains how to install a single High Level Interface. Please make sure you
have the :ref:`build prerequisites` installed.


Clone the repository
````````````````````

First you need to clone the repository of the High Level Interface you want to build:

.. code-block:: bash

    # For the C++ HLI use:
    git clone ssh://git@git.iter.org/imas/al-cpp.git
    # For the Fortran HLI use:
    git clone ssh://git@git.iter.org/imas/al-fortran.git
    # For the Java HLI use:
    git clone ssh://git@git.iter.org/imas/al-java.git
    # For the MATLAB HLI use:
    git clone ssh://git@git.iter.org/imas/al-matlab.git
    # For the Python HLI use:
    git clone ssh://git@git.iter.org/imas/al-python.git


Configuration
`````````````

Once you have cloned the repository, navigate your shell to the folder and run cmake.
You can pass configuration options with ``-D OPTION=VALUE``. See below list for an
overview of configuration options.

.. code-block:: bash

    cd al-cpp  # al-fortran, al-java, al-matlab or al-python
    cmake -B build -D CMAKE_INSTALL_PREFIX=$HOME/al-install -D OPTION1=VALUE1 -D OPTION2=VALUE2 [...]

.. note:: 

    CMake will automatically fetch dependencies from other Access Layer GIT repositories
    for you. You may need to provide credentials to clone the following repositories:

    -   `al-core (ssh://git@git.iter.org/imas/al-core.git)
        <https://git.iter.org/projects/IMAS/repos/al-core/browse>`__
    -   `al-plugins (ssh://git@git.iter.org/imas/al-plugins.git)
        <https://git.iter.org/projects/IMAS/repos/al-plugins/browse>`__
    -   `data-dictionary (https://github.com/iterorganization/IMAS-data-Dictionary.git)
        <https://github.com/iterorganization/IMAS-data-Dictionary>`__

    If you need to change the git repositories, for example to point to a mirror of the
    repository or to use a HTTPS URL instead of the default SSH URLs, you can update the
    :ref:`configuration options`. For example, add the following options to your
    ``cmake`` command to download the repositories over HTTPS instead of SSH:
    
    .. code-block:: text
        :caption: Use explicit options to download dependent repositories over HTTPS

        cmake -B build \
            -D AL_CORE_GIT_REPOSITORY=https://git.iter.org/scm/imas/al-core.git \
            -D AL_PLUGINS_GIT_REPOSITORY=https://git.iter.org/scm/imas/al-plugins.git \
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

.. important::

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

    -   ``AL_EXAMPLES``, allowed values ``ON`` *(default)* or ``OFF``. Enable/disable
        building the example programs in the ``examples`` directory.
    -   ``AL_TESTS``, allowed values ``ON`` *(default)* or ``OFF``. Enable/disable
        building the test programs in the ``tests`` folder.
    -   ``AL_PLUGINS``, allowed values ``ON`` or ``OFF`` *(default)* . Enable/disable
        building the plugins from the ``al-plugins`` repository.
    -   ``AL_HLI_DOCS``, allowed values ``ON`` or ``OFF`` *(default)*. Enable/disable
        building the documentation.
    -   ``AL_DOCS_ONLY``, allowed values ``ON`` or ``OFF`` *(default)*. When enabled,
        ONLY the documentation will be built (needs ``AL_HLI_DOCS=ON``). Regardless of
        other configuration options, nothing else will be built.
    -   ``AL_PYTHON_BINDINGS``, allowed values ``ON`` *(default when building the Python
        HLI)* or ``OFF`` *(default when not building the Python HLI)*. When enabled, this
        builds the Access Layer Python lowlevel bindings.

-   **Dependency configuration options**

    -   ``AL_DOWNLOAD_DEPENDENCIES``, allowed values ``ON`` *(default)* or ``OFF``.
        Enable or disable the automatic downloading of dependencies. Should be disabled
        when using a :ref:`development environment <Access Layer development guide>`.

    .. important::

        The following environment variables must be set before the first time you
        configure ``cmake``!

        If you have an existing ``build`` folder and want to use a different compiler,
        you should delete the ``build`` folder first, or use a differently named folder
        for the build tree.

    When ``AL_DOWNLOAD_DEPENDENCIES`` is enabled, the following settings can be used to
    configure the location and/or version of the dependencies that should be used.
    
    -   ``AL_CORE_GIT_REPOSITORY``,
        ``AL_PLUGINS_GIT_REPOSITORY``, ``DD_GIT_REPOSITORY``. Configure the git URLs
        where the ``al-core``, ``al-plugins`` c.q.
        ``data-dictionary`` repositories should be fetched from.
    -   ``AL_CORE_VERSION``, ``AL_PLUGINS_VERSION``,
        ``DD_VERSION``. Configure the version of the repository that should be used.
        This can point to any valid branch name, tag or commit hash.

        This setting can be used to control which version of the Data Dictionary you
        want to use. For example: ``-D DD_VERSION=3.38.1`` will use DD version 3.38.1
        instead of the default.

    .. code-block:: text
        :caption: Default values for ``*_GIT_REPOSITORY`` and ``*_VERSION`` options

        AL_CORE_GIT_REPOSITORY:     ssh://git@git.iter.org/imas/al-core.git
        AL_CORE_VERSION:            main

        AL_PLUGINS_GIT_REPOSITORY:  ssh://git@git.iter.org/imas/al-plugins.git
        AL_PLUGINS_VERSION:         main

        DD_GIT_REPOSITORY:          https://github.com/iterorganization/IMAS-data-Dictionary.git
        DD_VERSION:                 main

-   **Useful CMake options**

    -   ``CMAKE_INSTALL_PREFIX``. Configure the path where the Access Layer will be
        installed, for example ``-D CMAKE_INSTALL_PREFIX=$HOME/al-install`` will install
        the Access Layer inside the ``al-install`` folder in your home directory.
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


Build the High Level Interface
``````````````````````````````

Use ``make`` to build everything. You can speed things up by using parallel compiling
as shown with the ``-j`` option. Be careful with the amount of parallel processes
though: it's easy to exhaust your machine's available hardware (CPU or memory) which may
cause the build to fail. This is especially the case with the C++ High Level Interface.

.. code-block:: bash

    # Instruct make to build "all" in the "build" folder, using at most "8" parallel
    # processes:
    make -C build -j8 all

.. note::

    By default CMake on Linux will create ``Unix Makefiles`` for actually building
    everything, as assumed in this section.

    You can select different generators (such as Ninja) if you prefer, but these are not
    tested. See the `CMake documentation
    <https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html>`__ for more
    details.


Optional: Test the High Level Interface
```````````````````````````````````````

If you set either of the options ``AL_EXAMPLES`` or ``AL_TESTS`` to ``ON``, you can run
the corresponding test programs as follows:

.. code-block:: bash

    # Use make:
    make -C build test
    # Directly invoke ctest
    ctest --test-dir build

This executes ``ctest`` to run all test and example programs. Note that this may take a
long time to complete.


Install the High Level Interface
````````````````````````````````

Run ``make install`` to install the high level interface in the folder that you chose in
the configuration step above.


Use the High Level Interface
````````````````````````````

After installing the HLI, you need to ensure that your code can find the installed
Access Layer. To help you with this, a file ``al_env.sh`` is installed. You can
``source`` this file to set all required environment variables:

.. code-block:: bash
    :caption: Set environment variables (replace ``<install_dir>`` with your install folder)

    source <install_dir>/bin/al_env.sh

You may want to add this to your ``$HOME/.bashrc`` file to automatically make the Access
Layer installation available for you.

.. note:: 

    To use a ``public`` dataset, you also need to set the ``IMAS_HOME`` environment
    variable. For example, on SDCC, this would be ``export IMAS_HOME=/work/imas``.

    Some programs may rely on an environment variable ``IMAS_VERSION`` to detect which
    version of the data dictionary is used in the current IMAS environment. You may set
    it manually with the DD version you've build the HLI with, for example: ``export
    IMAS_VERSION=3.41.0``.

Once you have set the required environment variables, you may continue :ref:`Using the
Access Layer`.


Troubleshooting
```````````````

**Problem:** ``Target Boost::log already has an imported location``
    This problem is known to occur with the ``2020b`` toolchain on SDCC. Add the CMake
    configuration option ``-D Boost_NO_BOOST_CMAKE=ON`` to work around the problem.
