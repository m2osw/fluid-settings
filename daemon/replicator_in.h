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
#pragma once

/** \file
 * \brief The declaration of the service class.
 *
 * The service class is the one used whenever a fluid-settings daemon
 * connects to this fluid-settings daemon.
 */

// self
//
#include    "server.h"


// eventdispatcher
//
#include    <eventdispatcher/tcp_server_client_message_connection.h>
#include    <eventdispatcher/dispatcher.h>



namespace fluid_settings_daemon
{


class replicator_in
    : public ed::tcp_server_client_message_connection
{
public:
    typedef std::shared_ptr<replicator_in>    pointer_t;

                        replicator_in(
                                  server * s
                                , ed::tcp_bio_client::pointer_t client);
                        replicator_in(replicator_in const &) = delete;
    virtual             ~replicator_in() override;

    replicator_in &     operator = (replicator_in const &) = delete;

    void                msg_value_changed(ed::message & msg);

private:
    server *            f_server = nullptr;
    ed::communicator::pointer_t
                        f_communicator = ed::communicator::pointer_t();
    ed::dispatcher::pointer_t
                        f_dispatcher = ed::dispatcher::pointer_t();
};



} // namespace fluid_settings_daemon
// vim: ts=4 sw=4 et
