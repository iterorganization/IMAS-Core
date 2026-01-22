==========================================
Windows Installation Guide
==========================================

This guide provides instructions for building and installing IMAS-Core with MATLAB support on Windows.

Prerequisites
=============

1. **Visual Studio 2022** with:
   - Desktop Development with C++
   - C++ Make Tools for Windows

2. **MATLAB R2025b** (or compatible version)

3. **CMake** (included with Visual Studio)

4. **Python 3.10+** with venv module (verify with ``python --version``)


Setup vcpkg
===========

.. code-block:: batch

    git clone https://github.com/microsoft/vcpkg.git
    cd vcpkg
    bootstrap-vcpkg.bat


Configure PowerShell Environment
================================

Run these commands in PowerShell before building:

.. code-block:: powershell

    $env:PATH += ";C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\"
    $env:PATH += ";C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\HostX86\x86"
    $env:PATH += ";<VCPKG_INSTALLATION_PATH>"


if using already built IMAS-Core:

.. code-block:: powershell

    $env:AL_COMMON_PATH = "<IMAS_CORE_INSTALL_DIR>\share\common"

Build Configuration
===================

**Debug Build:**

.. code-block:: bash
    # with   -DAL_DOWNLOAD_DEPENDENCIES=ON
    cmake -Bbuild -S . -DVCPKG=ON -DAL_PYTHON_BINDINGS=ON -DCMAKE_INSTALL_PREFIX="<INSTALL_PATH>" -DCMAKE_TOOLCHAIN_FILE="<VCPKG_PATH>/scripts/buildsystems/vcpkg.cmake" -DAL_DOWNLOAD_DEPENDENCIES=ON -DAL_EXAMPLES=OFF -DAL_TESTS=OFF -DAL_PLUGINS=OFF -DAL_HLI_DOCS=OFF -DAL_DOCS_ONLY=OFF -DDD_VERSION="3.39.0" -DAL_BACKEND_UDA=OFF -DAL_BACKEND_UDAFAT=OFF -DAL_BACKEND_MDSPLUS=OFF
    # with   -DAL_DOWNLOAD_DEPENDENCIES=OFF
    cmake -Bbuild -S . -DVCPKG=ON -DAL_PYTHON_BINDINGS=ON -DCMAKE_INSTALL_PREFIX="<INSTALL_PATH>" -DCMAKE_TOOLCHAIN_FILE="<VCPKG_PATH>/scripts/buildsystems/vcpkg.cmake" -DAL_DOWNLOAD_DEPENDENCIES=OFF -DAL_EXAMPLES=OFF -DAL_TESTS=OFF -DAL_PLUGINS=OFF -DAL_HLI_DOCS=OFF -DAL_DOCS_ONLY=OFF -DDD_VERSION="3.39.0" -DAL_BACKEND_UDA=OFF -DAL_BACKEND_UDAFAT=OFF -DAL_BACKEND_MDSPLUS=OFF -DCMAKE_PREFIX_PATH="<IMAS_CORE_INSTALL_DIR>"

**Release Build:**

.. code-block:: bash

    # with   -DAL_DOWNLOAD_DEPENDENCIES=ON
    cmake -Bbuild -S . -DCMAKE_BUILD_TYPE=Release -DVCPKG=ON -DAL_PYTHON_BINDINGS=ON -DCMAKE_INSTALL_PREFIX="<INSTALL_PATH>" -DCMAKE_TOOLCHAIN_FILE="<VCPKG_PATH>/scripts/buildsystems/vcpkg.cmake" -DAL_DOWNLOAD_DEPENDENCIES=ON -DAL_EXAMPLES=OFF -DAL_TESTS=OFF -DAL_PLUGINS=OFF -DAL_HLI_DOCS=OFF -DAL_DOCS_ONLY=OFF -DDD_VERSION="3.39.0" -DAL_BACKEND_UDA=OFF -DAL_BACKEND_UDAFAT=OFF -DAL_BACKEND_MDSPLUS=OFF
    # with   -DAL_DOWNLOAD_DEPENDENCIES=OFF
    cmake -Bbuild -S . -DCMAKE_BUILD_TYPE=Release -DVCPKG=ON -DAL_PYTHON_BINDINGS=ON -DCMAKE_INSTALL_PREFIX="<INSTALL_PATH>" -DCMAKE_TOOLCHAIN_FILE="<VCPKG_PATH>/scripts/buildsystems/vcpkg.cmake" -DAL_DOWNLOAD_DEPENDENCIES=OFF -DAL_EXAMPLES=OFF -DAL_TESTS=OFF -DAL_PLUGINS=OFF -DAL_HLI_DOCS=OFF -DAL_DOCS_ONLY=OFF -DDD_VERSION="3.39.0" -DAL_BACKEND_UDA=OFF -DAL_BACKEND_UDAFAT=OFF -DAL_BACKEND_MDSPLUS=OFF -DCMAKE_PREFIX_PATH="<IMAS_CORE_INSTALL_DIR>"


Build and Install
=================

.. code-block:: bash
    # For Debug build
    cmake --build build --config Debug --target install

    # For Release build
    cmake --build build --config Release --target install


Using in MATLAB
===============

Example MATLAB script to access IMAS data:

.. code-block:: matlab

    % Set PATH to find all required DLLs
    setenv('PATH', [getenv('PATH') ...
        ';<IMAS_MATLAB_PATH>\build\Release' ...
        ';<IMAS_MATLAB_PATH>\matlab' ...
        ';<IMAS_MATLAB_PATH>\build\_deps\al-core-build\Release' ...
        ';<IMAS_MATLAB_PATH>\build\vcpkg_installed\x64-windows\bin']);

    % Add MATLAB helper functions and MEX files
    addpath('<IMAS_MATLAB_PATH>\matlab');
    addpath('<IMAS_MATLAB_PATH>\build\Release');

    % Open IMAS pulse file
    uri = 'imas:hdf5?path=<PATH_TO_DATA_FILE>';
    ctx = imas_open(uri, 40);
    if ctx < 0
        error('Unable to open pulse');
    end

    % Get IDS structure
    try
        m = ids_get(ctx, 'waves');
        disp('Success!')
    catch ME
        disp(['Error: ' ME.message])
    end

    % Display results
    disp('=== IDS Properties ===');
    disp(['Comment: ' m.ids_properties.comment]);
    disp(['Data Dictionary: ' m.ids_properties.version_put.data_dictionary]);


Run MATLAB Tests
================

.. code-block:: bash

    matlab -batch "test_code"


Troubleshooting
===============

- Ensure all PowerShell environment variables are set before running CMake
- Verify Visual Studio C++ build tools are installed
- Check that all dependencies are accessible at the specified network paths
- Confirm Python installation with ``python --version``
