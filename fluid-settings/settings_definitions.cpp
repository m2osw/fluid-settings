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

/** \file
 * \brief The version of the fluid settings library at compile time.
 *
 * This file records the fluid settings library version at
 * compile time.
 *
 * The `#define`s give you the library version at the time you are compiling.
 * The functions allow you to retrieve the version of a dynamically linked
 * library.
 */

// self
//
#include    "eventdispatcher/fluid-settings/settings_definitions.h"

#include    "eventdispatcher/fluid-settings/version.h"


// last include
//
#include    <snapdev/poison.h>



namespace fluid_settings
{


namespace
{


constexpr char const * const g_definitions_path = "/var/lib/eventdispatcher/fluid-settings";
constexpr char const * const g_definitions_pattern = "*.ini";


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
constexpr options_environment const g_options_environment =
{
    .f_project_name = "fluid-settings",
    .f_group_name = "ve",
    .f_options = nullptr,
    .f_options_files_directory = g_definitions_path,
    .f_environment_variable_name = nullptr,
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



/** \brief Load and update the settings definitons.
 *
 * This object is used to load and hold the settings definitions. These
 * definitions are found under `/usr/share/eventdispathcer/fluid-settings/...`.
 *
 * The settings are defined in configuration files which give the name of
 * each field that can be found in the fluid-settings. These definition
 * include a type, a default value, and a few other features.
 */
settings_definitions::settings_definitions()
{
}


/** \brief Load the list of files with option definitions.
 *
 * This file includes option definitions.
 *
 * \return true if some configuration files were found, false otherwise.
 */
bool load_definitions::load_definitions()
{
    f_opts = std::make_shared<getopt>(g_options_environment);

    snap::glob_to_list<std::list> files;

    files.read_path<>(
              std::string(g_definitions_path)
            + '/'
            + g_definitions_pattern);

    for(auto const & f : files)
    {
        f_opts->parse_options_from_file(f, 2, std::numeric_limits<int>::max());

//        advgetopt::conf_file_setup setup(f);
//        advgetopt::conf_file::pointer_t const config(advgetopt::conf_file::get_conf_file(setup);
//        advgetopt::conf_file::sections_t const sections(config.get_sections());
//
//        for(auto const & s : sections)
//        {
//            std::list<std::string> section_names;
//            snap::tokenize_string(
//                  section_names
//                , s
//                , "::");
//            if(section_names.size() < 2)
//            {
//                SNAP_LOG_RECOVERABLE_ERROR
//                    << "the name of a settings definition must include a namespace; \""
//                    << s
//                    << "\" is not considered valid."
//                    << SNAP_LOG_SEND;
//                continue;
//            }
//
//            // default
//            //
//            std::string default_str;
//            char * default_value(nullptr);
//            std::string const default_name(s + "::default");
//            if(config.has_parameter(default_name))
//            {
//                default_str = config.get_parameter(default_name);
//                default_value = default_str.c_str();
//            }
//
//            // description
//            //
//            std::string description_str;
//            char * description(nullptr);
//            std::string const description_name(s + "::description");
//            if(config.has_parameter(description_name))
//            {
//                description_str = config.get_parameter(description_name);
//                description = description_str.c_str();
//            }
//
//            // type
//            //
//            std::string type_str;
//            char * type(nullptr);
//            std::string const type_name(s + "::type");
//            if(config.has_parameter(type_name))
//            {
//                type_str = config.get_parameter(type_name);
//                type = type_str.c_str();
//            }
//
//            advgetopt::option dynamic_option[2] = {
//                define_option(
//                      advgetopt::Name(s)
//                    , advgetopt::Flags<advgetopt::GETOPT_FLAG_DYNAMIC_CONFIGURATION
//                                     , advgetopt::GETOPT_FLAG_REQUIRED
//                                     , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>()
//                    , advgetopt::DefaultValue(default_value)
//                    , advgetopt::Help(description)
//                    , advgetopt::Validator(type)
//                ),
//                end_options()
//            };
//
//            f_opts.parse_options_info(dynamic_option, false);
        }
    }
}


/** \brief Get the patch version of the library.
 *
 * This function returns the patch version of the running library
 * (the one you are linked against at runtime).
 *
 * \return The patch version.
 */
int get_patch_version()
{
    return FLUID_SETTINGS_VERSION_PATCH;
}


/** \brief Get the full version of the library as a string.
 *
 * This function returns the major, minor, and patch versions of the
 * running library (the one you are linked against at runtime) in the
 * form of a string.
 *
 * The build version is not made available. In most cases we change
 * the build version only to run a new build, so not code will have
 * changed (some documentation and non-code files may changed between
 * build versions; but the code will work exactly the same way.)
 *
 * \return The library version.
 */
char const * get_version_string()
{
    return FLUID_SETTINGS_VERSION_STRING;
}


} // fluid_settings namespace
// vim: ts=4 sw=4 et
