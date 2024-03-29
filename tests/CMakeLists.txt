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
## eventdispatcher unittest
##
project(unittest)

find_package(SnapCatch2)

if(SnapCatch2_FOUND)

    add_executable(${PROJECT_NAME}
        catch_main.cpp

        catch_fluid_definitions.cpp
        catch_version.cpp
    )

    target_include_directories(${PROJECT_NAME}
        PUBLIC
            ${SNAPCATCH2_INCLUDE_DIRS}
            ${LIBEXCEPT_INCLUDE_DIRS}
    )

    target_link_libraries(${PROJECT_NAME}
        fluid-settings
        ${LIBEXCEPT_LIBRARIES}
        ${SNAPCATCH2_LIBRARIES}
    )

    add_test(unittests ${PROJECT_NAME})

else(SnapCatch2_FOUND)

    message("snapcatch2 not found... no test will be built.")

endif(SnapCatch2_FOUND)

# vim: ts=4 sw=4 et
