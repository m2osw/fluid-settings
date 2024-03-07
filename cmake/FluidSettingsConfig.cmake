# - Find Fluid Settings
#
# FLUIDSETTINGS_FOUND                   - System has FluidSettings
# FLUIDSETTINGS_INCLUDE_DIRS            - The FluidSettings include directories
# FLUIDSETTINGS_LIBRARIES               - The libraries needed to use FluidSettings
# FLUIDSETTINGS_DEFINITIONS             - Compiler switches required for using FluidSettings
# FLUIDSETTINGS_DEFINITIONS_INSTALL_DIR - Directory where to install FluidSettings defiinitions
#
# License:
#
# Copyright (c) 2011-2024  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/fluid-settings
# contact@m2osw.com
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

find_path(
    FLUIDSETTINGS_INCLUDE_DIR
        fluid-settings/version.h

    PATHS
        ENV FLUIDSETTINGS_INCLUDE_DIR
)

find_library(
    FLUIDSETTINGS_LIBRARY
        fluid-settings

    PATHS
        ${FLUIDSETTINGS_LIBRARY_DIR}
        ENV FLUIDSETTINGS_LIBRARY
)

mark_as_advanced(
    FLUIDSETTINGS_INCLUDE_DIR
    FLUIDSETTINGS_LIBRARY
)

set(FLUIDSETTINGS_INCLUDE_DIRS ${FLUIDSETTINGS_INCLUDE_DIR})
set(FLUIDSETTINGS_LIBRARIES    ${FLUIDSETTINGS_LIBRARY})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
    FluidSettings
    REQUIRED_VARS
        FLUIDSETTINGS_INCLUDE_DIR
        FLUIDSETTINGS_LIBRARY
)

set(FLUIDSETTINGS_DEFINITIONS_INSTALL_DIR share/fluid-settings/definitions)

# vim: ts=4 sw=4 et
