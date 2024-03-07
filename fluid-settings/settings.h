// Copyright (c) 2011-2024  Made to Order Software Corp.  All Rights Reserved
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


constexpr char const * const g_settings_file = "/var/lib/fluid-settings/settings/settings.conf";
constexpr char const * const g_definitions_path = "/usr/share/fluid-settings/definitions:/var/lib/fluid-settings/definitions";
constexpr char const * const g_definitions_pattern = "*.ini";


enum class get_result_t
{
    GET_RESULT_ERROR,               // some error happened (other than value undefined)
    GET_RESULT_UNKNOWN,             // unknown value (name not found in lists)
    GET_RESULT_NOT_SET,             // the get "failed" because the value is not set
    GET_RESULT_PRIORITY_NOT_FOUND,  // some values are set, but not at the requested priority
    GET_RESULT_DEFAULT,             // default value is being returned
    GET_RESULT_SUCCESS,             // the value(s) is(are) being returned
};

enum class set_result_t
{
    SET_RESULT_ERROR,               // some error happened
    SET_RESULT_UNKNOWN,             // the named was not found in the existing values
    SET_RESULT_NEW,                 // that value was not set yet
    SET_RESULT_NEW_PRIORITY,        // the value existed, but not at that priority
    SET_RESULT_CHANGED,             // the value was changed
    SET_RESULT_NEWER,               // the timestamp changed, the value is the same
    SET_RESULT_UNCHANGED,           // the timestamp is older or the same
};


class settings
{
public:
    static constexpr char const     FIELD_SEPARATOR = '|';
    static constexpr char const     VALUE_SEPARATOR = '\n';

    bool                    load_definitions(
                                  std::string paths = std::string());
    std::string             list_of_options();
    get_result_t            get_default_value(
                                  std::string name
                                , std::string & result);
    get_result_t            get_value(
                                  std::string name
                                , std::string & value
                                , priority_t priority = HIGHEST_PRIORITY
                                , bool all = false);
    set_result_t            set_value(
                                  std::string name
                                , std::string const & value
                                , int priority
                                , snapdev::timespec_ex const & timestamp);
    bool                    reset_setting(
                                  std::string name
                                , int priority);
    void                    load(std::string const & filename);
    void                    save(std::string const & filename);
    std::string             serialize_value(std::string name);
    void                    unserialize_values(
                                  std::string const & name
                                , std::string const & value);

    static char const *     get_default_settings_filename();
    static char const *     get_default_path();

private:
    bool                    load_definition_file(std::string const & path);

    advgetopt::getopt::pointer_t
                            f_opts = advgetopt::getopt::pointer_t();
    value::map_t            f_values = value::map_t();
};


} // fluid_settings namespace
// vim: ts=4 sw=4 et
