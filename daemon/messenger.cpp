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


// advgetopt
//
#include    <advgetopt/validator_double.h>
#include    <advgetopt/validator_integer.h>


// libaddr
//
#include    <libaddr/addr_parser.h>


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

ed::dispatcher<messenger>::dispatcher_match::vector_t const g_dispatcher_messages =
{
    ed::dispatcher<messenger>::define_match(
          ed::dispatcher<messenger>::Expression("FLUID_SETTINGS_CONNECTED")
        , ed::dispatcher<messenger>::Execute(&messenger::msg_connected)
    ),
    ed::dispatcher<messenger>::define_match(
          ed::dispatcher<messenger>::Expression("FLUID_SETTINGS_DELETE")
        , ed::dispatcher<messenger>::Execute(&messenger::msg_delete)
    ),
    ed::dispatcher<messenger>::define_match(
          ed::dispatcher<messenger>::Expression("FLUID_SETTINGS_FORGET")
        , ed::dispatcher<messenger>::Execute(&messenger::msg_forget)
    ),
    ed::dispatcher<messenger>::define_match(
          ed::dispatcher<messenger>::Expression("FLUID_SETTINGS_GET")
        , ed::dispatcher<messenger>::Execute(&messenger::msg_get)
    ),
    ed::dispatcher<messenger>::define_match(
          ed::dispatcher<messenger>::Expression("FLUID_SETTINGS_GOSSIP")
        , ed::dispatcher<messenger>::Execute(&messenger::msg_gossip)
    ),
    ed::dispatcher<messenger>::define_match(
          ed::dispatcher<messenger>::Expression("FLUID_SETTINGS_LIST")
        , ed::dispatcher<messenger>::Execute(&messenger::msg_list)
    ),
    ed::dispatcher<messenger>::define_match(
          ed::dispatcher<messenger>::Expression("FLUID_SETTINGS_LISTEN")
        , ed::dispatcher<messenger>::Execute(&messenger::msg_listen)
    ),
    ed::dispatcher<messenger>::define_match(
          ed::dispatcher<messenger>::Expression("FLUID_SETTINGS_PUT")
        , ed::dispatcher<messenger>::Execute(&messenger::msg_put)
    ),
};

} // no name namespace



messenger::messenger(server * s, advgetopt::getopt & opts)
    : communicator(opts, "fluid_settings")
    , f_server(s)
    , f_dispatcher(std::make_shared<ed::dispatcher<messenger>>(this, g_dispatcher_messages))
{
    f_dispatcher->add_communicator_commands();
#ifdef _DEBUG
    f_dispatcher->set_trace();
#endif
    set_dispatcher(f_dispatcher);
}


messenger::~messenger()
{
}


void messenger::ready(ed::message & msg)
{
    snapdev::NOT_USED(msg);

    // send a first gossip message as soon as we are ready
    //
    f_server->send_gossip();
}


void messenger::restart(ed::message & msg)
{
    snapdev::NOT_USED(msg);

    f_server->restart();
}


void messenger::stop(bool quitting)
{
    f_server->stop(quitting);
}


void messenger::msg_connected(ed::message & msg)
{
    connect_from_gossip(msg, false);
}


