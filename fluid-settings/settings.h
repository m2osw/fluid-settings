// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/fluid-settings
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
#pragma once

/** \file
 * \brief Definitions of the loading of the fluid settings definitions.
 *
 * Settings that one can set or get in the fluid settings must all be
 * given a definition. This is very similar to an option in the advgetopt
 * library.
 *
 * The definitions are read from disk. They must be saved under
 * `/usr/share/eventdispatcher/fluid-settings/\*.ini`.
 */

// self
//
#include    "value.h"


// advgetopt
//
#include    <advgetopt/advgetopt.h>



namespace fluid_settings
{


class settings
{
public:
    bool                    load_definitions(
                                  std::string paths = std::string());
    std::string             list_of_options();
    bool                    get_value(
                                  std::string const & name
                                , std::string & value
                                , priority_t priority = HIGHEST_PRIORITY
                                , bool all = false);
    bool                    set_value(
                                  std::string const & name
                                , std::string const & value
                                , int priority
                                , snapdev::timespec_ex const & timestamp);
    bool                    reset_setting(
                                  std::string const & name
                                , int priority);
    void                    load(std::string const & filename);
    void                    save(std::string const & filename);

    static char const *     get_default_settings_filename();
    static char const *     get_default_path();

private:
    bool                    load_file(std::string const & path);

    advgetopt::getopt::pointer_t
                            f_opts = advgetopt::getopt::pointer_t();
    value::map_t            f_values = value::map_t();
};


} // fluid_settings namespace
// vim: ts=4 sw=4 et
