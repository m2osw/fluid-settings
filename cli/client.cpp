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
#include    "client.h"


// last include
//
#include    <snapdev/poison.h>



namespace fluid_settings_cli
{



ed::dispatcher<client>::dispatcher_match::vector_t const g_dispatcher_messages =
{
    { // reply when the default value is being returned
          "FLUID_SETTINGS_DEFAULT_VALUE"
        , &client::msg_default_value
    },
    { // reply to DELETE
          "FLUID_SETTINGS_DELETED"
        , &client::msg_deleted
    },
    { // reply when an error was detected (i.e. wrong value name)
          "FLUID_SETTINGS_ERROR"
        , &client::msg_error
    },
    { // reply on failure
          "FLUID_SETTINGS_FAILED"
        , &client::msg_failed
    },
    { // reply to LIST
          "FLUID_SETTINGS_OPTIONS"
        , &client::msg_options
    },
    { // reply to LISTEN
          "FLUID_SETTINGS_REGISTERED"
        , &client::msg_registered
    },
    { // reply to PUT
          "FLUID_SETTINGS_UPDATED"
        , &client::msg_updated
    },
    { // reply to GET
          "FLUID_SETTINGS_VALUE"
        , &client::msg_value
    },
    { // reply to LISTEN
          "FLUID_SETTINGS_VALUE_UPDATED"
        , &client::msg_value_updated
    },
    { // reply to SERVICESTATUS and messages when the status changes
          "STATUS"
        , &client::msg_status
    },
};


client::client(cli * parent, addr::addr const & address)
    : tcp_client_permanent_message_connection(
              address
            , ed::mode_t::MODE_PLAIN
            , ed::DEFAULT_PAUSE_BEFORE_RECONNECTING
            , true
            , get_our_service_name())
    , f_parent(parent)
    , f_dispatcher(std::make_shared<ed::dispatcher<client>>(this, g_dispatcher_messages))
{
    f_dispatcher->add_communicator_commands();
#ifdef _DEBUG
    f_dispatcher->set_trace();
#endif
    set_dispatcher(f_dispatcher);
}


client::~client()
{
}


void client::process_connected()
{
    tcp_client_permanent_message_connection::process_connected();
    register_service();
}


void client::msg_service_unavailable(ed::message & msg)
{
    snapdev::NOT_USED(msg);

    SNAP_LOG_ERROR
        << "fluid_settings service is not currently available."
        << SNAP_LOG_SEND;

    f_parent->close();
}


void client::ready(ed::message & msg)
{
    snapdev::NOT_USED(msg);

    f_parent->ready();
}


void client::msg_default_value(ed::message & msg)
{
    f_parent->value(msg, true);
}


void client::msg_deleted(ed::message & msg)
{
    snapdev::NOT_USED(msg);

    f_parent->deleted();
}


void client::msg_error(ed::message & msg)
{
    SNAP_LOG_ERROR
        << "an error occurred: "
        << msg.to_string()
        << SNAP_LOG_SEND;

    f_parent->close();
}


void client::msg_failed(ed::message & msg)
{
    f_parent->failed(msg);
}


void client::msg_options(ed::message & msg)
{
    f_parent->list(msg);
}


void client::msg_registered(ed::message & msg)
{
    snapdev::NOT_USED(msg);

    f_parent->registered();
}


void client::msg_updated(ed::message & msg)
{
    snapdev::NOT_USED(msg);

    f_parent->updated();
}


void client::msg_value(ed::message & msg)
{
    f_parent->value(msg, false);
}


void client::msg_value_updated(ed::message & msg)
{
    snapdev::NOT_USED(msg);

    f_parent->value_updated(msg);
}


void client::msg_status(ed::message & msg)
{
    if(msg.has_parameter("status"))
    {
        if(msg.get_parameter("status") == "up")
        {
            std::cout << "fluid_settings service is up.\n";
            f_parent->fluid_settings_listen();
        }
        else
        {
            std::cout << "fluid_settings service is down.\n";
        }
    }
}



} // fluid_settings namespace
// vim: ts=4 sw=4 et
