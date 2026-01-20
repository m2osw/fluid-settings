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


##
## Install various configuration files
##
install(
    FILES
        fluid_settings.service

    DESTINATION
        ${COMMUNICATOR_SERVICES_INSTALL_DIR}
)

# fluid-settings connection (a.k.a. client) definitions for fluid-settings
install(
    FILES
        fluid-settings.ini

    DESTINATION
        # See ${FLUIDSETTINGS_DEFINITIONS_INSTALL_DIR} in cmake/FluidSettingsConfig.cmake
        share/fluid-settings/definitions
)

# vim: ts=4 sw=4 et
