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

/** \file
 * \brief The implementation of the CLI.
 *
 * This file is the implementation of the CLI (Console Interface) of the
 * fluid-settings.
 *
 * This gives us access to the fluid-settings via the console. The main
 * functions are used to set new values and retrieve existing values.
 *
 * See the README.md for a list of supported options.
 */

// self
//
#include    "client.h"


// last include
//
#include    <snapdev/poison.h>



namespace fluid_settings_cli
{



client::client(cli * parent, advgetopt::getopt & opts)
    : fluid_settings_connection(opts, get_our_service_name())
    , f_parent(parent)
    , f_dispatcher(std::make_shared<ed::dispatcher>(this))
{
#ifdef _DEBUG
    f_dispatcher->set_trace();
    f_dispatcher->set_show_matches();
#endif
    set_dispatcher(f_dispatcher);

    add_fluid_settings_commands();

    // add the communicator commands last (it includes the "always match")
    f_dispatcher->add_communicator_commands();
}


client::~client()
{
}


void client::fluid_settings_changed(
      fluid_settings::fluid_settings_status_t status
    , std::string const & name
    , std::string const & value)
{
    switch(status)
    {
    case fluid_settings::fluid_settings_status_t::FLUID_SETTINGS_STATUS_NEW_VALUE:
        // after a SET (for the --watch capability)
        f_parent->value_updated(name, value);
        break;

    case fluid_settings::fluid_settings_status_t::FLUID_SETTINGS_STATUS_VALUE:
        f_parent->value(name, value, false);
        break;

    case fluid_settings::fluid_settings_status_t::FLUID_SETTINGS_STATUS_DEFAULT:
        f_parent->value(name, value, true);
        break;

    case fluid_settings::fluid_settings_status_t::FLUID_SETTINGS_STATUS_UNDEFINED:
        break;

    case fluid_settings::fluid_settings_status_t::FLUID_SETTINGS_STATUS_DELETED:
        f_parent->deleted();
        break;

    case fluid_settings::fluid_settings_status_t::FLUID_SETTINGS_STATUS_UPDATED:
        f_parent->updated();
        break;

    case fluid_settings::fluid_settings_status_t::FLUID_SETTINGS_STATUS_REGISTERED:
        f_parent->registered();
        break;

    case fluid_settings::fluid_settings_status_t::FLUID_SETTINGS_STATUS_READY:
        f_parent->fluid_ready();
        break;

    case fluid_settings::fluid_settings_status_t::FLUID_SETTINGS_STATUS_TIMEOUT:
        f_parent->timeout();
        break;

    case fluid_settings::fluid_settings_status_t::FLUID_SETTINGS_STATUS_UNAVAILABLE:
        f_parent->close();
        break;

    }
}


void client::fluid_settings_options(advgetopt::string_list_t const & options)
{
    f_parent->list(options);
}


void client::ready(ed::message & msg)
{
    snapdev::NOT_USED(msg);

    f_parent->ready();

    fluid_settings_connection::ready(msg);
}


void client::fluid_failed(ed::message & msg)
{
    // call base class function
    //
    fluid_settings::fluid_settings_connection::fluid_failed(msg);

    f_parent->failed(msg);
}


void client::service_status(
      std::string const & service
    , std::string const & status)
{
    if(service != "fluid_settings")
    {
        return;
    }

    if(status == "up")
    {
        std::cout << "fluid_settings service is up.\n";
    }
    else
    {
        std::cout << "fluid_settings service is down.\n";

        f_parent->service_down();
    }
}



} // fluid_settings namespace
// vim: ts=4 sw=4 et
