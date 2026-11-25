# IMAS-Core

[![Build](https://github.com/iterorganization/IMAS-Core/actions/workflows/wheels.yml/badge.svg?branch=develop)](https://github.com/iterorganization/IMAS-Core/actions/workflows/wheels.yml)
[![Open Issues](https://img.shields.io/github/issues/iterorganization/IMAS-Core)](https://github.com/iterorganization/IMAS-Core/issues)
[![License: LGPL-3.0](https://img.shields.io/github/license/iterorganization/IMAS-Core?label=License)](https://github.com/iterorganization/IMAS-Core/blob/main/LICENSE.txt)


**IMAS-Core** is a lowlevel library with Python bindings for reading and writing fusion experiment data in standard IMAS format.

It provides easy access to data stored in HDF5, MDSplus, and other formats, making it simple to work with fusion physics data across platforms (Linux, macOS, Windows).

This repository contains the **Lowlevel components of the Access Layer**:

- **C lowlevel interface** used by the various High Level Interfaces
- **Python bindings** to the lowlevel interface
- **Backends** for reading and writing IMAS data
- **MDS+ model logic** for creating the models required by the MDS+ backend


## Installation Options

### For Python Users

```bash
# Install from PyPI
pip install imas-core

# Verify installation
python -c "import imas; print(imas.__version__)"
```

### For Developers

To build IMAS-Core from source:

```bash
git clone https://github.com/iterorganization/IMAS-Core.git
cd IMAS-Core
cmake -Bbuild -GNinja -DAL_PYTHON_BINDINGS=ON -DCMAKE_INSTALL_PREFIX="$(pwd)/test-install"
cmake --build build --target install
```

See [Developer Guide](docs/source/developers/index.rst) for build instructions.


## Using IMAS-Core with High-Level Languages

When IMAS-Core is built and installed via CMake, it installs:

- **C/C++ Libraries** (`libal.so`) with full headers
- **Python Bindings** (`imas_core` Python package)
- **Fortran Support** via pkg-config configuration
- **Java Support** via `imas.jar`
- **MATLAB Support** via MEX bindings
- **Models** for MDSplus backend access

After installation, configure your environment:

```bash
export PATH="/path/to/install/bin:$PATH"
export LD_LIBRARY_PATH="/path/to/install/lib:$LD_LIBRARY_PATH"
export PKG_CONFIG_PATH="/path/to/install/lib/pkgconfig:$PKG_CONFIG_PATH"
export HDF5_USE_FILE_LOCKING=FALSE
export PYTHONPATH="/path/to/install/lib/pythonX.X/site-packages:$PYTHONPATH"
```

Then use IMAS-Core from the required language:
- **Python**: `import imas` (see examples above)
- **C/C++**: Link against `libal.so` with provided headers
- **Fortran**: Use pkg-config to get compiler flags
- **Java**: Use `imas.jar` in your classpath
- **MATLAB**: Add MEX directory to MATLAB path

See [Building from Source](docs/source/user_guide/installation.rst) for detailed build instructions.

## Documentation

- **[User Guide](docs/source/user_guide/index.rst)** - User documentation
- **[Installation Guide](docs/source/user_guide/installation.rst)** - Installation instructions
- **[Backends Guide](docs/source/user_guide/backends_guide.rst)** - Available data backends
- **[URIs Guide](docs/source/user_guide/uris_guide.rst)** - Data entry URI documentation
- **[Configuration](docs/source/user_guide/configuration.rst)** - Configuration options
- **[FAQ](docs/source/faq.rst)** - Frequently asked questions
- **[Troubleshooting](docs/source/troubleshooting.rst)** - Troubleshooting


## Available Backends

IMAS-Core supports these storage backends:

| Backend | Use Case | Remote | File-based |
|---------|----------|--------|-----------|
| **HDF5** | Default, local storage | No | Yes |
| **MDSplus** | ITER experiments | Yes | No |
| **UDA** | Distributed access | Yes | No |
| **Memory** | Testing | No | No |
| **FlexBuffers** | Message passing | No | Yes |
| **ASCII** | Debugging | No | Yes |


**Help**
- See [Troubleshooting Guide](docs/source/troubleshooting.rst)
- Check [FAQ](docs/source/faq.rst)
- Open an [Issue on GitHub](https://github.com/iterorganization/IMAS-Core/issues)


## Links

- **Homepage**: 
- **GitHub**: https://github.com/iterorganization/IMAS-Core
- **PyPI**: https://pypi.org/project/imas-core/
- **Data Dictionary**: https://imas-data-dictionary.readthedocs.io/
- **Issue Tracker**: https://github.com/iterorganization/IMAS-Core/issues

## License

IMAS-Core is released under the [LGPL-3.0 License](LICENSE.txt)

## Support

- **Email**: imas-support@iter.org
- **Documentation**: https://imas-core.readthedocs.io/
- **Issues**: https://github.com/iterorganization/IMAS-Core/issues
