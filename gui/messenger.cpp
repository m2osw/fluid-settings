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
#include    "messenger.h"


// fluid-settings
//
#include    <fluid-settings/names.h>


// eventdispatcher
//
#include    <eventdispatcher/names.h>


// communicator
//
#include    <communicator/names.h>


// advgetopt
//
#include    <advgetopt/validator_double.h>
#include    <advgetopt/validator_integer.h>


// snapdev
//
#include    <snapdev/not_used.h>


// last include
//
#include    <snapdev/poison.h>



//namespace fluid_settings_daemon
//{



messenger::messenger(FluidWindow * w, advgetopt::getopt & opts)
    : fluid_settings_connection(opts, "fluid_settings_gui")
    , f_window(w)
{
    set_name("fluid_settings_gui_messenger");
}


messenger::~messenger()
{
}


void messenger::ready(ed::message & msg)
{
    snapdev::NOT_USED(msg);

    // now that the fluid settings are ready, enable the window fully
    //
    f_window->ready();
}


void messenger::stop(bool quitting)
{
    snapdev::NOT_USED(quitting);

    f_window->quit();
}


// TODO: look at ../prinbee/prinbee/network/prinbee_connection.cpp
//       for example on how to get the data here



//} // namespace fluid_settings_daemon
// vim: ts=4 sw=4 et
