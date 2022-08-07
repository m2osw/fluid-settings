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
 * \brief The implementation of the CLI.
 *
 * This file is the implementation of the CLI (Console Interface) of the
 * fluid-settings.
 *
 * This gives us access to the fluid-settings via the console. The main
 * functions are used to set new values and retrieve existing values.
 *
 * See the README.md for a list of supported options.
 */

// self
//
#include    "cli.h"

#include    "client.h"
#include    "cli_timer.h"


// fluid-settings
//
#include    "fluid-settings/exception.h"
#include    "fluid-settings/version.h"


// libutf8
//
#include    <libutf8/iterator.h>
#include    <libutf8/libutf8.h>


// libaddr
//
#include    <libaddr/addr_parser.h>


// advgetopt
//
#include    <advgetopt/exception.h>
#include    <advgetopt/validator_duration.h>


// snaplogger
//
#include    <snaplogger/message.h>
#include    <snaplogger/options.h>


// eventdispatcher
//
#include    <eventdispatcher/dispatcher.h>


// boost
//
#include    <boost/preprocessor/stringize.hpp>


// last include
//
#include    <snapdev/poison.h>



namespace fluid_settings_cli
{


namespace
{


advgetopt::option const g_options[] =
{
    advgetopt::define_option(
          advgetopt::Name("delete")
        , advgetopt::ShortName('D')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_COMMANDS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("delete a value (return it to its default).")
    ),
    advgetopt::define_option(
          advgetopt::Name("get")
        , advgetopt::ShortName('g')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_COMMANDS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("get a value.")
    ),
    advgetopt::define_option(
          advgetopt::Name("get-default")
        , advgetopt::ShortName('G')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_COMMANDS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("get the default value if that field has one.")
    ),
    advgetopt::define_option(
          advgetopt::Name("list-all")
        , advgetopt::ShortName('a')
        , advgetopt::Flags(advgetopt::standalone_all_flags<
              advgetopt::GETOPT_FLAG_GROUP_COMMANDS>())
        , advgetopt::Help("retrieve a list of all the options.")
    ),
    advgetopt::define_option(
          advgetopt::Name("list-options")
        , advgetopt::ShortName('l')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_COMMANDS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("retrieve a list of all the options of the named service.")
    ),
    advgetopt::define_option(
          advgetopt::Name("list-services")
        , advgetopt::ShortName('L')
        , advgetopt::Flags(advgetopt::standalone_all_flags<
              advgetopt::GETOPT_FLAG_GROUP_COMMANDS>())
        , advgetopt::Help("retrieve a list of all the services using fluid-settings.")
    ),
    advgetopt::define_option(
          advgetopt::Name("put")
        , advgetopt::ShortName('p')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_COMMANDS
            , advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_MULTIPLE>())
        , advgetopt::Alias("set")
    ),
    advgetopt::define_option(
          advgetopt::Name("set")
        , advgetopt::ShortName('s')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_COMMANDS
            , advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_MULTIPLE>())
        , advgetopt::Help("set a value.")
    ),
    advgetopt::define_option(
          advgetopt::Name("timeout")
        , advgetopt::ShortName('t')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_COMMANDS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue("10")
        , advgetopt::Validator("duration")
        , advgetopt::Help("time given for a message to be sent and a reply received.")
    ),
    advgetopt::define_option(
          advgetopt::Name("verbose")
        , advgetopt::ShortName('v')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_COMMANDS>())
        , advgetopt::Help("show additional information about the value, as available.")
    ),
    advgetopt::define_option(
          advgetopt::Name("watch")
        , advgetopt::ShortName('w')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_COMMANDS
            , advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_MULTIPLE>())
        , advgetopt::Help("watch values until Ctrl-C is hit.")
    ),
    advgetopt::define_option(
          advgetopt::Name("watch-if-up")
        , advgetopt::ShortName('W')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_COMMANDS
            , advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_MULTIPLE>())
        , advgetopt::Help("watch values if the fluid-settings service is up until Ctrl-C is hit.")
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


constexpr char const * const g_configuration_files[] =
{
    "/etc/fluid-settings/fluid-settings-cli.conf",
    nullptr
};


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
constexpr advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "fluid-settings-cli",
    .f_group_name = "fluid-settings",
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "FLUID_SETTINGS_CLI",
    .f_environment_variable_intro = nullptr,
    .f_section_variables_name = nullptr,
    .f_configuration_files = g_configuration_files,
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




std::string     g_service_name;




}
// no name namespace



