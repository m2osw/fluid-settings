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

#include    "gossip_timer.h"
#include    "listener.h"
#include    "messenger.h"
#include    "replicator_in.h"
#include    "replicator_out.h"
#include    "save_timer.h"


// fluid-settings
//
#include    <fluid-settings/names.h>
#include    <fluid-settings/version.h>


// eventdispatcher
//
#include    <eventdispatcher/broadcast_message.h>


// communicatord
//
#include    <communicatord/communicator.h>
#include    <communicatord/names.h>


// advgetopt
//
#include    <advgetopt/exception.h>
#include    <advgetopt/validator_duration.h>
#include    <advgetopt/validator_integer.h>


// libaddr
//
#include    <libaddr/addr_parser.h>


// snaplogger
//
#include    <snaplogger/message.h>
#include    <snaplogger/options.h>


// snapdev
//
#include    <snapdev/safe_variable.h>
#include    <snapdev/stringize.h>
#include    <snapdev/tokenize_string.h>


// C++
//
#include    <functional>


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
          advgetopt::Name("definitions")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("a colon separated list of paths to fluid-settings definitions.")
    ),
    advgetopt::define_option(
          advgetopt::Name("gossip-timeout")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue("60s")
        , advgetopt::Validator("duration")
        , advgetopt::Help("number of seconds to wait before sending another FLUID_SETTINGS_GOSSIP message.")
    ),
    advgetopt::define_option(
          advgetopt::Name("listen")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue("127.0.0.1:4049")
        , advgetopt::Help("set the IP:port to listen on for connections by other fluid-settings daemons.")
    ),
    advgetopt::define_option(
          advgetopt::Name("settings")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue(fluid_settings::g_settings_file)
        , advgetopt::Help("a full path and filename to a file where to save the fluid settings.")
    ),
    advgetopt::define_option(
          advgetopt::Name("save-timeout")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue("5s")
        , advgetopt::Validator("duration")
        , advgetopt::Help("number of seconds to wait before saving the latest changes; must be a valid positive number.")
    ),
    advgetopt::define_option(
          advgetopt::Name("snapcommunicator")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue(communicatord::g_communicatord_default_ip_port.data())
        , advgetopt::Help("set the snapcommunicator IP:port to connect to.")
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
    "/etc/fluid-settings/fluid-settings.conf",
    nullptr
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
                   SNAPDEV_STRINGIZE(UTC_BUILD_YEAR)
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
    , f_messenger(std::make_shared<messenger>(this, f_opts))
{
    snaplogger::add_logger_options(f_opts);
    f_opts.finish_parsing(argc, argv);
    if(!snaplogger::process_logger_options(f_opts, "/etc/fluid-settings/logger"))
    {
        // exit on any error
        throw advgetopt::getopt_exit("logger options generated an error.", 1);
    }
    f_messenger->process_communicatord_options();
}


int server::run()
{
    using prepare_t = bool(server::*)();

    prepare_t initializers[] = {
        &server::prepare_settings,
        &server::prepare_listener,
        &server::prepare_save_timer,
        &server::prepare_gossip_timer,
    };

    for(auto const & f : initializers)
    {
        if(!(this->*f)())
        {
            return 1;
        }
    }

    f_communicator->run();

    return f_exit_code;
}


bool server::prepare_settings()
{
    std::string paths;
    if(f_opts.is_defined("definitions"))
    {
        paths = f_opts.get_string("definitions");
    }
    if(!f_settings.load_definitions(paths))
    {
        SNAP_LOG_NOTICE
            << "no definitions found; is fluid-settings expecting definitions from other computers?"
            << SNAP_LOG_SEND;
    }

    f_settings.load(f_opts.get_string("settings"));

    return true;
}


bool server::prepare_listener()
{
    f_listener_address = addr::string_to_addr(
                          f_opts.get_string("listen")
                        , "127.0.0.1"
                        , 4052
                        , "tcp");

    f_listener = std::make_shared<listener>(this, f_listener_address, 5);
    f_communicator->add_connection(f_listener);

    return true;
}


bool server::prepare_save_timer()
{
    std::string const & timeout(f_opts.get_string("save-timeout"));
    double seconds(0.0);
    if(!advgetopt::validator_duration::convert_string(
              timeout
            , advgetopt::validator_duration::VALIDATOR_DURATION_DEFAULT_FLAGS
            , seconds))
    {
        SNAP_LOG_FATAL
            << "the --save-timeout parameter must be a valid duration (\""
            << timeout
            << "\" is invalid)."
            << SNAP_LOG_SEND;
        return false;
    }
    if(seconds <= 0.0)
    {
        SNAP_LOG_FATAL
            << "the --save-timeout parameter must be a valid positive duration (\""
            << timeout
            << "\" is invalid)."
            << SNAP_LOG_SEND;
        return false;
    }
    f_save_timeout = seconds * 1'000'000;

    f_save_timer = std::make_shared<save_timer>(this, f_save_timeout);
    f_communicator->add_connection(f_save_timer);

    return true;
}


bool server::prepare_gossip_timer()
{
    f_gossip_timeout = f_opts.get_long("gossip-timeout");
    if(f_gossip_timeout <= 0)
    {
        SNAP_LOG_FATAL
            << "the --gossip-timeout parameter must be a valid positive number (\""
            << f_opts.get_string("gossip-timeout")
            << "\" is invalid)."
            << SNAP_LOG_SEND;
        return false;
    }
    f_gossip_timer = std::make_shared<gossip_timer>(this, f_gossip_timeout * 1'000);
    f_communicator->add_connection(f_gossip_timer);

    return true;
}


void server::restart()
{
    f_exit_code = 1;
    stop(false);
}


void server::stop(bool quitting)
{
    if(f_messenger != nullptr)
    {
        f_messenger->unregister_communicator(quitting);
    }

    if(f_communicator != nullptr)
    {
        f_communicator->remove_connection(f_gossip_timer);
        f_gossip_timer.reset();

        f_communicator->remove_connection(f_save_timer);
        f_save_timer.reset();

        f_communicator->remove_connection(f_listener);
        f_listener.reset();
    }
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
    for(auto & n : split_names)
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
            << "received a forget() message with an empty list of names."
            << SNAP_LOG_SEND;
        return false;
    }

    server_service ss;
    ss.f_server = server_name;
    ss.f_service = service_name;

    bool result(true);
    for(auto & n : split_names)
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
    return f_settings.list_of_options();
}


fluid_settings::get_result_t server::get_default_value(
      std::string const & name
    , std::string & value)
{
    return f_settings.get_default_value(name, value);
}


fluid_settings::get_result_t server::get_value(
      std::string const & name
    , std::string & value
    , fluid_settings::priority_t priority
    , bool all)
{
    return f_settings.get_value(name, value, priority, all);
}


fluid_settings::set_result_t server::set_value(
      std::string const & name
    , std::string const & value
    , fluid_settings::priority_t priority
    , fluid_settings::timestamp_t const & timestamp)
{
    fluid_settings::set_result_t result(f_settings.set_value(name, value, priority, timestamp));
    switch(result)
    {
    case fluid_settings::set_result_t::SET_RESULT_NEW:
    case fluid_settings::set_result_t::SET_RESULT_NEW_PRIORITY:
    case fluid_settings::set_result_t::SET_RESULT_CHANGED:
        value_changed(name);
        break;

    default:
        break;

    }

    return result;
}


bool server::reset_setting(
      std::string const & name
    , fluid_settings::priority_t priority)
{
    if(f_settings.reset_setting(name, priority))
    {
        value_changed(name);
        return true;
    }

    return false;
}


void server::value_changed(std::string const & name)
{
    if(f_messenger == nullptr)
    {
        return;
    }

    if(!f_save_timer->is_enabled())
    {
        f_save_timer->set_enable(true);
        f_save_timer->set_timeout_delay(f_save_timeout);
    }

    // tell the listeners about the new value
    //
    std::string value;
    fluid_settings::get_result_t const result(f_settings.get_value(name, value));
    for(auto const & s : f_listeners[name])
    {
        ed::message new_value;
        new_value.set_command(fluid_settings::g_name_fluid_settings_cmd_fluid_settings_value_updated);
        new_value.add_parameter(fluid_settings::g_name_fluid_settings_param_name, name);
        switch(result)
        {
        case fluid_settings::get_result_t::GET_RESULT_SUCCESS:
        case fluid_settings::get_result_t::GET_RESULT_DEFAULT:
            new_value.add_parameter(fluid_settings::g_name_fluid_settings_param_value, value);
            break;

        default:
            new_value.add_parameter(fluid_settings::g_name_fluid_settings_param_reason, "value undefined");
            break;

        }
        new_value.set_server(s.f_server);
        new_value.set_service(s.f_service);
        f_messenger->send_message(new_value);
    }

    // if this change happened because another fluid-settings sent us
    // a message, avoid broadcasting back
    //
    // TODO: verify that this is really correct... while everyone is
    //       properly connected, we're certainly just fine, if someone
    //       was not connected to the sender, maybe it is to this
    //       instance and it should also be sent a copy of the new value
    //
    if(f_remote_change)
    {
        return;
    }

    // next we want to tell the other fluid-settings that things changed
    //
    ed::message value_changed;
    value_changed.set_command(fluid_settings::g_name_fluid_settings_cmd_value_changed);
    value_changed.add_parameter(fluid_settings::g_name_fluid_settings_param_name, name);
    value_changed.add_parameter(fluid_settings::g_name_fluid_settings_param_values, f_settings.serialize_value(name));
    ed::broadcast_message(f_replicators, value_changed, false);

    // old way...
    //for(auto & c : f_communicator->get_connections())
    //{
    //    // fluid-settings we connected to
    //    //
    //    replicator_out::pointer_t rout(std::dynamic_pointer_cast<replicator_out>(c));
    //    if(rout != nullptr)
    //    {
    //        rout->send_message(value_changed);
    //        continue;
    //    }
    //
    //    // fluid-settings that connected to us
    //    //
    //    replicator_in::pointer_t rin(std::dynamic_pointer_cast<replicator_in>(c));
    //    if(rin != nullptr)
    //    {
    //        rin->send_message(value_changed);
    //        continue;
    //    }
    //}
}


void server::save_settings()
{
    f_settings.save(f_opts.get_string("settings"));
}


addr::addr const & server::get_listener_address() const
{
    return f_listener_address;
}


void server::send_gossip()
{
    if(f_messenger == nullptr)
    {
        return;
    }

    ed::message gossip;
    gossip.set_command(fluid_settings::g_name_fluid_settings_cmd_fluid_settings_gossip);
    gossip.set_server(communicatord::g_name_communicatord_server_any);
    gossip.set_service(fluid_settings::g_name_fluid_settings_service_fluid_settings);
    gossip.add_parameter(fluid_settings::g_name_fluid_settings_param_my_ip, f_listener_address.to_ipv4or6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT));
    f_messenger->send_message(gossip);
}


void server::connect_to_other_fluid_settings(addr::addr const & their_ip)
{
    replicator_out::pointer_t connection(std::make_shared<replicator_out>(this, their_ip));
    if(f_communicator->add_connection(connection))
    {
        add_replicator(connection);
    }
    else
    {
        SNAP_LOG_ERROR
            << "new replicator_out could not be added to ed::communicator."
            << SNAP_LOG_SEND;
    }
}


void server::add_replicator(ed::connection_with_send_message::weak_t connection)
{
    f_replicators.push_back(connection);
}


void server::remote_value_changed(
      ed::message const & msg
    , ed::connection_with_send_message::pointer_t const & c)
{
    snapdev::NOT_USED(c);

    snapdev::safe_variable<bool> safe(f_remote_change, true, false);

    std::string const name(msg.get_parameter(fluid_settings::g_name_fluid_settings_param_name));
    std::string const values(msg.get_parameter(fluid_settings::g_name_fluid_settings_param_values));

    f_settings.unserialize_values(name, values);
}



} // fluid_settings namespace
// vim: ts=4 sw=4 et
