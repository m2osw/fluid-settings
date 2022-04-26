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
#include    "cli.h"


// libaddr
//
#include    <libaddr/addr.h>


// eventdispatcher
//
#include    <eventdispatcher/timer.h>



namespace fluid_settings_cli
{



class cli_timer
    : public ed::timer
{
public:
    typedef std::shared_ptr<cli_timer>      pointer_t;

                        cli_timer(cli * c, std::int64_t timeout_us);
                        cli_timer(cli_timer const &) = delete;
    virtual             ~cli_timer() override;
    cli_timer &         operator = (cli_timer const &) = delete;

    // timer implementation
    virtual void        process_timeout() override;

private:
    cli *               f_cli = nullptr;
};



} // namespace fluid_settings_cli
// vim: ts=4 sw=4 et