cli::cli(int argc, char *argv[])
    : f_opts(g_options_environment)
    , f_communicator(ed::communicator::instance())
{
    snaplogger::add_logger_options(f_opts);
    f_client = std::make_shared<client>(this, f_opts);
    f_communicator->add_connection(f_client);

    f_opts.finish_parsing(argc, argv);
    if(!snaplogger::process_logger_options(
              f_opts
            , "/etc/fluid-settings/logger"
            , std::cout
            , false))
    {
        // exit on any error
        throw advgetopt::getopt_exit("logger options generated an error.", 1);
    }

    // initialize those after the finish_parsing() so command line defined
    // parameter do not get overwritten by fluid-settings
    //
    f_client->automatic_watch_initialization();

    int cmd(0);
    if(f_opts.is_defined("delete"))
    {
        ++cmd;
    }
    if(f_opts.is_defined("get"))
    {
        ++cmd;
    }
    if(f_opts.is_defined("get-default"))
    {
        ++cmd;
    }
    if(f_opts.is_defined("list-all"))
    {
        ++cmd;
    }
    if(f_opts.is_defined("list-options"))
    {
        ++cmd;
    }
    if(f_opts.is_defined("list-services"))
    {
        ++cmd;
    }
    if(f_opts.is_defined("set"))
    {
        ++cmd;
    }
    if(f_opts.is_defined("watch"))
    {
        ++cmd;
    }
    if(f_opts.is_defined("watch-if-up"))
    {
        ++cmd;
    }
    if(cmd != 1)
    {
        SNAP_LOG_ERROR
            << "you must specified exactly one command line option such as --delete, --get, --list-services, --list-options, --set, or --watch."
            << SNAP_LOG_SEND;
        throw advgetopt::getopt_exit("incorrect number of commands.", 1);
    }

    f_client->process_fluid_settings_options();
}


int cli::run()
{
    std::string const timeout_str(f_opts.get_string("timeout"));
    double duration(0.0);
    advgetopt::validator_duration::convert_string(
                  timeout_str
                , advgetopt::validator_duration::VALIDATOR_DURATION_DEFAULT_FLAGS
                , duration);
    f_timer = std::make_shared<cli_timer>(this, duration * 1'000'000LL);
    f_communicator->add_connection(f_timer);

    f_communicator->run();

    return f_success ? 0 : 1;
}


void cli::fluid_ready()
{
    std::cout << "fluid ready: all fields were received\n";
}


void cli::ready()
{
    ed::message msg;
    if(f_opts.is_defined("delete"))
    {
        msg.set_command("FLUID_SETTINGS_DELETE");
        msg.set_service("fluid_settings");
        msg.add_parameter("name", f_opts.get_string("delete"));
        msg.add_parameter("cache", "no;reply");
        f_client->send_message(msg);
    }
    else if(f_opts.is_defined("get"))
    {
        msg.set_command("FLUID_SETTINGS_GET");
        msg.set_service("fluid_settings");
        msg.add_parameter("name", f_opts.get_string("get"));
        msg.add_parameter("cache", "no;reply");
        f_client->send_message(msg);
    }
    else if(f_opts.is_defined("get-default"))
    {
        msg.set_command("FLUID_SETTINGS_GET");
        msg.set_service("fluid_settings");
        msg.add_parameter("name", f_opts.get_string("get-default"));
        msg.add_parameter("default_value", "true");
        msg.add_parameter("cache", "no;reply");
        f_client->send_message(msg);
    }
    else if(f_opts.is_defined("list-all")
         || f_opts.is_defined("list-options")
         || f_opts.is_defined("list-services"))
    {
        msg.set_command("FLUID_SETTINGS_LIST");
        msg.set_service("fluid_settings");
        if(f_opts.is_defined("list-options"))
        {
            msg.add_parameter("name", f_opts.get_string("list-options"));
        }
        msg.add_parameter("cache", "no;reply");
        f_client->send_message(msg);
    }
    else if(f_opts.is_defined("set"))
    {
        msg.set_command("FLUID_SETTINGS_PUT");
        msg.set_service("fluid_settings");
        msg.add_parameter("name", f_opts.get_string("set"));
        msg.add_parameter("value", f_opts.get_string("set", 1));
        msg.add_parameter("cache", "no;reply");
        f_client->send_message(msg);
    }
    else if(f_opts.is_defined("watch")
         || f_opts.is_defined("watch-if-up"))
    {
        if(f_opts.is_defined("watch"))
        {
            f_timer->set_enable(false);
        }
        setup_watches();
    }
    else
    {
        // TBD: should this be an error?
        //
        SNAP_LOG_WARNING
            << "no command found."
            << SNAP_LOG_SEND;
    }
}


