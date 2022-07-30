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



namespace fluid_settings_daemon
{



class server;


class replicator_out
    : public ed::tcp_client_permanent_message_connection
{
public:
    typedef std::shared_ptr<replicator_out> pointer_t;

    static constexpr int const              REPLICATOR_ERROR_LIMIT = 10;

                        replicator_out(server * s, addr::addr const & address);
                        replicator_out(replicator_out const &) = delete;
    virtual             ~replicator_out() override;
    replicator_out &    operator = (replicator_out const &) = delete;

    bool                count_errors();
    void                reset_errors();

    // tcp_client_permanent_message_connection implementation
    //
    virtual void        process_error() override;
    virtual void        process_hup() override;
    virtual void        process_invalid() override;
    virtual void        process_connected() override;

    void                msg_value_changed(ed::message & msg);

private:
    server *            f_server = nullptr;
    ed::communicator::pointer_t
                        f_communicator = ed::communicator::pointer_t();
    ed::dispatcher::pointer_t
                        f_dispatcher = ed::dispatcher::pointer_t();
    int                 f_errors = 0;
};



} // namespace fluid_settings_daemon
// vim: ts=4 sw=4 et
