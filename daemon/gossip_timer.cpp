// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
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
 * \brief The implementation of the gossip_timer.
 *
 * This file is the implementation of the gossip_timer class. It allows us
 * to save the settings after a few seconds, which gives time for the
 * client(s) to set multiple values in a row before a save happens.
 */

// self
//
#include    "gossip_timer.h"


//// advgetopt
////
//#include    <advgetopt/validator_double.h>
//#include    <advgetopt/validator_integer.h>
//
//
//// boost
////
//#include    <boost/preprocessor/stringize.hpp>


// last include
//
#include    <snapdev/poison.h>



namespace fluid_settings_daemon
{



gossip_timer::gossip_timer(server * s, std::int64_t timeout_us)
    : timer(timeout_us)
    , f_server(s)
{
    // by default, there is nothing to save
    //
    set_enable(false);
}


gossip_timer::~gossip_timer()
{
}


void gossip_timer::process_timeout()
{
    f_server->send_gossip();
}



} // namespace fluid_settings_daemon
// vim: ts=4 sw=4 et
