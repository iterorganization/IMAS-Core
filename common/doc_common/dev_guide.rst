Access Layer development guide
==============================


Access Layer repositories
-------------------------

The IMAS Access Layer consists of a number of components which are developed in separate
repositories:

-   `al-common <https://git.iter.org/projects/IMAS/repos/al-common/browse>`__: common
    files used by other repositories

    This includes build scripts (in the ``cmake`` folder) and common documentation
    templates (in the ``doc_common`` folder).

-   `al-core <https://git.iter.org/projects/IMAS/repos/al-core/browse>`__: the
    Access Layer core repository, MDSplus model generator and Python lowlevel
    bindings.
-   `data-dictionary
    <https://git.iter.org/projects/IMAS/repos/data-dictionary/browse>`__: the IMAS Data
    Dictionary definitions, used for generating MDSplus models and the traditional High
    Level Interfaces.
-   `al-plugins <https://git.iter.org/projects/IMAS/repos/al-plugins/browse>`__: Access
    Layer plugins.
-   Traditional (code-generated) High Level Interfaces

    -   `al-cpp <https://git.iter.org/projects/IMAS/repos/al-cpp/browse>`__: C++ HLI
    -   `al-fortran <https://git.iter.org/projects/IMAS/repos/al-fortran/browse>`__:
        Fortran HLI
    -   `al-java <https://git.iter.org/projects/IMAS/repos/al-java/browse>`__: Java HLI
    -   `al-matlab <https://git.iter.org/projects/IMAS/repos/al-matlab/browse>`__:
        MATLAB HLI
    -   `al-python <https://git.iter.org/projects/IMAS/repos/al-python/browse>`__:
        Python HLI

-   Non-generated code HLIs. The following High Level Interfaces load the Data
    Dictionary definitions at runtime

    -   `IMASPy <https://git.iter.org/projects/IMAS/repos/imaspy/browse>`__: alternative
        Python HLI
    -   `al-hdc <https://git.iter.org/projects/IMAS/repos/al-hdc/browse>`__: alternative
        HLI based on the HDC (Hierarchical Data Containers) library

The documentation on this page covers everything except the Non-generated HLIs, those
are documented in their own projects.


Development environment
-----------------------

.. note::

    This is the first iteration of the development process after the Access Layer split.
    The process is not set in stone or sacred. Please suggest improvements to the
    development flow based on your experience, if you feel the process the can be
    streamlined.

See the :ref:`build prerequisites` section for an overview of modules you need to load
when on SDCC or packages to install when using Ubuntu 22.04.

The recommended development folder layout is to clone all :ref:`Access Layer
repositories` in a single root folder (``al-dev`` in below example, but the name of that
folder is not important).

.. code-block:: text

    al-dev/                 # Feel free to name this folder however you want
    ├── al-common/
    ├── al-core/
    ├── al-plugins/         # Optional
    ├── al-cpp/             # Optional
    ├── al-fortran/         # Optional
    ├── al-java/            # Optional
    ├── al-matlab/          # Optional
    ├── al-python/          # Optional
    └── data-dictionary/

Then, when you configure a project for building (see :ref:`Configuration`), set the
option ``-D AL_DOWNLOAD_DEPENDENCIES=OFF``. Instead of fetching requirements from the
ITER git, CMake will now use the repositories as they are checked out in your
development folders.

The additional option ``-D AL_DEVELOPMENT_LAYOUT=ON`` may be required when you have set
the environment variable ``$AL_COMMON_PATH``. This environment variable is set by the
easybuild module ``IMAS-AL-Common`` and is used in the CMake configuration to build
easybuild modules.

.. important::

    With this setup, it is your responsibility to update the repositories to their
    latest versions (if needed). The ``<component>_VERSION`` configuration options are
    ignored when ``AL_DOWNLOAD_DEPENDENCIES=OFF``.

This setup allows you to develop in multiple repositories in parallel.


Dependency management
---------------------

With all Access Layer components spread over different repositories, managing
dependencies is more complex than before. Below diagram expresses the dependencies
between the different repositories:

.. md-mermaid::
    :name: repository-dependencies

    flowchart
        core[al-core] -->|"MDSplus<br>models"| dd[data-dictionary]
        core[al-core] -->|"CMake<br>configuration"| common[al-common]
        plugins[al-plugins] --> core
        hli["al-{hli}"] --> core
        hli --> common
        hli --> dd
        hli --> plugins

To manage the "correct" version of each of the dependencies, the CMake configuration
specifies which branch to use from each repository:

-   Each HLI indicates which commit to use from the ``al-common`` repository. This is
    defined by the ``AL_COMMON_VERSION`` cache string in the main ``CMakeLists.txt`` of
    the repository.

    The default version used is ``main``, which is the last stable release of
    ``al-common``.
-   Inside the ``al-common`` repository, the commits to use for the ``al-core``,
    ``al-plugins`` and ``data-dictionary`` are set in `ALCommonConfig.cmake
    <https://git.iter.org/projects/IMAS/repos/al-common/browse/cmake/ALCommonConfig.cmake>`__.

    The default versions used are ``main`` for ``al-core``, and ``master/3`` for
    ``data-dictionary``.


.. info::

    CMake supports setting branch names, tags and commit hashes for the dependencies.


CMake
-----

