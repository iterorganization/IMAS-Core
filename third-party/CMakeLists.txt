cmake_minimum_required(VERSION 3.11)

project(third-party)

include(ExternalProject)

ExternalProject_Add(
  uda
  GIT_REPOSITORY https://github.com/ukaea/uda.git
  GIT_TAG "2.7.6"
#   BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/UDA/build
#   INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/UDA/install
)
