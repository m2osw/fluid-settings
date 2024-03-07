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

find_package(Qt5Core)
find_package(Qt5Widgets)
find_package(EventDispatcherQt)

if(Qt5Core_FOUND AND Qt5Widgets_FOUND AND EventDispatcherQt_FOUND)

    set(CMAKE_AUTOMOC TRUE)

    ##
    ## fluid settings graphical user interface
    ##
    project(fluid-settings-gui)

    qt5_wrap_ui(
        USER_INTERFACE_HEADERS
            fluid_settings.ui
    )

    qt5_add_resources(
        RESOURCE_FILES
            resources.qrc
    )

    add_executable(${PROJECT_NAME}
        ${USER_INTERFACE_HEADERS}
        ${RESOURCE_FILES}

        fluid_window.cpp
        main.cpp
        resources.qrc
    )

    target_include_directories(${PROJECT_NAME}
        PUBLIC
            ${PROJECT_BINARY_DIR}
            ${ADVGETOPT_INCLUDE_DIRS}
            ${COMMUNICATORD_INCLUDE_DIRS}
            ${EVENTDISPATCHER_INCLUDE_DIRS}
            ${LIBADDR_INCLUDE_DIRS}
            ${LIBEXCEPT_INCLUDE_DIRS}
            ${Qt5Core_INCLUDE_DIRS}
            ${Qt5Widgets_INCLUDE_DIRS}
            ${SNAPLOGGER_INCLUDE_DIRS}
    )

    target_link_libraries(${PROJECT_NAME}
        fluid-settings
        ${ADVGETOPT_LIBRARIES}
        ${COMMUNICATORD_LIBRARIES}
        ${EVENTDISPATCHER_LIBRARIES}
        ${EVENTDISPATCHER_QT_LIBRARIES}
        ${LIBADDR_LIBRARIES}
        ${LIBEXCEPT_LIBRARIES}
        ${SNAPLOGGER_LIBRARIES}

        Qt5::Core
        Qt5::Widgets
    )

    install(
        TARGETS
            ${PROJECT_NAME}

        RUNTIME DESTINATION
            bin
    )

else()
    message("Qt not found. The GUI will not be built.")
endif()

# vim: ts=4 sw=4 et
