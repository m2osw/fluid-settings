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
#include    "server.h"

#include    "messenger.h"


// fluid-settings
//
#include    <fluid-settings/version.h>


// advgetopt
//
#include    <advgetopt/exception.h>


// libaddr
//
#include    <libaddr/addr_parser.h>


// snaplogger
//
#include    <snaplogger/options.h>


// snapdev
//
#include    <snapdev/join_strings.h>
#include    <snapdev/map_keyset.h>


// boost
//
#include    <boost/preprocessor/stringize.hpp>


// last include
//
#include    <snapdev/poison.h>



namespace fluid_settings_daemon
{


namespace
{


advgetopt::option const g_options[] =
{
    advgetopt::define_option(
          advgetopt::Name("snapcommunicator")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::DefaultValue("127.0.0.1:4050")
        , advgetopt::Help("set the snapcommunicator IP:port to connect to.")
    ),
    advgetopt::define_option(
          advgetopt::Name("verbose")
        , advgetopt::Flags(advgetopt::standalone_all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("show what is happening inside the fluid-settings daemon.")
    ),
    advgetopt::end_options()
};


advgetopt::group_description const g_group_descriptions[] =
{
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_COMMANDS)
        , advgetopt::GroupName("command")
        , advgetopt::GroupDescription("Commands:")
    ),
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_OPTIONS)
        , advgetopt::GroupName("option")
        , advgetopt::GroupDescription("Options:")
    ),
    advgetopt::end_groups()
};



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
constexpr advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "fluid-settings-daemon",
    .f_group_name = "fluid-settings",
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "FLUID_SETTINGS_DAEMON",
    .f_environment_variable_intro = nullptr,
    .f_section_variables_name = nullptr,
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>] <settings-definitions filename>\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = FLUID_SETTINGS_VERSION_STRING,
    .f_license = "GNU GPL v3",
    .f_copyright = "Copyright (c) 2022-"
                   BOOST_PP_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    .f_build_date = UTC_BUILD_DATE,
    .f_build_time = UTC_BUILD_TIME,
    .f_groups = g_group_descriptions,
};
#pragma GCC diagnostic pop


}
// no name namespace



server::server(int argc, char * argv[])
    : f_opts(g_options_environment)
    , f_communicator(ed::communicator::instance())
{
    snaplogger::add_logger_options(f_opts);
    f_opts.finish_parsing(argc, argv);
    if(!snaplogger::process_logger_options(f_opts, "/etc/snapcommunicator/logger"))
    {
        // exit on any error
        throw advgetopt::getopt_exit("logger options generated an error.", 1);
    }

    f_address = addr::string_to_addr(
                          f_opts.get_string("snapcommunicator")
                        , "127.0.0.1"
                        , 4050
                        , "tcp");
}


int server::run()
{
    f_messenger = std::make_shared<messenger>(this, f_address);
    f_communicator->add_connection(f_messenger);

    f_communicator->run();

    return 0;
}


bool server::listen(
      std::string const & server_name
    , std::string const & service_name
    , std::string const & names)
{
    advgetopt::string_list_t split_names;
    advgetopt::split_string(names, split_names, { "," });
    if(split_names.empty())
    {
        SNAP_LOG_INFO
            << "received a listen() message with an empty list of names."
            << SNAP_LOG_SEND;
        return false;
    }

    server_service ss;
    ss.f_server = server_name;
    ss.f_service = service_name;

    bool result(true);
    for(auto const & n : split_names)
    {
        if(f_listeners[n].insert(ss).second)
        {
            result = false;
        }
    }

    return result;
}


bool server::forget(
      std::string const & server_name
    , std::string const & service_name
    , std::string const & names)
{
    advgetopt::string_list_t split_names;
    advgetopt::split_string(names, split_names, { "," });
    if(split_names.empty())
    {
        SNAP_LOG_INFO
            << "received a listen() message with an empty list of names."
            << SNAP_LOG_SEND;
        return false;
    }

    server_service ss;
    ss.f_server = server_name;
    ss.f_service = service_name;

    bool result(true);
    for(auto const & n : split_names)
    {
        auto it(f_listeners.find(n));
        if(it != f_listeners.end())
        {
            auto e(it->second.find(ss));
            if(e != it->second.end())
            {
                it->second.erase(e);
                if(it->second.empty())
                {
                    f_listeners.erase(it);
                    result = false;
                }
            }
        }
    }

    return result;
}


std::string server::list_of_options()
{
    std::set<std::string> options;
    snapdev::map_keyset(options, f_opts.get_options());
    return snapdev::join_strings(options, ",");
}


bool server::get_value(std::string const & name, std::string & value)
{
    advgetopt::option_info::pointer_t o(f_opts.get_option(name));
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

    value = it->second.rbegin()->f_value;

    return true;
}


bool server::set_value(
      std::string const & name
    , std::string const & value
    , int priority
    , snapdev::timespec_ex const & timestamp)
{
    advgetopt::option_info::pointer_t o(f_opts.get_option(name));
    if(o == nullptr)
    {
        return false;
    }

    o->set_value(0, value, advgetopt::option_source_t::SOURCE_DYNAMIC);
    if(!o->is_defined())
    {
        return false;
    }

    value_priority v;
    v.f_value = value;
    v.f_priority = priority;
    v.f_timestamp = timestamp;

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
        else if(timestamp > vp->f_timestamp)
        {
            // it was already there, but message value is more recent
            // than stored value so keep it
            //
            it->second.insert(v);
        }
    }

    return true;
}


void server::reset_setting(std::string const & name, int priority)
{
    advgetopt::option_info::pointer_t o(f_opts.get_option(name));
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

    value_priority v;
    v.f_priority = priority;
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





} // fluid_settings namespace
// vim: ts=4 sw=4 et
