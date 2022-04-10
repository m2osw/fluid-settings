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
//
//
//// advgetopt
////
//#include    <advgetopt/exception.h>
//
//
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



namespace fluid_settings_cli
{



ed::dispatcher<client>::dispatcher_match::vector_t const g_dispatcher_messages =
{
    { // reply to DELETE
          "FLUID_SETTINGS_DELETED"
        , &client::msg_deleted
    },
    { // reply on failure
          "FLUID_SETTINGS_FAILED"
        , &client::msg_failed
    },
    { // reply to LIST
          "FLUID_SETTINGS_OPTIONS"
        , &client::msg_options
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
};


client::client(cli * parent, addr::addr const & address)
    : tcp_client_permanent_message_connection(address)
    , f_parent(parent)
    , f_dispatcher(std::make_shared<ed::dispatcher<client>>(this, g_dispatcher_messages))
{
    f_dispatcher->add_communicator_commands();
#ifdef _DEBUG
    f_dispatcher->set_trace();
#endif
}


client::~client()
{
}


void client::process_timeout()
{
    f_parent->timeout();
}


void client::msg_deleted(ed::message & msg)
{
    snapdev::NOT_USED(msg);

    f_parent->deleted();
}


void client::msg_failed(ed::message & msg)
{
    f_parent->failed(msg);
}


void client::msg_options(ed::message & msg)
{
    f_parent->list(msg);
}


void client::msg_updated(ed::message & msg)
{
    snapdev::NOT_USED(msg);

    f_parent->updated();
}


void client::msg_value(ed::message & msg)
{
    f_parent->value(msg);
}


void client::msg_value_updated(ed::message & msg)
{
    snapdev::NOT_USED(msg);

    f_parent->value_updated(msg);
}



} // fluid_settings namespace
// vim: ts=4 sw=4 et
