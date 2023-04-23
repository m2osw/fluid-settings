// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/snapcommunicator
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
 * \brief Implementation of the Snap! Communicator service connection.
 *
 * A service is a local daemon offering a service to our system. Such
 * as service connections to the snapcommunicator daemon via the local
 * TCP connection and uses that connection to register itself and
 * then send messages to other services wherever they are in the network.
 */

// self
//
#include    "replicator_in.h"


// fluid-settings
//
#include    <fluid-settings/names.h>


// last include
//
#include    <snapdev/poison.h>







namespace fluid_settings_daemon
{




/** \class service_connection
 * \brief Listen for messages.
 *
 * The snapcommunicator TCP connection simply listen for process_message()
 * callbacks and processes those messages by calling the process_message()
 * of the connections class.
 *
 * It also listens for disconnections so it can send a new STATUS command
 * whenever the connection goes down.
 */


/** \brief Create a service connection and assigns \p socket to it.
 *
 * The constructor of the service connection expects a socket that
 * was just accept()'ed.
 *
 * The snapcommunicator daemon listens on to two different ports
 * and two different addresses on those ports:
 *
 * \li TCP 127.0.0.1:4040 -- this address is expected to be used by all the
 * local services
 *
 * \li TCP 0.0.0.0:4040 -- this address is expected to be used by remote
 * snapcommunicators; it is often changed to a private network IP
 * address such as 192.168.0.1 to increase safety. However, if your
 * cluster spans multiple data centers, it will not be possible to
 * use a private network IP address.
 *
 * \li UDP 127.0.0.1:4041 -- this special port is used to accept UDP
 * signals sent to the snapcommunicator; UDP signals are most often
 * used to very quickly send signals without having to have a full
 * TCP connection to a daemon
 *
 * The connections happen on 127.0.0.1 are fully trusted. Connections
 * happening on 0.0.0.0 are generally viewed as tainted.
 *
 * \param[in] s  The fluid settings server (i.e. parent)
 * \param[in] client  The socket that was just created by the accept()
 *                    command.
 */
replicator_in::replicator_in(
            server * s
          , ed::tcp_bio_client::pointer_t client)
    : tcp_server_client_message_connection(client)
    , f_server(s)
    , f_communicator(ed::communicator::instance())
    , f_dispatcher(std::make_shared<ed::dispatcher>(this))
{
    f_dispatcher->add_matches({
        DISPATCHER_MATCH(fluid_settings::g_name_fluid_settings_cmd_value_changed, &replicator_in::msg_value_changed),
    });
}


/** \brief Connection lost.
 *
 * When a connection goes down it gets deleted. This is when we can
 * send a new STATUS event to all the other STATUS hungry connections.
 */
replicator_in::~replicator_in()
{
}


void replicator_in::msg_value_changed(ed::message & msg)
{
    f_server->remote_value_changed(
          msg
        , std::dynamic_pointer_cast<replicator_in>(shared_from_this()));
}


//bool replicator_in::send_message(ed::message & msg, bool cache)
//{
//    return tcp_server_client_message_connection::send_message(msg, cache);
//}
//
//
//
///** \brief We are losing the connection, send a STATUS message.
// *
// * This function is called in all cases where the connection is
// * lost so we can send a STATUS message with information saying
// * that the connection is gone.
// */
//void service_connection::send_status()
//{
//    // mark connection as down before we call the send_status()
//    //
//    set_connection_type(connection_type_t::CONNECTION_TYPE_DOWN);
//
//    f_server->send_status(shared_from_this());
//}
//
//
///** \brief Remove ourselves when we receive a timeout.
// *
// * Whenever we receive a shutdown, we have to remove everything but
// * we still want to send some messages and to do so we need to use
// * the timeout, which happens after we finalize all read and write
// * callbacks.
// */
//void service_connection::process_timeout()
//{
//    remove_from_communicator();
//
//    send_status();
//}
//
//
//void service_connection::process_error()
//{
//    tcp_server_client_message_connection::process_error();
//
//    send_status();
//}
//
//
///** \brief Process a hang up.
// *
// * It is important for some processes to know when a remote connection
// * is lost (i.e. for dynamic QUORUM calculations in snaplock, for
// * example.) So we handle the process_hup() event and send a
// * HANGUP if this connection is a remote connection.
// */
//void service_connection::process_hup()
//{
//    tcp_server_client_message_connection::process_hup();
//
//    if(is_remote()
//    && !get_server_name().empty())
//    {
//        // TODO: this is nice, but we would probably need such in the
//        //       process_invalid(), process_error(), process_timeout()?
//        //
//        ed::message hangup;
//        hangup.set_command("HANGUP");
//        hangup.set_service(".");
//        hangup.add_parameter("server_name", get_server_name());
//        f_server->broadcast_message(hangup);
//
//        f_server->cluster_status(shared_from_this());
//    }
//
//    send_status();
//}
//
//
//void service_connection::process_invalid()
//{
//    tcp_server_client_message_connection::process_invalid();
//
//    send_status();
//}
//
//
///** \brief Tell that the connection was given a real name.
// *
// * Whenever we receive an event through this connection,
// * we want to mark the message as received from the service.
// *
// * However, by default the name of the service is on purpose
// * set to an "invalid value" (i.e. a name with a space.) That
// * value is not expected to be used when forwarding the message
// * to another service.
// *
// * Once a system properly registers with the REGISTER message,
// * we receive a valid name then. That name is saved in the
// * connection and the connection is marked as having a valid
// * name.
// *
// * This very function must be called once the proper name was
// * set in this connection.
// */
//void service_connection::properly_named()
//{
//    f_named = true;
//}
//
//
///** \brief Return the type of address this connection has.
// *
// * This function determines the type of address of the connection.
// *
// * \return A reference to the remote address of this connection.
// */
//addr::addr const & service_connection::get_address() const
//{
//    return f_address;
//}



} // namespace fluid_settings_daemon
// vim: ts=4 sw=4 et