/** \brief Delete a value.
 *
 * This function resets the named setting.
 *
 * Note that it does not really \em delete the value since it is not possible
 * to do so. Instead, it resets the value back to its defaults. If no default,
 * then the value still exists but is unassigned (FLUID_SETTINGS_NOT_SET).
 *
 * \param[in] msg  The message used to find the name of the value to delete.
 */
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

    int priority(fluid_settings::ADMINISTRATOR_PRIORITY);
    if(msg.has_parameter("priority"))
    {
        priority = msg.get_integer_parameter("priority");
        if(priority < fluid_settings::MINIMUM_PRIORITY
        || priority > fluid_settings::MAXIMUM_PRIORITY)
        {
            reply.set_command("FLUID_SETTINGS_ERROR");
            reply.add_parameter(
                      "error"
                    , "parameter \"priority\" is out of range ("
                      BOOST_PP_STRINGIZE(fluid_settings::MINIMUM_PRIORITY)
                      " .. "
                      BOOST_PP_STRINGIZE(fluid_settings::MAXIMUM_PRIORITY)
                      ")");
            reply.add_parameter("error_command", "FLUID_SETTINGS_DELETE");
            send_message(reply);
            return;
        }
    }

    std::string const name(msg.get_parameter("name"));
    if(f_server->reset_setting(name, priority))
    {
        reply.set_command("FLUID_SETTINGS_DELETED");
        reply.add_parameter("name", name);
        send_message(reply);
    }
    else
    {
        // we still reply positively so the other side does not have to do
        // anything special about the fact that nothing was deleted
        //
        reply.set_command("FLUID_SETTINGS_DELETED");
        reply.add_parameter("name", name);
        reply.add_parameter("message", "nothing was deleted");
        send_message(reply);
    }
}


/** \brief Forget a previously registered listener.
 *
 * This message is used to disconnect from the fluid-settings service.
 *
 * \param[in] msg  The message with the information of which service(s) to forget.
 */
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


/** \brief Get a value.
 *
 * The message supports the following parameters:
 *
 * * `name` (mandatory) -- the name of the parameter to retrieve; you must
 * retrieve values one at a time; see the msg_listen() to listen to values
 * instead (i.e. for an app. continuously running)
 * * `priority` (optional) -- to retrieve a value at a specific priority
 * such as the DEFAULT_PRIORITY and the ADMINISTRATOR_PRIORITY; the default
 * is to use the HIGHEST_PRIORITY; note, however, that you cannot set this
 * parameter to that value (i.e. it is out of bounds)
 * * `all` (optional) -- to retrieve all the currently available values
 *
 * The function may reply with the following messages:
 *
 * * FLUID_SETTINGS_ALL_VALUES -- the `all` parameter was set to `true`;
 * the `values` parameter contains a comma separated list of values (with
 * commas within values backslash escaped)
 * * FLUID_SETTINGS_VALUE -- the current or priority specific value
 * * FLUID_SETTINGS_ERROR -- an error occurred (i.e. value not defined,
 * missing parameter, etc.)
 *
 * \note
 * The `priority` and `all` parameters are mutually exclusive. Specifying
 * both at the same time results in an error.
 *
 * \param[in] msg  The GET message.
 */
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

    bool all(false);
    if(msg.has_parameter("all"))
    {
        if(msg.has_parameter("priority"))
        {
            reply.set_command("FLUID_SETTINGS_ERROR");
            reply.add_parameter("error", "parameters \"all\" and \"priority\" are mutually exclusive");
            reply.add_parameter("error_command", "FLUID_SETTINGS_GET");
            send_message(reply);
            return;
        }

        all = advgetopt::is_true(msg.get_parameter("all"));
    }

    fluid_settings::priority_t priority(fluid_settings::HIGHEST_PRIORITY);
    if(msg.has_parameter("priority"))
    {
        std::int64_t result(0);
        if(!advgetopt::validator_integer::convert_string(msg.get_parameter("priority"), result))
        {
            reply.set_command("FLUID_SETTINGS_ERROR");
            reply.add_parameter("error", "parameter \"priority\" must be an integer when defined");
            reply.add_parameter("error_command", "FLUID_SETTINGS_GET");
            send_message(reply);
            return;
        }

        priority = static_cast<fluid_settings::priority_t>(result);
    }

    std::string const name(msg.get_parameter("name"));
    reply.add_parameter("name", name);

    std::string value;
    fluid_settings::get_result_t const r(f_server->get_value(name, value, priority, all));
    switch(r)
    {
    case fluid_settings::get_result_t::GET_RESULT_SUCCESS:
        if(all)
        {
            // since commas need special handling in this case, we use
            // different names for the reply
            //
            reply.set_command("FLUID_SETTINGS_ALL_VALUES");
            reply.add_parameter("values", value);
        }
        else
        {
            reply.set_command("FLUID_SETTINGS_VALUE");
            reply.add_parameter("value", value);
        }
        break;

    case fluid_settings::get_result_t::GET_RESULT_DEFAULT:
        reply.set_command("FLUID_SETTINGS_DEFAULT_VALUE");
        reply.add_parameter("value", value);
        break;

    case fluid_settings::get_result_t::GET_RESULT_NOT_SET:
        reply.set_command("FLUID_SETTINGS_NOT_SET");
        reply.add_parameter("error", "this setting has current no values set");
        break;

    case fluid_settings::get_result_t::GET_RESULT_PRIORITY_NOT_FOUND:
        reply.set_command("FLUID_SETTINGS_NOT_SET");
        reply.add_parameter("error", "no value at the requested priority");
        break;

    case fluid_settings::get_result_t::GET_RESULT_ERROR:
        reply.set_command("FLUID_SETTINGS_ERROR");
        reply.add_parameter("error", "found a parameter named \"" + name + "\" but no corresponding value (logic error)");
        reply.add_parameter("error_command", "FLUID_SETTINGS_GET");
        break;

    case fluid_settings::get_result_t::GET_RESULT_UNKNOWN:
        reply.set_command("FLUID_SETTINGS_ERROR");
        reply.add_parameter("error", "no parameter named \"" + name + "\"");
        reply.add_parameter("error_command", "FLUID_SETTINGS_GET");
        break;

    }
    send_message(reply);
}


