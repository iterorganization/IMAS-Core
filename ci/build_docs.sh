#!/bin/bash
# ITER Bamboo CI (ci.iter.org) script to build sphinx documentation

# Quit with error when any command is unsuccessful
# set -e

# Set up environment such that module files can be loaded
if test -f /etc/profile.d/modules.sh ;then
. /etc/profile.d/modules.sh
else
. /usr/share/Modules/init/sh
fi

# Make Python available
module purge
module load Python
module load Doxygen

# Create and activate a venv
rm -rf docs_venv
python -m venv docs_venv
. docs_venv/bin/activate

# Install dependencies
pip install --upgrade pip
pip install -r docs/requirements.txt

# Output all installed packages
pip list -v

# Update the ##VERSION## string in the deploy script
if test `git describe` = `git describe --abbrev=0`; then
    # Strip patch version from the release tag, e.g. 5.0.1 -> 5.0
    VERSION=`git describe | cut -d. -f1-2`
else
    VERSION=dev
fi

# Instruct sphinx to treat all warnings as errors, except for api docs generation
export SPHINXOPTS='-n --keep-going'
export SPHINX_APIDOC_OPTIONS='--no-warnings'
make -C docs clean
make -C docs html 
# Make all the docs
# We're not using `make docs`, because some HLI Makefiles
# won't do anything unless the right environment parameters exist

