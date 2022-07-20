// Copyright (c) 2022  Made to Order Software Corp.  All Rights Reserved
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
#pragma once

/** \file
 * \brief The declaration of the CLI class.
 *
 * This file is the declaration of the CLI class.
 *
 * Note that the CLI is a complete client that connects to the
 * snapcommunicator. That way we have full communication with
 * the fluid-settings service.
 *
 * This also means that we depend on getting a reply from the
 * fluid-settings service. If no reply is received with one
 * second, then the CLI fails with an error.
 *
 * \note
 * The CLI class itself is not the connection so that way the
 * connection can be properly managed by the CLI object.
 */


// self
//
#include    "server.h"


// libaddr
//
#include    <libaddr/addr.h>


// eventdispatcher
//
#include    <eventdispatcher/dispatcher.h>


// communicatord
//
#include    <communicatord/communicatord.h>





namespace fluid_settings_daemon
{



class server;


class messenger
    //: public ed::tcp_client_permanent_message_connection
    : public communicatord::communicator
{
public:
    typedef std::shared_ptr<messenger>      pointer_t;

                        messenger(
                              server * s
                            , advgetopt::getopt & opts);
                        messenger(messenger const &) = delete;
    virtual             ~messenger() override;
    messenger &         operator = (messenger const &) = delete;

    // connection_with_send_message implementation
    //
    virtual void        ready(ed::message & msg) override;
    virtual void        restart(ed::message & msg) override;
    virtual void        stop(bool quitting) override;

    void                msg_connected(ed::message & msg);
    void                msg_delete(ed::message & msg);
    void                msg_forget(ed::message & msg);
    void                msg_get(ed::message & msg);
    void                msg_gossip(ed::message & msg);
    void                msg_list(ed::message & msg);
    void                msg_listen(ed::message & msg);
    void                msg_put(ed::message & msg);

private:
    void                connect_from_gossip(ed::message & msg, bool send_reply);

    server *            f_server = nullptr;
    ed::dispatcher<messenger>::pointer_t
                        f_dispatcher = ed::dispatcher<messenger>::pointer_t();
};



} // namespace fluid_settings_daemon
// vim: ts=4 sw=4 et
