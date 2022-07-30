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
 * communicator daemon. That way we have full communication with
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


// advgetopt
//
#include    "advgetopt/advgetopt.h"


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/tcp_client_permanent_message_connection.h>



namespace fluid_settings_cli
{


class client;
typedef std::shared_ptr<client>     client_pointer_t;


class cli
{
public:
    typedef std::shared_ptr<cli>    pointer_t;

                        cli(int argc, char * argv[]);

    int                 run();
    void                ready();
    void                setup_watches();
    void                fluid_settings_listen();
    void                deleted();
    void                list(advgetopt::string_list_t const & options);
    void                registered();
    void                updated();
    void                value(
                              std::string const & name
                            , std::string const & value
                            , bool is_default);
    void                value_updated(
                              std::string const & name
                            , std::string const & value);
    void                close();
    void                timeout();
    void                failed(ed::message & msg);

private:
    bool                print_value(std::string const & value);

    advgetopt::getopt   f_opts;
    ed::communicator::pointer_t
                        f_communicator = ed::communicator::pointer_t();
    addr::addr          f_address = addr::addr();
    client_pointer_t    f_client = client_pointer_t();
    ed::connection::pointer_t
                        f_timer = ed::connection::pointer_t();
    bool                f_success = false;
};


std::string const & get_our_service_name();


} // namespace fluid_settings_cli
// vim: ts=4 sw=4 et