We're using CMake for the build configuration. See `the CMake documentation
<https://cmake.org/cmake/help/latest/>`__ for more details about CMake.

The ``FetchContent`` CMake module for making :ref:`dependencies from other repositories
<dependency management>` available. For more information on this module we refer to the
`FetchContent CMake documentation
<https://cmake.org/cmake/help/latest/module/FetchContent.html>`__


Documentation overview
----------------------

The documentation is generated with Sphinx. Because the documentation of each HLI
depends on the contents of the ``al-plugins`` and ``al-common`` repositories, it is
configured with CMake. For more information on Sphinx, see the `Sphinx docs
<https://www.sphinx-doc.org/en/master/>`__ and the `documentation of the theme
(sphinx-immaterial) that we're using
<https://jbms.github.io/sphinx-immaterial/index.html>`__.

Documentation of the HLI is inside the ``doc`` folder of the repository. This folder
contains the configuration (``conf.py``), and documentation pages (``*.rst``).
Documentation that is common to all High Level Interfaces (such as this developer guide)
is in the `doc_common folder in the al-common repository
<https://git.iter.org/projects/IMAS/repos/al-common/browse/doc_common>`__. If you need
to update the common documentation, then don't forget to update the
``AL_COMMON_VERSION`` tag in the HLI repositories.


Building the documentation
''''''''''''''''''''''''''

Use the option ``-D AL_HLI_DOCS`` to enable building documentation. This will create a
target ``al-<hli>-docs``, e.g. ``al-python-docs`` that will only build the
documentation. You could also use ``-D AL_DOCS_ONLY`` to only build the documentation,
and nothing else.

.. code-block:: console
    :caption: Example: building the documentation for the Python HLI

    al-dev$ cd al-python
    al-python$ # Configure cmake to only create the documentation:
    al-python$ cmake -B build -D AL_HLI_DOCS -D AL_DOCS_ONLY
    [...]
    al-python$ make -C build al-python-docs
    [...]


CI and deployment overview
--------------------------

Main CI plans. These plans execute the script ``ci/build_and_test.sh`` in each of the
repositories. This allows for easy reproduction of the CI plans on SDCC: just execute
``bash ci/build_and_test.sh`` in a clone of the repository.

-   `AL-Core <https://ci.iter.org/browse/IC8-ALCORE>`__ tests building the Access
    Layer core. Note that there are no unit tests in this repository, the CI plan
    only checks that the AL can be built successfully.
-   `AL-Cpp <https://ci.iter.org/browse/IC8-ALCPP>`__ builds and tests the C++ HLI.
-   `AL-Fortran <https://ci.iter.org/browse/IC8-ALFOR>`__ builds and tests the Fortran
    HLI.

    .. note::
        There are three Jobs inside this plan. One uses the GCC compilers, one uses the
        Intel compilers and the third uses the NAGfor Fortran compiler.

        All three execute ``ci/build_and_test.sh``, but some set the ``CC``, ``CXX`` and
        ``FC`` environment variables to select the compiler.

-   `AL-Java <https://ci.iter.org/browse/IC8-ALJAVA>`__ builds and tests the Java HLI.
-   `AL-Matlab <https://ci.iter.org/browse/IC8-ALMAT>`__ builds and tests the Matlab HLI.
-   `AL-Python <https://ci.iter.org/browse/IC8-ALPY>`__ builds and tests the Python HLI.

Documentation CI plan:

-   `Access Layer Doc <https://ci.iter.org/browse/IC8-ALDOC>`__ builds the AL
    documentation for all HLIs (only on the ``main`` and ``develop`` branches). The
    output of this CI plan is used to deploy the documentation to sharepoint with the
    `Access-Layer doc deployment project
    <https://ci.iter.org/deploy/viewDeploymentProjectEnvironments.action?id=1887895553>`__.

Other CI plans:

-   `Data-Dictionary Dev <https://ci.iter.org/browse/IC8-DDDEV2>`__ builds and tests all
    HLIs (develop branch) with the Data Dictionary branch that triggered the build.

    This project is used to test the compatibility of new Data Dictionary developments
    with the generated Access Layer HLIs. This CI plan is triggered for the develop
    branches of the Data Dictionary, and with every PR in the Data-Dictionary
    repository.

    .. note::
        The Main CI plans use the last released Data Dictionary version for testing.

-   `IMAS AL DEV <https://ci.iter.org/browse/IC8-ALDEV>`__ builds development modules
    for all components. These modules are published to SDCC by the `IMAS AL DEV Deploy
    <https://ci.iter.org/deploy/viewDeploymentProjectEnvironments.action?id=1908899846>`__
    deployment project.

    These development modules can be used on SDCC as follows:

    .. code-block:: bash
        :caption: Development modules based on the Data Dictionary ``develop/3`` branch

        module use /work/imas/opt/bamboo_deploy/imas3-dev-modules/modules/all/
        # For intel:
        module load IMAS/develop3-develop-intel-2020b
        # For foss:
        module load IMAS/develop3-develop-foss-2020b

    .. code-block:: bash
        :caption: Development modules based on the Data Dictionary ``develop/4`` branch
        
        module use /work/imas/opt/bamboo_deploy/imas4-dev-modules/modules/all/
        # For intel:
        module load IMAS/develop4-develop-intel-2020b
        # For foss:
        module load IMAS/develop4-develop-foss-2020b
