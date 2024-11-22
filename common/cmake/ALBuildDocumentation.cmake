# CMake config for building Sphinx docs
find_package( Python3 REQUIRED )

file( GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/doc.sh"
    CONTENT "#!/bin/bash
# Link al-common/doc_common folder
ln -sf '${al-common_SOURCE_DIR}/doc_common' '${CMAKE_CURRENT_SOURCE_DIR}/doc/'
# Link plugins folder
ln -sfT '${al-plugins_SOURCE_DIR}/' '${CMAKE_CURRENT_SOURCE_DIR}/doc/plugins'

# Set up python venv
${Python3_EXECUTABLE} -m venv --no-site-package '${CMAKE_CURRENT_BINARY_DIR}/doc_venv'
source '${CMAKE_CURRENT_BINARY_DIR}/doc_venv/bin/activate'
pip install -r --upgrade '${CMAKE_CURRENT_SOURCE_DIR}/doc/doc_common/requirements.txt'

# Instruct sphinx to treat all warnings as errors
export SPHINXOPTS='-W --keep-going'
# Build HTML documentation
make -C '${CMAKE_CURRENT_SOURCE_DIR}/doc' clean html
# Add a version file:
echo ${FULL_VERSION} > ${CMAKE_CURRENT_SOURCE_DIR}/doc/_build/html/version.txt
"
)

add_custom_target( "${PROJECT_NAME}-docs" ALL
    COMMAND bash "${CMAKE_CURRENT_BINARY_DIR}/doc.sh"
)
