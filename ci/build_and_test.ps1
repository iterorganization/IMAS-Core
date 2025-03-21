
IF ($null -ne $env:bamboo_HTTP_AUTH_BEARER_PASSWORD) {
  git config --global http.https://git.iter.org/.extraheader "Authorization: Bearer $env:bamboo_HTTP_AUTH_BEARER_PASSWORD"
}

# Default DD version unless specified in the plan
$DD_VERSION = $DD_VERSION -or "main"

if (Test-Path 'build') {
  Remove-Item 'build' -Recurse -Force
}

$env:VCPKG_ROOT = Join-Path $pwd ..\vcpkg\
IF (-Not (Test-Path $env:VCPKG_ROOT)) {
  git clone https://github.com/microsoft/vcpkg.git $env:VCPKG_ROOT
  & $env:VCPKG_ROOT\bootstrap-vcpkg.bat -disableMetrics
}

python -m venv build\pip_install
build\pip_install\Scripts\activate.ps1
python -m pip install --upgrade pip 

# Retrieve variables from the environment or use default values
$CMAKE_ARGS = @(
  "-DCMAKE_INSTALL_PREFIX=$pwd\build\test-install"
  "-DPython_FIND_VIRTUALENV=ONLY"
  "-DAL_BACKEND_HDF5=$(${env:AL_BACKEND_HDF5:-ON})"
  "-DVCPKG=$(${env:VCPKG:-ON})"
  "-DAL_BACKEND_MDSPLUS=$(${env:AL_BACKEND_MDSPLUS:-OFF})"
  "-DAL_BACKEND_UDA=$(${env:AL_BACKEND_UDA:-OFF})"
  "-DAL_BUILD_MDSPLUS_MODELS=$(${env:AL_BUILD_MDSPLUS_MODELS:-OFF})"
  "-DAL_PYTHON_BINDINGS=$(${env:AL_PYTHON_BINDINGS:-ON})"
  "-DAL_DOWNLOAD_DEPENDENCIES=$(${env:AL_DOWNLOAD_DEPENDENCIES:-ON})"
  "-DDD_GIT_REPOSITORY=$(${env:DD_GIT_REPOSITORY:-https://git.iter.org/scm/imas/data-dictionary.git})"
  "-DDD_VERSION=$(${env:DD_VERSION:-$DD_VERSION})"
)

cmake -Bbuild @CMAKE_ARGS
cmake --build build --target install

Get-ChildItem build\test-install -Name -Recurse  | select-string "^(?!.*numpy)"

pip install --find-links=build\dist "imas-core[test,cov]"
pytest --junitxml results.xml --cov imas_core --cov-report xml --cov-report html
coverage2clover -i coverage.xml -o clover.xml
deactivate
