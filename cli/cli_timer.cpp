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
 * \brief The implementation of the cli_timer.
 *
 * This file is the implementation of the cli_timer class. It allows us
 * to timeout in case we can't get a connection or a reply to our message.
 */

// self
//
#include    "cli_timer.h"


// last include
//
#include    <snapdev/poison.h>



namespace fluid_settings_cli
{



cli_timer::cli_timer(cli * c, std::int64_t timeout_us)
    : timer(timeout_us)
    , f_cli(c)
{
}


cli_timer::~cli_timer()
{
}


void cli_timer::process_timeout()
{
    f_cli->timeout();
}



} // namespace fluid_settings_cli
// vim: ts=4 sw=4 et
