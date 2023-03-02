# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../common/components")

# COMMAND python "../tools/embeddedgen.py" -i ${CMAKE_SOURCE_DIR}/../main/www -o ${CMAKE_SOURCE_DIR}/../main

project(base-fw)
