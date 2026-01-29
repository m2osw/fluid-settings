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
 * \brief Implementation of the listener.
 *
 * The listener connection is the one listening for connections from
 * local and remote services.
 */

// self
//
#include    "listener.h"

#include    "replicator_in.h"


// snaplogger
//
#include    <snaplogger/message.h>


// libaddr
//
//#include    <libaddr/addr.h>
#include    <libaddr/addr_parser.h>


// included last
//
#include    <snapdev/poison.h>



namespace fluid_settings_daemon
{



/** \class listener
 * \brief Handle new connections from clients.
 *
 * This class is an implementation of the snap server connection so we can
 * handle new connections from various clients.
 */




/** \brief The listener initialization.
 *
 * The listener creates a new TCP server to listen for incoming
 * TCP connection.
 *
 * \warning
 * At this time the \p max_connections parameter is ignored.
 *
 * \param[in] s  The server managing this listener.
 * \param[in] address  The address:port to listen on.
 * \param[in] max_connections  The maximum number of connections to keep.
 */
listener::listener(
          server * s
        , addr::addr const & address
        , int max_connections)
    : tcp_server_connection(
              address
            , std::string()
            , std::string()
            , ed::mode_t::MODE_PLAIN
            , max_connections
            , true)
    , f_server(s)
    , f_communicator(ed::communicator::instance())
{
    set_name("listener");
}


void listener::process_accept()
{
    // a new client just connected, create a new replicator_in
    // object and add it to the snap_communicator object.
    //
    ed::tcp_bio_client::pointer_t const new_client(accept());
    if(new_client == nullptr)
    {
        // an error occurred, report in the logs
        //
        int const e(errno);
        SNAP_LOG_ERROR
            << "somehow accept() failed with errno: "
            << e
            << " -- "
            << strerror(e)
            << SNAP_LOG_SEND;
        return;
    }

    replicator_in::pointer_t service(std::make_shared<replicator_in>(
                  f_server
                , new_client));
    if(f_communicator->add_connection(service))
    {
        f_server->add_replicator(service);
    }
    else
    {
        SNAP_LOG_RECOVERABLE_ERROR
            << "new replicator_in connection could not be added to the ed::communicator."
            << SNAP_LOG_SEND;
    }
}




} // namespace fluid_settings_daemon
// vim: ts=4 sw=4 et