void cli::setup_watches()
{
    // the names shold be separated by commas but we give the user the
    // ability to write them between spaces
    //
    //   --watch msg1,msg2,msg3,...
    //
    // or
    //
    //   --watch msg1 msg2 msg3
    //
    // both formats can be mixed
    //
    std::size_t const max(f_opts.size("watch"));
    for(std::size_t idx(0); idx < max; ++idx)
    {
        advgetopt::string_list_t list;
        advgetopt::split_string(f_opts.get_string("watch", idx), list, {","});
        for(auto const & l : list)
        {
            f_client->add_watch(l);
        }
    }
}


void cli::deleted()
{
    f_success = true;

    close();
}


void cli::failed(ed::message & msg)
{
    if(msg.has_parameter("error_command"))
    {
        std::cerr
            << "command that generated the error: "
            << msg.get_parameter("error_command")
            << '\n';
    }
    if(msg.has_parameter("error"))
    {
        std::cerr
            << "error message: "
            << msg.get_parameter("error")
            << '\n';
    }

    close();
}


void cli::list(advgetopt::string_list_t const & options)
{
    if(f_opts.is_defined("list-all"))
    {
        for(auto const & o : options)
        {
            std::cout << o << '\n';
        }
        f_success = true;
    }
    else if(f_opts.is_defined("list-options"))
    {
        std::string start_with(f_opts.get_string("list-options"));
        if(start_with.empty())
        {
            SNAP_LOG_ERROR
                << "the --list-options command line option must specified a non-empty service name."
                << SNAP_LOG_SEND;
        }
        else
        {
            if(start_with.back() == ':')
            {
                if(start_with.length() >= 2
                && start_with[start_with.length() - 2] != ':')
                {
                    start_with += ':';
                }
            }
            else
            {
                start_with += "::";
            }
            for(auto const & o : options)
            {
                if(o.length() > start_with.length()
                && o.substr(0, start_with.length()) == start_with)
                {
                    std::cout << o.substr(start_with.length()) << '\n';
                }
            }
            f_success = true;
        }
    }
    else if(f_opts.is_defined("list-services"))
    {
        std::set<std::string> services;
        for(auto const & o : options)
        {
            std::string::size_type const pos(o.find(':'));
            if(pos != std::string::npos)
            {
                services.insert(o.substr(0, pos));
            }
        }
        for(auto const & s : services)
        {
            std::cout << s << '\n';
        }
        f_success = true;
    }
    else
    {
        throw fluid_settings::fluid_settings_implementation_error("which --list-... option was used?");
    }

    close();
}


void cli::registered()
{
    // we are registered to watch for changes so we do not want to time out
    //
    f_timer->set_enable(false);
}


void cli::service_down()
{
    if(f_opts.is_defined("watch-if-up"))
    {
        // the service just went down, quit now
        //
        close();
    }
}


void cli::updated()
{
    f_success = true;
    close();
}


void cli::value(
          std::string const & name
        , std::string const & value
        , bool is_default)
{
    if(f_opts.is_defined("verbose")
    && is_default)
    {
        std::cout << "the value is not currently set, here is the default value:\n";
    }
    std::cout << name << '=';
    f_success = print_value(value);

    close();
}


void cli::value_updated(std::string const & name, std::string const & value)
{
    std::cout << name << '=';
    print_value(value);
}


void cli::close()
{
    f_client->unregister_fluid_settings(false);
    f_communicator->remove_connection(f_client);
    f_communicator->remove_connection(f_timer);
}


void cli::timeout()
{
    SNAP_LOG_ERROR
        << "we did not receive a reply to our query in time."
        << SNAP_LOG_SEND;

    close();
}


bool cli::print_value(std::string const & value)
{
    bool result(true);
    for(libutf8::utf8_iterator it(value)
      ; it != value.end()
      ; ++it)
    {
        char32_t c(*it);
        if(c < ' ')
        {
            std::cout << '^' << (static_cast<char>(c) + '@');
        }
        else if(c >= 0x80 && c < 0xA0)
        {
            std::cout << '@' << (static_cast<char>(c) - (0x80 - '@'));
        }
        else if(libutf8::is_surrogate(c) != libutf8::surrogate_t::SURROGATE_NO)
        {
            // surrogates are not allowed in UTF-8 strings
            //
            SNAP_LOG_WARNING
                << "found a surrogate character in option value."
                << SNAP_LOG_SEND;
            result = false;
        }
        else
        {
            std::cout << libutf8::to_u8string(c);
        }
    }
    std::cout << '\n';

    return result;
}


std::string const & get_our_service_name()
{
    if(g_service_name.empty())
    {
        g_service_name = "fluid_settings_cli";
        g_service_name += std::to_string(getpid());
    }

    return g_service_name;
}


} // fluid_settings namespace
// vim: ts=4 sw=4 et
