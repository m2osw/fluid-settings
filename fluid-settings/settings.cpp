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
#include    <advgetopt/conf_file.h>
#include    <advgetopt/exception.h>
#include    <advgetopt/validator_integer.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/glob_to_list.h>
#include    <snapdev/join_strings.h>
#include    <snapdev/map_keyset.h>
#include    <snapdev/string_replace_many.h>
#include    <snapdev/tokenize_string.h>


// last include
//
#include    <snapdev/poison.h>



namespace fluid_settings
{


namespace
{


constexpr char const * const g_settings_file = "/var/lib/fluid-settings/settings/settings.conf";
constexpr char const * const g_definitions_path = "/usr/share/fluid-settings/definitions:/var/lib/fluid-settings/definitions";
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
bool settings::load_definitions(std::string paths)
{
    // completely reset the whole table of options
    //
    f_opts = std::make_shared<advgetopt::getopt>(g_options_environment);

    if(!paths.empty())
    {
        paths = ':' + paths;
    }
    paths = g_definitions_path + paths;

    advgetopt::string_list_t list;
    advgetopt::split_string(paths, list, { ":" });
    bool found(false);
    for(auto const & p : list)
    {
        if(load_definition_file(p))
        {
            found = true;
        }
    }
    if(!found)
    {
        SNAP_LOG_WARNING
            << "no fluid-settings definition files found anywhere; fluid-settings will be dormant."
            << SNAP_LOG_SEND;
    }
    return !f_opts->get_options().empty();
}


bool settings::load_definition_file(std::string const & path)
{
    snapdev::glob_to_list<std::list<std::string>> files;
    if(!files.read_path<>(path + '/' + g_definitions_pattern))
    {
        SNAP_LOG_WARNING
            << "no fluid-settings definition files found in \""
            << path
            << "\" (with pattern \""
            << g_definitions_pattern
            << "\")."
            << SNAP_LOG_SEND;
        return false;
    }

    for(auto const & f : files)
    {
        SNAP_LOG_CONFIGURATION
            << "loading fluid-settings definitions from \""
            << f
            << "\"."
            << SNAP_LOG_SEND;

        try
        {
            f_opts->parse_options_from_file(
                      f
                    , 2
                    , std::numeric_limits<int>::max()
                    , true);
        }
        catch(advgetopt::getopt_logic_error const & e)
        {
            SNAP_LOG_SEVERE
                << "the fluid-settings option parser found an invalid parameter: "
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


/** \brief Retrieved the named value.
 *
 * This function searches for a value in the existing settings.
 *
 * The value has to exist for anything to be returned.
 *
 * If the \p all parameter is set to true, then all the defined values
 * are returned. In other words, if you have a default, one or two
 * applications overwritting the value, and ad administrator value,
 * each on of these gets returned. In this case the result is a comma
 * separated list of values. If a value includes a comma, then it
 * will be escaped. Not that the escape only happens if you request
 * all the values.
 *
 * If no value with that name is defined, then the function returns
 * false allowing the caller to react properly. Note that at the moment
 * you cannot know whether the value is declared or just undefined (i.e.
 * whether a definition was properly loaded but the value is not currently
 * set to anything--i.e. a value without a default returns false until
 * set for the first time).
 *
 * To get a list of valid names, use the list_of_options() function.
 *
 * The \p priority parameter is generally set to HIGHEST_PRIORITY. This
 * allows to get what is viewed as the current value for that specific
 * \p name. The priority can also be specified to get the value at that
 * specific priority. For example, the value with priority DEFAULT_PRIORITY
 * defines the default value and the one at ADMINISTRATOR_PRIORITY is
 * the one the administrator is expected to edit even if a value with
 * a higher priority exists.
 *
 * Note that if you specific a priority other than HIGHEST_PRIORITY,
 * then the function may return false even though a value is defined,
 * only no value is defined at the specific priority you passed to the
 * function.
 *
 * \note
 * If \p all is set to true, then the \p priority parameter is ignored.
 *
 * \param[in] name  The name of the value to retrieve.
 * \param[out] result  The variable where the value gets saved.
 * \param[in] priority  The value at that specific priority.
 * \param[in] all  All the values are returned if true.
 *
 * \return true if a value was found, false otherwise.
 */
get_result_t settings::get_value(
      std::string const & name
    , std::string & result
    , priority_t priority
    , bool all)
{
    advgetopt::option_info::pointer_t o(f_opts->get_option(name));
    if(o == nullptr)
    {
        return get_result_t::GET_RESULT_UNKNOWN;
    }

    if(!o->is_defined())
    {
        if(o->has_default())
        {
            result = o->get_default();
            return get_result_t::GET_RESULT_DEFAULT;
        }
        return get_result_t::GET_RESULT_NOT_SET;
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
        return get_result_t::GET_RESULT_ERROR;
    }
    if(it->second.empty())
    {
        // array of value is empty
        //
        if(o->has_default())
        {
            result = o->get_default();
            return get_result_t::GET_RESULT_DEFAULT;
        }
        return get_result_t::GET_RESULT_NOT_SET;
    }

    if(all)
    {
        result.clear();
        for(auto const & v : it->second)
        {
            if(!result.empty())
            {
                result += ',';
            }
            result += snapdev::string_replace_many(
                              v.get_value()
                            , { { ",", "\\," } });
        }
    }
    else if(priority == HIGHEST_PRIORITY)
    {
        // the default is to return the HIGHEST_PRIORITY
        //
        result = it->second.rbegin()->get_value();
    }
    else
    {
        // a specific priority was give, search for that item
        //
        value v;
        timestamp_t const now(timestamp_t::gettime());
        v.set_value(std::string(), priority, now);
        auto vp(it->second.find(v));
        if(vp == it->second.end())
        {
            return get_result_t::GET_RESULT_PRIORITY_NOT_FOUND;
        }
        result = vp->get_value();
    }

    return get_result_t::GET_RESULT_SUCCESS;
}


bool settings::set_value(
      std::string const & name
    , std::string const & new_value
    , int priority
    , timestamp_t const & timestamp)
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


bool settings::reset_setting(
      std::string const & name
    , priority_t priority)
{
    advgetopt::option_info::pointer_t o(f_opts->get_option(name));
    if(o == nullptr)
    {
        return false;
    }

    o->reset();

    auto it(f_values.find(name));
    if(it == f_values.end())
    {
        return false;
    }

    timestamp_t const now(timestamp_t::gettime());
    value v;
    v.set_value(std::string(), priority, now);
    auto vp(it->second.find(v));
    if(vp == it->second.end())
    {
        return false;
    }

    it->second.erase(vp);

    if(it->second.empty())
    {
        f_values.erase(it);
    }

    return true;
}


void settings::load(std::string const & filename)
{
    advgetopt::conf_file_setup setup(filename);
    advgetopt::conf_file::pointer_t data(advgetopt::conf_file::get_conf_file(setup));

    for(auto const & p : data->get_parameters())
    {
        std::vector<std::string> sections;
        snapdev::tokenize_string(sections, p.first, { "::" }, true);
        std::int64_t priority(0);
        advgetopt::validator_integer::convert_string(sections.back(), priority);
        sections.pop_back();
        std::string const & value(p.second.get_value());
        std::string::size_type const pos(value.find('|'));
        if(pos == std::string::npos)
        {
            SNAP_LOG_ERROR
                << "found value \""
                << value
                << "\" in parameter \""
                << p.first
                << "\" without a | to separate the timestamp from the value."
                << SNAP_LOG_SEND;
            continue;
        }
        std::int64_t timestamp_nsec(0);
        advgetopt::validator_integer::convert_string(value.substr(0, pos), timestamp_nsec);

        set_value(
              snapdev::join_strings(sections, "::")
            , value.substr(pos + 1)
            , static_cast<priority_t>(priority)
            , timestamp_nsec);
    }
}


void settings::save(std::string const & filename)
{
    advgetopt::conf_file_setup setup(filename);
    advgetopt::conf_file::pointer_t data(advgetopt::conf_file::get_conf_file(setup));

    // TODO: look into a way to not have to do that erase since it is really
    //       slow (it deletes one parameter at a time and saves all the info)
    //
    //       this happens in part because the configuration files are
    //       cached in memory (for speed) -- deleting the file has not
    //       effect since the cache is always returned at the moment
    //
    data->erase_all_parameters();

    // the default warning is not going to cut it for fluid-settings since
    // it mentions advgetopt instead and that you can safely edit the file
    //
    std::string startup_comment(
        "# WARNING: AUTO-GENERATED FILE, DO NOT EDIT\n"
        "#          see `man fluid-settings` for details\n");

    for(auto const & m : f_values)
    {
        for(auto const & s : m.second)
        {
            std::string const np(std::to_string(s.get_priority()));

            fluid_settings::timestamp_t const t(s.get_timestamp());

            std::string tv(std::to_string(t.to_nsec()));
            tv += FIELD_SEPARATOR;
            tv += s.get_value();

            data->set_parameter(
                  m.first
                , np
                , tv
                , startup_comment);
            startup_comment.clear();
        }
    }

    data->save_configuration(".bak", true, false);
}


std::string settings::serialize_value(std::string const & name)
{
    std::string result;

    auto m(f_values.find(name));
    if(m == f_values.end())
    {
        return result;
    }

    std::string const separator(1, FIELD_SEPARATOR);
    for(auto const & s : m->second)
    {
        result += std::to_string(s.get_priority());
        result += FIELD_SEPARATOR;

        fluid_settings::timestamp_t const t(s.get_timestamp());
        result += std::to_string(t.to_nsec());
        result += FIELD_SEPARATOR;

        // the value may include
        //
        result += snapdev::string_replace_many(
                    s.get_value(),
                    {
                        { separator, "\\P" },
                        { "\\", "\\S" },
                        { "\n", "\\n" },
                        { "\r", "\\r" },
                    });

        result += VALUE_SEPARATOR;
    }

    return result;
}


char const * settings::get_default_settings_filename()
{
    return g_settings_file;
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
