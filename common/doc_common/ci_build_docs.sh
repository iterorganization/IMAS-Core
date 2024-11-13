#!/bin/bash
# ITER Bamboo CI (ci.iter.org) script to build sphinx documentation for all HLIs

# Script assumes that the pwd is the access layer git root directory (i.e. the
# parent directory of where this script is located).

# Quit with error when any command is unsuccessful
set -e

# Set up environment such that module files can be loaded
if test -f /etc/profile.d/modules.sh ;then
. /etc/profile.d/modules.sh
else
. /usr/share/Modules/init/sh
fi

# Make Python available
module purge
module load Python/3.8.6-GCCcore-10.2.0

# Create and activate a venv
rm -rf docs_venv
python -m venv docs_venv
. docs_venv/bin/activate

# Install dependencies
pip install --upgrade pip
pip install -r doc_common/requirements.txt

# Output all installed packages
pip list -v

# Update the ##VERSION## string in the deploy script
if test `git describe` = `git describe --abbrev=0`; then
    # Strip patch version from the release tag, e.g. 5.0.1 -> 5.0
    VERSION=`git describe | cut -d. -f1-2`
else
    VERSION=dev
fi
sed -i "s/##VERSION##/$VERSION/g" doc_common/deploy.ps1

# Instruct sphinx to treat all warnings as errors
export SPHINXOPTS='-W --keep-going'

# Make all the docs
# We're not using `make docs`, because some HLI Makefiles
# won't do anything unless the right environment parameters exist
make -C cppinterface/doc html
make -C fortraninterface/doc html
make -C javainterface/doc html
make -C mexinterface/doc html
make -C pythoninterface/doc html
