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
          advgetopt::Name("definitions")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::DefaultValue("127.0.0.1:4050")
        , advgetopt::Help("a colon separated list of paths to fluid-settings definitions.")
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
    std::string const paths(f_opts.get_string("definitions"));
    f_definitions.load_definitions(paths);

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
    return f_definitions.list_of_options();
}


bool server::get_value(std::string const & name, std::string & value)
{
    return f_definitions.get_value(name, value);
}


bool server::set_value(
      std::string const & name
    , std::string const & value
    , int priority
    , snapdev::timespec_ex const & timestamp)
{
    return f_definitions.set_value(name, value, priority, timestamp);
}


void server::reset_setting(std::string const & name, int priority)
{
    return f_definitions.reset_setting(name, priority);
}





} // fluid_settings namespace
// vim: ts=4 sw=4 et
