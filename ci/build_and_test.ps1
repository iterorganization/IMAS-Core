
IF ($null -ne $env:bamboo_HTTP_AUTH_BEARER_PASSWORD) {
  git config --global http.https://git.iter.org/.extraheader "Authorization: Bearer $env:bamboo_HTTP_AUTH_BEARER_PASSWORD"
}

if (Test-Path 'build') {
  Remove-Item 'build' -Recurse -Force
}

$env:VCPKG_ROOT = Join-Path $pwd ..\vcpkg\
IF (-Not (Test-Path $env:VCPKG_ROOT)) {
  git clone https://github.com/microsoft/vcpkg.git $env:VCPKG_ROOT
  $env:VCPKG_ROOT = Resolve-Path $env:VCPKG_ROOT
}

python -m venv build\pip_install
build\pip_install\Scripts\activate.ps1
python -m pip install --upgrade pip 

$CMAKE_ARGS = @(
  "-DCMAKE_INSTALL_PREFIX=$pwd\build\test-install"
  "-DPython_FIND_VIRTUALENV=ONLY"
  "-DAL_BACKEND_HDF5=ON"
  "-DVCPKG=ON"
  "-DAL_BACKEND_MDSPLUS=OFF"
  "-DAL_BACKEND_UDA=OFF"
  "-DAL_BUILD_MDSPLUS_MODELS=OFF"
  "-DAL_PYTHON_BINDINGS=ON"
  "-DAL_DOWNLOAD_DEPENDENCIES=ON"
  "-DAL_COMMON_GIT_REPOSITORY=https://git.iter.org/scm/imas/al-common.git"
  "-DDD_GIT_REPOSITORY=https://git.iter.org/scm/imas/data-dictionary.git"
  "-DDD_VERSION=master/3"
)

cmake -Bbuild @CMAKE_ARGS --trace-expand
cmake --build build --target install

Get-ChildItem build\test-install -Name -Recurse  | select-string "^(?!.*numpy)"

pip install --find-links=build\dist "imas-core[test,cov]"
pytest --junitxml results.xml --cov imas_core --cov-report xml --cov-report html
coverage2clover -i coverage.xml -o clover.xml
deactivate
