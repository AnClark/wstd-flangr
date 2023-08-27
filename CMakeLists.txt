#  WSTD_FLANGR CMakeLists.txt
#
#  Copyright (C) 2023 AnClark Liu
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

cmake_minimum_required (VERSION 3.20)

project (WSTD_FLANGR_BUILD
    VERSION 0.2.1
)

#
# Environment configurations
#

set (VENV_DIR ${PROJECT_BINARY_DIR}/python-venv-workdir)
set (VENV_PYTHON_EXECUTABLE ${VENV_DIR}/bin/python)
set (HVCC_OUTPUT_DIR ${PROJECT_BINARY_DIR}/WSTD_FLANGR)

set (HVCC_VERSION 424308377a8b0e4291e0bfda3bcf68ae9fd88f33)

#
# Rules stamp files
#

set (CMD_CREATE_VENV ${PROJECT_BINARY_DIR}/cmd-create-python-venv.stamp)
set (CMD_INSTALL_HVCC ${PROJECT_BINARY_DIR}/cmd-install-hvcc.stamp)

#
# Check for Python 3.x and VirtualEnv
#

find_package (Python3 COMPONENTS Interpreter)

if (NOT Python3_Interpreter_FOUND)
    message (FATAL_ERROR "WSTD_FLANGR requires Python 3.x to build!")
endif ()

execute_process (COMMAND ${Python3_EXECUTABLE} -m virtualenv --version OUTPUT_VARIABLE VENV_VERSION)

if (VENV_VERSION)
    message (STATUS "Found Python VirtualEnv: ${VENV_VERSION}")
else ()
    message (FATAL_ERROR "Python VirtualEnv is not installed! This is essential for WSTD_FLANGR's build environment.")
endif ()

#
# Setup HVCC environment
#

# Setup Python Virtual Environment
add_custom_command (OUTPUT ${CMD_CREATE_VENV}
    COMMAND ${Python3_EXECUTABLE} -m virtualenv ${VENV_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch ${CMD_CREATE_VENV}
    DEPENDS Python3::Interpreter
)

# Install HVCC from Git submodule
add_custom_command (OUTPUT ${CMD_INSTALL_HVCC}
    #COMMAND ${VENV_PYTHON_EXECUTABLE} -m pip install git+https://github.com/Wasted-Audio/hvcc.git@${HVCC_VERSION}
    COMMAND ${VENV_PYTHON_EXECUTABLE} -m pip install ${PROJECT_SOURCE_DIR}/dep/hvcc
    COMMAND ${CMAKE_COMMAND} -E touch ${CMD_INSTALL_HVCC}
    DEPENDS ${CMD_CREATE_VENV}
)

#
# Generate plugin source code with HVCC
#
# We should generate sources every time, to keep generated code up-to-date when there are modifications.
# HVCC runs extremely fast!
#

add_custom_target (cmd_hvcc_build
    COMMAND ${VENV_PYTHON_EXECUTABLE} ${VENV_DIR}/bin/hvcc wstd-flangr.pd -n WSTD_FLANGR -m wstd-flangr.json -o ${HVCC_OUTPUT_DIR} -g dpf -p dep/heavylib/ dep/ --copyright \"Copyright (c) Wasted Audio 2023 - GPL-3.0-or-later\"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different override/* ${HVCC_OUTPUT_DIR}/plugin/source/
    DEPENDS ${CMD_INSTALL_HVCC}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

#
# Generate CMakeLists.txt for WSTD-Flangr plugin
#

set (DPF_DIR ${PROJECT_SOURCE_DIR}/dep/dpf)
set (DPF_WIDGETS_DIR ${PROJECT_SOURCE_DIR}/dep/dpf-widgets)
set (WSTD_CLRS_DIR ${PROJECT_SOURCE_DIR}/dep/wstd.clrs)

# HVCC will remove output directory before generation.
# So we have to generate CMakeLists.txt elsewhere, then copy it when needed.
set (GENERATED_CMAKE_FILE ${PROJECT_BINARY_DIR}/CMakeLists.wstd-flangr.txt)
configure_file (CMakeLists.wstd-flangr.txt.in ${GENERATED_CMAKE_FILE} @ONLY)

add_custom_target (cmd_copy_generated_cmake_file
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GENERATED_CMAKE_FILE} ${HVCC_OUTPUT_DIR}/CMakeLists.txt
    DEPENDS cmd_hvcc_build
)

#
# Build WSTD-Flangr plugin
#
# Here I use the powerful ExternalProject to build the final generated plugin code. 
#

include (ExternalProject)

# ExternalProjects will complain about empty source directory. But at this time, plugin source is not generated yet.
# So, we trick it :-)
make_directory (${HVCC_OUTPUT_DIR})
file (TOUCH ${HVCC_OUTPUT_DIR}/dummy.txt)

ExternalProject_Add (build_wstd_flangr
    SOURCE_DIR ${HVCC_OUTPUT_DIR}
    DEPENDS cmd_hvcc_build cmd_copy_generated_cmake_file

    CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

    INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Plugin built successfully. Copying files..."
    COMMAND ${CMAKE_COMMAND} -E copy_directory "<BINARY_DIR>/bin" "${PROJECT_BINARY_DIR}/bin"
    COMMAND ${CMAKE_COMMAND} -E echo "Done! Plugin files has been written to: ${PROJECT_BINARY_DIR}/bin"

    BUILD_ALWAYS ON     # Must switch it on, otherwise plugin will only build once!
)
