// Copyright (c) 2022-2024  Made to Order Software Corp.  All Rights Reserved
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
#include    <eventdispatcher/timer.h>



namespace fluid_settings_daemon
{



class server;


class save_timer
    : public ed::timer
{
public:
    typedef std::shared_ptr<save_timer>      pointer_t;

                        save_timer(server * s, std::int64_t timeout_us);
                        save_timer(save_timer const &) = delete;
    virtual             ~save_timer() override;
    save_timer &        operator = (save_timer const &) = delete;

    virtual void        process_timeout() override;

private:
    server *            f_server = nullptr;
};



} // namespace fluid_settings_daemon
// vim: ts=4 sw=4 et
