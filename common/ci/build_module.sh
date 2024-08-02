#!/bin/bash
# Bamboo CI script to build an EB development module
#
# This script expects to be run from the Bamboo build folder with the repository in 
# the `al-common` sub-folder.

# Debuggging:
set -e -o pipefail
echo "Loading modules..."

# Set up environment such that module files can be loaded
. /usr/share/Modules/init/sh

# The default git doesn't understand $Format:%(describe)$
module load git
set -x  # debugging

# Compatibility for al-lowlevel -> al-core rename
if [ ! -d al-core ]; then
    ln -s al-lowlevel al-core
fi

# Export repositories:
for repo in al-common al-core al-cpp al-fortran al-java al-matlab al-python data-dictionary
do
    pushd $repo
    git archive --output ../$repo-dev.tar.gz HEAD
    popd
done
set +x  # disable, to avoid verbose output during `module load`

# Load EB module
module purge
module load EasyBuild
set -x  # debugging


# Populate DD version from branch name:
DD_VERSION=$(cd data-dictionary && git rev-parse --abbrev-ref HEAD)
DD_VERSION=${DD_VERSION/\//}  # Remove '/' so develop/3 becomes develop3
if [[ -z "$DD_VERSION" ]]
then
    echo "Error: Could not determine DD branch name"
    exit 1
fi
# Get the full version used by the DD repository:
# - Full tags are fine (3.41.0 -> 3.41.0)
# - When not on a tag, convert first `-` to `+` and the second `-` to `.`
#   (3.40.1-127-g05454a8 -> 3.40.1+127.g05454a8)
DD_VERSIONEER_VERSION=$(cd data-dictionary && git describe | sed 's/-/+/' | sed 's/-/./')

# Create EB files from templates:
cp */ci/*.eb.in .
for ebfile in *.eb.in
do
    sed -i "s/@DD_VERSIONEER_VERSION@/${DD_VERSIONEER_VERSION}/g" $ebfile
    sed -i "s/@DD_VERSION@/${DD_VERSION}/g" $ebfile
    mv $ebfile ${ebfile/.in/}
done

# Ensure easybuild folder is empty
rm -rf easybuild

# Build modules
mkdir easybuild
EBPREFIX=$(readlink -f easybuild)
EBOPTIONS="--prefix ${EBPREFIX} -l"
# CentOS 8 nodes used EnvironmentModules, RHEL 9 uses Lmod:
if [ -z $LMOD_CMD ]; then
    export EASYBUILD_MODULE_SYNTAX=Tcl EASYBUILD_MODULES_TOOL=EnvironmentModules
else
    export EASYBUILD_MODULES_TOOL=Lmod EASYBUILD_MODULE_SYNTAX=Lua
fi

eb $EBOPTIONS Data-Dictionary-develop-*.eb
eb $EBOPTIONS IMAS-AL-Common-develop.eb
eb $EBOPTIONS IMAS-AL-Core-develop-*.eb
eb $EBOPTIONS IMAS-AL-MDSplus-models-develop-*.eb
eb $EBOPTIONS IMAS-AL-Cpp-develop-*.eb
eb $EBOPTIONS IMAS-AL-Fortran-develop-*.eb
eb $EBOPTIONS IMAS-AL-Java-develop-*.eb
eb $EBOPTIONS IMAS-AL-Matlab-develop-*.eb
eb $EBOPTIONS IMAS-AL-Python-develop-*.eb
eb $EBOPTIONS IMAS-develop-*.eb

# Tar easybuild modules
tar -czvf easybuild.tar.gz easybuild
