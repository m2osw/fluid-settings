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
 * \brief The declaration of the client class to access fluid-settings.
 *
 * This file is the declaration of the client class fluid_settings_connection
 * allowing your process to gather the current settings and listen for
 * changes over the time your process is running.
 *
 * This base class handles the connection to communicator daemon in order
 * to communicate with the fluid-settings daemon and any other service your
 * tool needs to communicate with.
 *
 * It also automatically registers with the fluid settings for all the
 * settings you are interested with. It can also be used to do a GET
 * (one time read of a value) and a PUT (update of a value) and even a
 * DELETE (reset the value back to its default or the undefined state).
 *
 * Your own class should derive from this class:
 *
 * \code
 *     class messenger
 *         : public fluid_settings::fluid_settings_connection
 *     {
 *     public:
 *         messenger(..., advgetopt::getopt & opt, ...)
 *             : fluid_settings_connection(opt, "service_name")
 *         {
 *         }
 *     };
 * \endcode
 *
 * The \p opt parameter passed to the constructor is going to be used to
 * gather parameters that the class makes use of to connect to the
 * communicator daemon and the fluid-settings class itself.
 */


// self
//
#include    "fluid-settings/value.h"


// communicatord
//
#include    <communicatord/communicatord.h>


// eventdispatcher
//
#include    <eventdispatcher/connection_with_send_message.h>
#include    <eventdispatcher/dispatcher.h>



namespace fluid_settings
{                                           // message includes:
                                            // +---- name
                                            // |
                                            // | +-- value
enum class fluid_settings_status_t          // v v
{                                           //
    FLUID_SETTINGS_STATUS_VALUE,            // x x   got the value
    FLUID_SETTINGS_STATUS_DEFAULT,          // x x   got the default value
    FLUID_SETTINGS_STATUS_NEW_VALUE,        // x x   got a new value (i.e. a SET just happened)
    FLUID_SETTINGS_STATUS_UNDEFINED,        // x -   GET/LISTEN failed to find a value
    FLUID_SETTINGS_STATUS_DELETED,          // x -   DELETE succeeded
    FLUID_SETTINGS_STATUS_UPDATED,          // x -   SET succeeded
    FLUID_SETTINGS_STATUS_TIMEOUT,          // - -   for an explicit command to "end" at some point
    FLUID_SETTINGS_STATUS_UNAVAILABLE,      // - -   the fluid-settings daemon is not available
    FLUID_SETTINGS_STATUS_REGISTERED,       // - -   LISTEN worked
    FLUID_SETTINGS_STATUS_READY,            // - x   you received all the current values--if value is not empty, it represents an error
};


class fluid_settings_connection
    : public communicatord::communicator
{
public:
                        fluid_settings_connection(
                              advgetopt::getopt & opts
                            , std::string const & service_name);
                        fluid_settings_connection(fluid_settings_connection const &) = delete;
    virtual             ~fluid_settings_connection() override;
    fluid_settings_connection &
                        operator = (fluid_settings_connection const &) = delete;

    // connection_with_send_message implementation
    //
    virtual void        ready(ed::message & msg) override;

    void                automatic_watch_initialization();
    void                add_fluid_settings_commands();
    void                process_fluid_settings_options();
    void                unregister_fluid_settings(bool quitting);

    void                get_settings_value(std::string const & name);
    void                get_settings_all_values(std::string const & name);
    void                get_settings_value_with_priority(std::string const & name, priority_t priority);
    void                get_settings_default_value(std::string const & name);
    void                add_watch(std::string const & name);
    std::string         qualify_name(std::string const & name);

    // connection_with_send_message implementation
    //
    virtual void        msg_service_unavailable(ed::message & msg) override;

    // new callbacks
    //
    virtual void        fluid_failed(ed::message & msg);
    virtual void        fluid_settings_changed(
                              fluid_settings_status_t status
                            , std::string const & name
                            , std::string const & value);
    virtual void        fluid_settings_options(advgetopt::string_list_t const & list);
    virtual void        service_status(std::string const & service, std::string const & status);

    // the following are internal message handlers and as such should be
    // considered private
    //
    void                msg_fluid_default_value(ed::message & msg);
    void                msg_fluid_deleted(ed::message & msg);
    void                msg_fluid_error(ed::message & msg);
    void                msg_fluid_options(ed::message & msg);
    void                msg_fluid_registered(ed::message & msg);
    void                msg_fluid_status(ed::message & msg);
    void                msg_fluid_updated(ed::message & msg);
    void                msg_fluid_value(ed::message & msg);
    void                msg_fluid_value_updated(ed::message & msg);
    void                msg_fluid_ready(ed::message & msg);
    void                msg_fluid_timeout();

private:
    void                listen(std::string const & watches);

    advgetopt::getopt & f_opts;
    bool                f_registered = false;
    std::set<std::string>
                        f_watches = std::set<std::string>();
};



} // namespace fluid_settings
// vim: ts=4 sw=4 et
