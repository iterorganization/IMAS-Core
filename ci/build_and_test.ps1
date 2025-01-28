
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
$AL_BACKEND_HDF5 = if ($env:AL_BACKEND_HDF5) { $env:AL_BACKEND_HDF5 } else { "ON" }
$VCPKG = if ($env:VCPKG) { $env:VCPKG } else { "ON" }
$AL_BACKEND_MDSPLUS = if ($env:AL_BACKEND_MDSPLUS) { $env:AL_BACKEND_MDSPLUS } else { "OFF" }
$AL_BACKEND_UDA = if ($env:AL_BACKEND_UDA) { $env:AL_BACKEND_UDA } else { "OFF" }
$AL_BUILD_MDSPLUS_MODELS = if ($env:AL_BUILD_MDSPLUS_MODELS) { $env:AL_BUILD_MDSPLUS_MODELS } else { "OFF" }
$AL_PYTHON_BINDINGS = if ($env:AL_PYTHON_BINDINGS) { $env:AL_PYTHON_BINDINGS } else { "ON" }
$AL_DOWNLOAD_DEPENDENCIES = if ($env:AL_DOWNLOAD_DEPENDENCIES) { $env:AL_DOWNLOAD_DEPENDENCIES } else { "ON" }
$DD_GIT_REPOSITORY = if ($env:DD_GIT_REPOSITORY) { $env:DD_GIT_REPOSITORY } else { "git@github.com:ITEROrganization/IMAS-Data-Dictionary.git" }
$DD_VERSION = if ($env:DD_VERSION) { $env:DD_VERSION } else { "main" }
$CMAKE_CXX_STANDARD = if ($env:CMAKE_CXX_STANDARD) { $env:CMAKE_CXX_STANDARD } else { 17 }

$CMAKE_ARGS = @(
  "-DCMAKE_INSTALL_PREFIX=$pwd\build\test-install"
  "-DPython_FIND_VIRTUALENV=ONLY"
  "-DAL_BACKEND_HDF5=$AL_BACKEND_HDF5"
  "-DVCPKG=$VCPKG"
  "-DAL_BACKEND_MDSPLUS=$AL_BACKEND_MDSPLUS"
  "-DAL_BACKEND_UDA=$AL_BACKEND_UDA"
  "-DAL_BUILD_MDSPLUS_MODELS=$AL_BUILD_MDSPLUS_MODELS"
  "-DAL_PYTHON_BINDINGS=$AL_PYTHON_BINDINGS"
  "-DAL_DOWNLOAD_DEPENDENCIES=$AL_DOWNLOAD_DEPENDENCIES"
  "-DDD_GIT_REPOSITORY=$DD_GIT_REPOSITORY"
  "-DDD_VERSION=$DD_VERSION"
  "-DCMAKE_CXX_STANDARD=$CMAKE_CXX_STANDARD}
)

cmake -Bbuild @CMAKE_ARGS
cmake --build build --target install

Get-ChildItem build\test-install -Name -Recurse  | select-string "^(?!.*numpy)"

pip install --find-links=build\dist "imas-core[test,cov]"
pytest --junitxml results.xml --cov imas_core --cov-report xml --cov-report html
coverage2clover -i coverage.xml -o clover.xml
deactivate
