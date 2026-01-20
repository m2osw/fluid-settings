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
 * \brief Implementation of the listener object.
 *
 * The listener object is the one that accepts connections from the outside.
 */



// self
//
#include    "server.h"



namespace fluid_settings_daemon
{



class listener
    : public ed::tcp_server_connection
{
public:
                        listener(
                              server * s
                            , addr::addr const & addr
                            , int max_connections);
                        listener(listener const &) = delete;

    listener &          operator = (listener const &) = delete;

    // ed::tcp_server_connection
    //
    virtual void        process_accept() override;

private:
    server *            f_server = nullptr;
    ed::communicator::pointer_t
                        f_communicator = ed::communicator::pointer_t();
};



} // namespace fluid_settings_daemon
// vim: ts=4 sw=4 et
