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
#include    "replicator_out.h"


// fluid-settings
//
#include    <fluid-settings/names.h>


// last include
//
#include    <snapdev/poison.h>



namespace fluid_settings_daemon
{




replicator_out::replicator_out(
              server * s
            , addr::addr const & address)
    : tcp_client_permanent_message_connection(address)
    , f_server(s)
    , f_communicator(ed::communicator::instance())
    , f_dispatcher(std::make_shared<ed::dispatcher>(this))
{
    f_dispatcher->add_communicator_commands();
#ifdef _DEBUG
    f_dispatcher->set_trace();
#endif
    f_dispatcher->add_matches({
        DISPATCHER_MATCH(fluid_settings::g_name_fluid_settings_cmd_value_changed, &replicator_out::msg_value_changed),
    });
}


replicator_out::~replicator_out()
{
}


bool replicator_out::count_errors()
{
    ++f_errors;
    if(f_errors >= REPLICATOR_ERROR_LIMIT)
    {
        // this fluid-settings is not accessible, drop it for now
        // (maybe it was really removed)
        //
        f_communicator->remove_connection(shared_from_this());
        return false;
    }

    return true;
}


void replicator_out::reset_errors()
{
    f_errors = 0;
}


void replicator_out::process_error()
{
    if(count_errors())
    {
        tcp_client_permanent_message_connection::process_error();
    }
}


void replicator_out::process_hup()
{
    if(count_errors())
    {
        tcp_client_permanent_message_connection::process_hup();
    }
}


void replicator_out::process_invalid()
{
    if(count_errors())
    {
        tcp_client_permanent_message_connection::process_invalid();
    }
}


void replicator_out::process_connected()
{
    reset_errors();
}


void replicator_out::msg_value_changed(ed::message & msg)
{
    f_server->remote_value_changed(
          msg
        , std::dynamic_pointer_cast<replicator_out>(shared_from_this()));
}



} // namespace fluid_settings_daemon
// vim: ts=4 sw=4 et
