// Copyright (c) 2021-2022  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/eventdispatcher
// contact@m2osw.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#pragma once

/** \file
 * \brief Definitions of the loading of the fluid settings definitions.
 *
 * Settings that one can set or get in the fluid settings must all be
 * given a definition. This is very similar to an option in the advgetopt
 * library.
 *
 * The definitions are read from disk. They must be saved under
 * `/usr/share/eventdispatcher/fluid-settings/*.ini`.
 */



namespace fluid_settings
{


class settings_definitions
{
public:
    void                    load_definitions();

private:
    advgetopt::getopt::pointer_t
                            f_opts = advgetopt::getopt::pointer_t();
};


} // fluid_settings namespace
// vim: ts=4 sw=4 et
