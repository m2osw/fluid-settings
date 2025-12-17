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
 * \brief Definitions of the server.
 *
 * The server is created and starts the messenger and runs the communicator
 * run loop.
 */

// fluid-settings
//
#include    <fluid-settings/settings.h>


// advgetopt
//
#include    <advgetopt/advgetopt.h>


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/tcp_client_permanent_message_connection.h>
#include    <eventdispatcher/tcp_server_connection.h>



namespace fluid_settings_daemon
{


class messenger;


class server
{
public:
    typedef std::shared_ptr<server> pointer_t;

                            server(int argc, char * argv[]);

    int                     run();
    void                    restart();
    void                    stop(bool quitting);

    bool                    listen(
                                  std::string const & server_name
                                , std::string const & service_name
                                , std::string const & names);
    bool                    forget(
                                  std::string const & server_name
                                , std::string const & service_name
                                , std::string const & names);
    std::string             list_of_options();
    fluid_settings::get_result_t
                            get_value(
                                  std::string const & name
                                , std::string & value
                                , fluid_settings::priority_t priority
                                , bool all);
    fluid_settings::get_result_t
                            get_default_value(
                                  std::string const & name
                                , std::string & value);
    fluid_settings::set_result_t
                            set_value(
                                  std::string const & name
                                , std::string const & value
                                , fluid_settings::priority_t priority
                                , snapdev::timespec_ex const & timestamp);
    bool                    reset_setting(
                                  std::string const & name
                                , int priority);
    void                    value_changed(std::string const & name);
    void                    save_settings();
    addr::addr const &      get_listener_address() const;
    void                    send_gossip();
    void                    connect_to_other_fluid_settings(
                                  addr::addr const & their_ip);
    void                    remote_value_changed(
                                  ed::message const & msg
                                , ed::connection_with_send_message::pointer_t const & c);
    void                    add_replicator(ed::connection_with_send_message::weak_t connection);

private:
    bool                    prepare_settings();
    bool                    prepare_listener();
    bool                    prepare_save_timer();
    bool                    prepare_gossip_timer();

    advgetopt::getopt       f_opts;
    ed::communicator::pointer_t
                            f_communicator = ed::communicator::pointer_t();
    std::shared_ptr<messenger>
                            f_messenger = std::shared_ptr<messenger>();
    addr::addr              f_address = addr::addr();
    addr::addr              f_listener_address = addr::addr();
    ed::tcp_server_connection::pointer_t
                            f_listener = ed::tcp_server_connection::pointer_t();
    std::int64_t            f_save_timeout = 5'000'000;
    ed::timer::pointer_t    f_save_timer = ed::timer::pointer_t();
    fluid_settings::settings
                            f_settings = fluid_settings::settings();
    bool                    f_remote_change = false;
    ed::connection::pointer_t
                            f_gossip_timer = ed::connection::pointer_t();
    int                     f_exit_code = 0;
    ed::connection_with_send_message::list_weak_t
                            f_replicators = ed::connection_with_send_message::list_weak_t();

    struct server_service
    {
        typedef std::set<server_service>    set_t;

        std::string             f_server = std::string();
        std::string             f_service = std::string();

        bool operator < (server_service const & rhs) const
        {
            if(f_server < rhs.f_server)
            {
                return true;
            }
            if(f_server > rhs.f_server)
            {
                return false;
            }

            return f_service < rhs.f_service;
        }
    };
    typedef std::map<std::string, server_service::set_t>    listener_t;

    listener_t              f_listeners = listener_t();
};


} // namespace fluid_settings_daemon
// vim: ts=4 sw=4 et
