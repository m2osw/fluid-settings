# Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
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

AtomicNames(names.an)

##
## fluid settings library
##
project(fluid-settings)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

add_library(${PROJECT_NAME} SHARED
    fluid_settings_connection.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/names.cpp
    settings.cpp
    value.cpp
    version.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${ADVGETOPT_INCLUDE_DIRS}
        ${COMMUNICATORD_INCLUDE_DIRS}
        ${EVENTDISPATCHER_INCLUDE_DIRS}
        ${LIBADDR_INCLUDE_DIRS}
        ${LIBEXCEPT_INCLUDE_DIRS}
        ${SNAPLOGGER_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    ${ADVGETOPT_LIBRARIES}
    ${COMMUNICATORD_LIBRARIES}
    ${EVENTDISPATCHER_LIBRARIES}
    ${LIBADDR_LIBRARIES}
    ${LIBEXCEPT_LIBRARIES}
    ${SNAPLOGGER_LIBRARIES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION
        ${FLUID_SETTINGS_VERSION_MAJOR}.${FLUID_SETTINGS_VERSION_MINOR}

    SOVERSION
        ${FLUID_SETTINGS_VERSION_MAJOR}
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        bin

    LIBRARY DESTINATION
        lib

    ARCHIVE DESTINATION
        lib
)


install(
    FILES
        exception.h
        fluid_settings_connection.h
        ${CMAKE_CURRENT_BINARY_DIR}/names.h
        settings.h
        value.h
        ${CMAKE_CURRENT_BINARY_DIR}/version.h

    DESTINATION
        include/fluid-settings
)



# vim: ts=4 sw=4 et