void messenger::msg_gossip(ed::message & msg)
{
    connect_from_gossip(msg, true);
}


void messenger::connect_from_gossip(ed::message & msg, bool send_reply)
{
    ed::message reply;
    reply.reply_to(msg);

    if(!msg.has_parameter("my_ip"))
    {
        reply.set_command("FLUID_SETTINGS_ERROR");
        reply.add_parameter("error", "parameter \"my_ip\" missing in message");
        reply.add_parameter("error_command", "FLUID_SETTINGS_GOSSIP");
        send_message(reply);
        return;
    }

    std::string const their_ip(msg.get_parameter("my_ip"));

    addr::addr const a(f_server->get_listener_address());
    addr::addr const b(addr::string_to_addr(
                          their_ip
                        , "127.0.0.1"
                        , 4051
                        , "tcp"));

    if(a < b)
    {
        f_server->connect_to_other_fluid_settings(b);

        reply.add_parameter("message", "we sent you a connection request");
    }
    else
    {
        reply.add_parameter("message", "you connect to us");
    }


    if(send_reply)
    {
        reply.set_command("FLUID_SETTINGS_CONNECTED");
        reply.add_parameter("my_ip", a.to_ipv4or6_string(addr::string_ip_t::STRING_IP_PORT));

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

    std::string const server(msg.get_sent_from_server());
    std::string const service(msg.get_sent_from_service());

    if(server.empty()
    || service.empty()
    || !msg.has_parameter("names"))
    {
        reply.set_command("FLUID_SETTINGS_ERROR");
        reply.add_parameter("error", "parameter \"server\" ("
                + server
                + ") or \"service\" ("
                + service
                + ") are empty or parameter \"names\" ("
                + (msg.has_parameter("names") ? msg.get_parameter("names") : std::string())
                + ") is missing in message");
        reply.add_parameter("error_command", "FLUID_SETTINGS_LISTEN");
        send_message(reply);
        return;
    }

    std::string const names(msg.get_parameter("names"));

    reply.set_command("FLUID_SETTINGS_REGISTERED");
    if(f_server->listen(server, service, names))
    {
        reply.add_parameter("message", "already registered");
    }
    send_message(reply);

    // we then want to send the current value as if the value had just been
    // updated although the message will clearly say that it is the current
    // value
    //
    advgetopt::string_list_t split_names;
    advgetopt::split_string(names, split_names, { "," });
    for(auto const & n : split_names)
    {
        ed::message current_value;
        current_value.reply_to(msg);
        current_value.set_command("FLUID_SETTINGS_VALUE_UPDATED");
        current_value.add_parameter("name", n);

        std::string value;
        fluid_settings::get_result_t const r(f_server->get_value(
                      n
                    , value
                    , fluid_settings::HIGHEST_PRIORITY
                    , false));
        switch(r)
        {
        case fluid_settings::get_result_t::GET_RESULT_DEFAULT:
            current_value.add_parameter("default", "true");
            [[fallthrough]];
        case fluid_settings::get_result_t::GET_RESULT_SUCCESS:
            current_value.add_parameter("value", value);
            current_value.add_parameter("message", "current value");
            break;

        case fluid_settings::get_result_t::GET_RESULT_NOT_SET:
            current_value.add_parameter("error", "not set");
            break;

        case fluid_settings::get_result_t::GET_RESULT_PRIORITY_NOT_FOUND:
            // this one should never happen since we use "highest"
            //
            current_value.add_parameter("error", "priority not found");
            break;

        case fluid_settings::get_result_t::GET_RESULT_ERROR:
            current_value.add_parameter(
                      "error", "found a parameter named \""
                    + n
                    + "\" but no corresponding value (logic error)");
            break;

        case fluid_settings::get_result_t::GET_RESULT_UNKNOWN:
            current_value.add_parameter("error",
                      "no parameter named \""
                    + n
                    + "\"");
            break;

        }
        send_message(current_value);
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
        timestamp = timestamp.gettime();
    }

    int priority(50);
    if(msg.has_parameter("priority"))
    {
        priority = msg.get_integer_parameter("priority");
        if(priority < 0 || priority > 99)
        {
            reply.set_command("FLUID_SETTINGS_ERROR");
            reply.add_parameter("error", "parameter \"priority\" ("
                                + std::to_string(priority)
                                + ") is out of range (0 .. 99)");
            reply.add_parameter("error_command", "FLUID_SETTINGS_PUT");
            send_message(reply);
            return;
        }
    }

    fluid_settings::set_result_t const result(f_server->set_value(name, value, priority, timestamp));
    switch(result)
    {
    case fluid_settings::set_result_t::SET_RESULT_NEW: // that value was not yet set
        reply.set_command("FLUID_SETTINGS_UPDATED");
        reply.add_parameter("name", name);
        reply.add_parameter("reason", "new");
        break;

    case fluid_settings::set_result_t::SET_RESULT_NEWER: // timestamp changed, value is the same
        reply.set_command("FLUID_SETTINGS_UPDATED");
        reply.add_parameter("name", name);
        reply.add_parameter("reason", "newer");
        break;

    case fluid_settings::set_result_t::SET_RESULT_NEW_PRIORITY: // new value at that priority
        reply.set_command("FLUID_SETTINGS_UPDATED");
        reply.add_parameter("name", name);
        reply.add_parameter("reason", "new priority");
        break;

    case fluid_settings::set_result_t::SET_RESULT_CHANGED: // value existed and was replaced
        reply.set_command("FLUID_SETTINGS_UPDATED");
        reply.add_parameter("name", name);
        reply.add_parameter("reason", "changed");
        break;

    case fluid_settings::set_result_t::SET_RESULT_UNCHANGED: // value exists and no change was required (timestamp is older than current value timestamp)
        reply.set_command("FLUID_SETTINGS_UPDATED");
        reply.add_parameter("name", name);
        reply.add_parameter("reason", "unchanged");
        break;

    case fluid_settings::set_result_t::SET_RESULT_ERROR: // value was refused by advgetopt
        reply.set_command("FLUID_SETTINGS_ERROR");
        reply.add_parameter("error", "put named setting \"" + name + "\" to value \"" + value + "\" failed");
        reply.add_parameter("error_command", "FLUID_SETTINGS_PUT");
        break;

    case fluid_settings::set_result_t::SET_RESULT_UNKNOWN: // no settings with that name found
        reply.set_command("FLUID_SETTINGS_ERROR");
        reply.add_parameter("error", "no parameter named \"" + name + "\"");
        reply.add_parameter("error_command", "FLUID_SETTINGS_PUT");
        break;

    }

    send_message(reply);
}



} // namespace fluid_settings_daemon
// vim: ts=4 sw=4 et
