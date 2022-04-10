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
#include    "messenger.h"


//// fluid-settings
////
//#include    "fluid-settings/version.h"
//
//
//// libutf8
////
//#include    <libutf8/iterator.h>
//#include    <libutf8/libutf8.h>
//
//
//// libaddr
////
//#include    <libaddr/addr_parser.h>


// advgetopt
//
#include    <advgetopt/validator_double.h>


//// snaplogger
////
//#include    <snaplogger/options.h>
//
//
//// eventdispatcher
////
//#include    <eventdispatcher/dispatcher.h>
//
//
//// boost
////
//#include    <boost/preprocessor/stringize.hpp>


// last include
//
#include    <snapdev/poison.h>



namespace fluid_settings_daemon
{



ed::dispatcher<messenger>::dispatcher_match::vector_t const g_dispatcher_messages =
{
    {
          "FLUID_SETTINGS_DELETE"
        , &messenger::msg_delete
    },
    {
          "FLUID_SETTINGS_FORGET"
        , &messenger::msg_forget
    },
    {
          "FLUID_SETTINGS_GET"
        , &messenger::msg_get
    },
    {
          "FLUID_SETTINGS_LIST"
        , &messenger::msg_list
    },
    {
          "FLUID_SETTINGS_LISTEN"
        , &messenger::msg_listen
    },
    {
          "FLUID_SETTINGS_PUT"
        , &messenger::msg_put
    },
};


messenger::messenger(server * s, addr::addr const & address)
    : tcp_client_permanent_message_connection(address)
    , f_server(s)
    , f_dispatcher(std::make_shared<ed::dispatcher<messenger>>(this, g_dispatcher_messages))
{
    f_dispatcher->add_communicator_commands();
#ifdef _DEBUG
    f_dispatcher->set_trace();
#endif
}


messenger::~messenger()
{
}


void messenger::msg_delete(ed::message & msg)
{
    ed::message reply;
    reply.reply_to(msg);

    if(!msg.has_parameter("name"))
    {
        reply.set_command("FLUID_SETTINGS_ERROR");
        reply.add_parameter("error", "parameter \"name\" missing in message");
        reply.add_parameter("error_command", "FLUID_SETTINGS_DELETE");
        send_message(reply);
        return;
    }

    int priority(50);
    if(msg.has_parameter("priority"))
    {
        priority = msg.get_integer_parameter("priority");
        if(priority < 0 || priority > 99)
        {
            reply.set_command("FLUID_SETTINGS_ERROR");
            reply.add_parameter("error", "parameter \"priority\" is out of range (0 .. 99)");
            reply.add_parameter("error_command", "FLUID_SETTINGS_DELETE");
            send_message(reply);
            return;
        }
    }

    f_server->reset_setting(msg.get_parameter("name"), priority);

    reply.set_command("FLUID_SETTINGS_DELETED");
    reply.add_parameter("name", msg.get_parameter("name"));
    send_message(reply);
}


void messenger::msg_forget(ed::message & msg)
{
    ed::message reply;
    reply.reply_to(msg);

    std::string const server(msg.get_server());
    std::string const service(msg.get_service());

    if(server.empty()
    || service.empty()
    || !msg.has_parameter("names"))
    {
        reply.set_command("FLUID_SETTINGS_ERROR");
        reply.add_parameter("error", "parameter \"server\" or \"service\" or \"names\" missing in message");
        reply.add_parameter("error_command", "FLUID_SETTINGS_FORGET");
        send_message(reply);
        return;
    }

    std::string const names(msg.get_parameter("names"));
    if(f_server->forget(server, service, names))
    {
        reply.set_command("FLUID_SETTINGS_FORGET");
        reply.add_parameter("message", "not listening");
        send_message(reply);
    }
    else
    {
        reply.set_command("FLUID_SETTINGS_FORGET");
        send_message(reply);
    }
}


void messenger::msg_get(ed::message & msg)
{
    ed::message reply;
    reply.reply_to(msg);

    if(!msg.has_parameter("name"))
    {
        reply.set_command("FLUID_SETTINGS_ERROR");
        reply.add_parameter("error", "parameter \"name\" missing in message");
        reply.add_parameter("error_command", "FLUID_SETTINGS_GET");
        send_message(reply);
        return;
    }

    std::string const name(msg.get_parameter("name"));
    std::string value;
    if(f_server->get_value(name, value))
    {
        reply.set_command("FLUID_SETTINGS_VALUE");
        reply.add_parameter("value", value);
        send_message(reply);
    }
    else
    {
        reply.set_command("FLUID_SETTINGS_ERROR");
        reply.add_parameter("error", "no parameter named \"" + name + "\"");
        reply.add_parameter("error_command", "FLUID_SETTINGS_GET");
        reply.add_parameter("name", name);
        send_message(reply);
    }
}


void messenger::msg_list(ed::message & msg)
{
    ed::message reply;
    reply.reply_to(msg);

    std::string const options(f_server->list_of_options());

    reply.set_command("FLUID_SETTINGS_OPTIONS");
    reply.add_parameter("options", options);
    send_message(reply);
}


void messenger::msg_listen(ed::message & msg)
{
    ed::message reply;
    reply.reply_to(msg);

    std::string const server(msg.get_server());
    std::string const service(msg.get_service());

    if(server.empty()
    || service.empty()
    || !msg.has_parameter("names"))
    {
        reply.set_command("FLUID_SETTINGS_ERROR");
        reply.add_parameter("error", "parameter \"server\" or \"service\" or \"names\" missing in message");
        reply.add_parameter("error_command", "FLUID_SETTINGS_LISTEN");
        send_message(reply);
        return;
    }

    std::string const names(msg.get_parameter("names"));
    if(f_server->listen(server, service, names))
    {
        reply.set_command("FLUID_SETTINGS_REGISTERED");
        reply.add_parameter("message", "already registered");
        send_message(reply);
    }
    else
    {
        reply.set_command("FLUID_SETTINGS_REGISTERED");
        send_message(reply);
    }
}


void messenger::msg_put(ed::message & msg)
{
    ed::message reply;
    reply.reply_to(msg);

    if(!msg.has_parameter("name")
    || !msg.has_parameter("value"))
    {
        reply.set_command("FLUID_SETTINGS_ERROR");
        reply.add_parameter("error", "parameter \"name\" or \"value\" missing in message");
        reply.add_parameter("error_command", "FLUID_SETTINGS_PUT");
        send_message(reply);
        return;
    }

    std::string const name(msg.get_parameter("name"));
    std::string const value(msg.get_parameter("value"));
    snapdev::timespec_ex timestamp;
    if(msg.has_parameter("timestamp"))
    {
        std::string stamp(msg.get_parameter("timestamp"));
        double result(0.0);
        advgetopt::validator_double::convert_string(stamp, result);
        timestamp.set(result);
    }
    else
    {
        timestamp.gettime();
    }

    int priority(50);
    if(msg.has_parameter("priority"))
    {
        priority = msg.get_integer_parameter("priority");
        if(priority < 0 || priority > 99)
        {
            reply.set_command("FLUID_SETTINGS_ERROR");
            reply.add_parameter("error", "parameter \"priority\" is out of range (0 .. 99)");
            reply.add_parameter("error_command", "FLUID_SETTINGS_PUT");
            send_message(reply);
            return;
        }
    }

    if(f_server->set_value(name, value, priority, timestamp))
    {
        reply.set_command("FLUID_SETTINGS_UPDATED");
        reply.add_parameter("name", name);
        send_message(reply);
    }
    else
    {
        reply.set_command("FLUID_SETTINGS_ERROR");
        reply.add_parameter("error", "setting named \"" + name + "\" to value \"" + value + "\" failed");
        reply.add_parameter("error_command", "FLUID_SETTINGS_PUT");
        send_message(reply);
    }
}



} // namespace fluid_settings_daemon
// vim: ts=4 sw=4 et
