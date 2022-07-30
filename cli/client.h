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


// fluid_settings
//
#include    <fluid-settings/fluid_settings_connection.h>


// eventdispatcher
//
#include    <eventdispatcher/connection_with_send_message.h>
#include    <eventdispatcher/dispatcher.h>



namespace fluid_settings_cli
{



class client
    : public fluid_settings::fluid_settings_connection
{
public:
                        client(cli * parent, advgetopt::getopt & opts);
                        client(client const &) = delete;
    virtual             ~client() override;
    client &            operator = (client const &) = delete;

    // connection_with_send_message implementation
    //
    virtual void        ready(ed::message & msg) override;

    // fluid_settings_connection implementation
    //
    virtual void        fluid_failed(ed::message & msg) override;
    virtual void        fluid_settings_changed(
                              fluid_settings::fluid_settings_status_t status
                            , std::string const & name
                            , std::string const & value) override;
    virtual void        fluid_settings_options(
                              advgetopt::string_list_t const & options) override;
    virtual void        service_status(
                              std::string const & service
                            , std::string const & status) override;

private:
    cli *               f_parent = nullptr;
    ed::dispatcher::pointer_t
                        f_dispatcher = ed::dispatcher::pointer_t();
};



} // namespace fluid_settings_cli
// vim: ts=4 sw=4 et
