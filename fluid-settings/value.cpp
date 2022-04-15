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
 * \brief This implements the management of one value.
 *
 * This file implements the "value" class.
 */

// self
//
#include	"value.h"

#include	"exception.h"


// last include
//
#include	<snapdev/poison.h>



namespace fluid_settings
{



void value::set_value(
      std::string const & v
    , priority_t priority
    , snapdev::timespec_ex const & timestamp)
{
    if(priority < MINIMUM_PRIORITY
    || priority > MAXIMUM_PRIORITY)
    {
        throw fluid_settings_parameter_error(
              "priority "
            + std::to_string(priority)
            + " is out of bounds ("
            + std::to_string(MINIMUM_PRIORITY)
            + " to "
            + std::to_string(MAXIMUM_PRIORITY)
            + ").");
    }
    timestamp_t now(snapdev::timespec_ex::gettime());
    timestamp_t max_diff(60 * 60, 0);   // 1h
    now -= max_diff;
    if(timestamp < now)
    {
        throw fluid_settings_parameter_error(
              "timestamp too low to be acceptable; a message should"
              " never take 1h or more to travel to the fluid-settings service ("
            + timestamp.to_string()
            + " < "
            + now.to_string()
            + ").");
    }

    f_value = v;
    f_priority = priority;
    f_timestamp = timestamp;
}


std::string const & value::get_value() const
{
    return f_value;
}


priority_t value::get_priority() const
{
    return f_priority;
}


snapdev::timespec_ex const & value::get_timestamp() const
{
    return f_timestamp;
}


bool value::operator < (value const & rhs) const
{
    return f_priority < rhs.f_priority;
}



} // namespace fluid_settings
// vim: ts=4 sw=4 et
