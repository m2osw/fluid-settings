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
#include    "fluid-settings/settings.h"

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
#include    <snapdev/join_strings.h>
#include    <snapdev/map_keyset.h>


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
    .f_group_name = "fluid-settings",
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



/** \class settings
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
 * "default definitions path". You can obtain the default path using the
 * get_default_path() function.
 *
 * \note
 * You can load files in one specific location using this function and
 * only one path as the input string.
 *
 * \param[in] paths  A list of colon separated paths used to read all the
 * available definitions.
 *
 * \return true if some configuration files were found, false otherwise.
 */
bool settings::load_definitions(std::string const & paths)
{
    advgetopt::string_list_t list;
    advgetopt::split_string(paths, list, { ":" });
    bool result(true);
    for(auto const & p : list)
    {
        result = load_file(p) && result;
    }
    return result;
}


bool settings::load_file(std::string const & path)
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

    bool const ignore_duplicates(!f_opts->get_options().empty());
    for(auto const & f : files)
    {
        try
        {
            f_opts->parse_options_from_file(
                      f
                    , 2
                    , std::numeric_limits<int>::max()
                    , ignore_duplicates);
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


/** \brief Retrieve the list of options.
 *
 * This function reads all the option names and return a comma separated
 * list of joined options.
 *
 * Make sure to call the load_definitions() function at least once
 * before.
 *
 * \return The list of options as a string of comma separated names.
 */
std::string settings::list_of_options()
{
    if(f_opts == nullptr)
    {
        return std::string();
    }

    std::set<std::string> options;
    snapdev::map_keyset(options, f_opts->get_options());
    return snapdev::join_strings(options, ",");
}


bool settings::get_value(std::string const & name, std::string & value)
{
    advgetopt::option_info::pointer_t o(f_opts->get_option(name));
    if(o == nullptr)
    {
        return false;
    }

    if(!o->is_defined())
    {
        return false;
    }

    // the value is defined so we can retrieve it, "unfortunately" the
    // advgetopt option_info object does not track priorities; there we
    // instead save the latest which happens to be the values from the
    // configuration file with the highest priority...
    //
    // in fluid-settings we have to have our own table because we want
    // to save all the values with all of their priorities so here we
    // do the necessary to retrieve the value with the highest priority
    //
    auto it(f_values.find(name));
    if(it == f_values.end())
    {
        // weird, if o->is_defined() is true then we should have found this!?
        //
        return false;
    }
    if(it->second.empty())
    {
        return false;
    }

    value = it->second.rbegin()->get_value();

    return true;
}


bool settings::set_value(
      std::string const & name
    , std::string const & new_value
    , int priority
    , snapdev::timespec_ex const & timestamp)
{
    advgetopt::option_info::pointer_t o(f_opts->get_option(name));
    if(o == nullptr)
    {
        return false;
    }

    o->set_value(
              0
            , new_value
            , advgetopt::option_source_t::SOURCE_DYNAMIC);
    if(!o->is_defined())
    {
        return false;
    }

    value v;
    v.set_value(new_value, priority, timestamp);

    auto it(f_values.find(name));
    if(it == f_values.end())
    {
        // no such value yet, just save that value_priority as is
        //
        f_values[name].insert(v);
    }
    else
    {
        auto vp(it->second.find(v));
        if(vp == it->second.end())
        {
            // not there yet, just insert
            //
            it->second.insert(v);
        }
        else if(timestamp > vp->get_timestamp())
        {
            // it was already there, but message value is more recent
            // than stored value so keep it
            //
            it->second.insert(v);
        }
    }

    return true;
}


void settings::reset_setting(std::string const & name, int priority)
{
    advgetopt::option_info::pointer_t o(f_opts->get_option(name));
    if(o == nullptr)
    {
        return;
    }

    o->reset();

    auto it(f_values.find(name));
    if(it == f_values.end())
    {
        return;
    }

    snapdev::timespec_ex timestamp;
    value v;
    v.set_value("ignore", priority, timestamp);
    auto vp(it->second.find(v));
    if(vp == it->second.end())
    {
        return;
    }

    it->second.erase(vp);

    if(it->second.empty())
    {
        f_values.erase(it);
    }
}


/** \brief Retrieve a copy of the path to the settings definitions.
 *
 * By default, all the settings definitions are expected to be saved under
 * this path.
 */
char const * settings::get_default_path()
{
    return g_definitions_path;
}


} // namespace fluid_settings
// vim: ts=4 sw=4 et
