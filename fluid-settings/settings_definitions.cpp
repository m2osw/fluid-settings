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

/** \file
 * \brief This implements the reading of fluid setting definitions.
 *
 * The fluid settings depend on a list of definitions that declare what is
 * valid. In other words, only values that are defined in a fluid setting
 * definition can be defined in the fluid settings (although we can start
 * listening on a value which is  not yet defined).
 *
 * The definitions may appear on any computer on the network. One job of
 * the fluid settings system is to gather all of these definitions on the
 * computers running the fluid settings daemon.
 *
 * This file implements the loading of those definitions to memory. This
 * allows us to then share that information with the other services,
 * including a service such as snapmanager which can display the values
 * to an administrator for editing.
 */

// self
//
#include    "fluid-settings/settings_definitions.h"

#include    "fluid-settings/version.h"


// advgetopt
//
#include    <advgetopt/exception.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/glob_to_list.h>


// last include
//
#include    <snapdev/poison.h>



namespace fluid_settings
{


namespace
{


constexpr char const * const g_definitions_path = "/var/lib/fluid-settings";
constexpr char const * const g_definitions_pattern = "*.ini";


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
constexpr advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "fluid-settings",
    .f_group_name = "ve",
    .f_options = nullptr,
    .f_options_files_directory = g_definitions_path,
    .f_environment_variable_name = nullptr,
    .f_environment_variable_intro = nullptr,
    .f_section_variables_name = nullptr,
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = 0,
    .f_help_header = nullptr,
    .f_help_footer = nullptr,
    .f_version = FLUID_SETTINGS_VERSION_STRING,
    .f_license = nullptr,
    .f_copyright = nullptr,
    .f_build_date = UTC_BUILD_DATE,
    .f_build_time = UTC_BUILD_TIME,
    .f_groups = nullptr,
};
#pragma GCC diagnostic pop


}
// no name namespace



/** \class settings_definitions
 * \brief Load and update the settings definitons.
 *
 * This object is used to load and hold the settings definitions. These
 * definitions are found under `/usr/share/eventdispathcer/fluid-settings/...`.
 *
 * The settings are defined in configuration files which give the name of
 * each field that can be found in the fluid-settings. These definition
 * include a type, a default value, and a few other features.
 */





/** \brief Load the list of files with option definitions.
 *
 * This function reads files that include option definitions.
 *
 * By default, the function loads the files installed under the 
 * "definitions path". You can obtain the default path using the
 * get_default_path() function.
 *
 * \param[in] path  The path to use to read the definitions.
 *
 * \return true if some configuration files were found, false otherwise.
 */
bool settings_definitions::load_definitions(std::string const & path)
{
    f_opts = std::make_shared<advgetopt::getopt>(g_options_environment);

    snapdev::glob_to_list<std::list<std::string>> files;

    if(!files.read_path<>(
              (path.empty() ? std::string(g_definitions_path) : path)
            + '/'
            + g_definitions_pattern))
    {
        SNAP_LOG_ERROR
            << "no fluid-settings definition files found in \""
            << g_definitions_path
            << "\" (with pattern \""
            << g_definitions_pattern
            << "\")."
            << SNAP_LOG_SEND;
        return false;
    }

    for(auto const & f : files)
    {
        try
        {
        f_opts->parse_options_from_file(f, 2, std::numeric_limits<int>::max());
        }
        catch(advgetopt::getopt_logic_error const & e)
        {
            SNAP_LOG_SEVERE
                << "the fluid settings option parser found an invalid parameter: "
                << e.what()
                << SNAP_LOG_SEND;
        }
    }

    return true;
}


/** \brief Retrieve a copy of the path to the settings definitions.
 *
 * By default, all the settings definitions are expected to be saved under
 * this path.
 */
char const * settings_definitions::get_default_path()
{
    return g_definitions_path;
}


} // namespace fluid_settings
// vim: ts=4 sw=4 et
