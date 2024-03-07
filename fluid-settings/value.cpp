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
 * \brief This implements the management of one value.
 *
 * This file implements the "value" class.
 */

// self
//
#include	"value.h"

#include	"exception.h"


// snapdev
//
#include	<snapdev/timestamp.h>


// last include
//
#include	<snapdev/poison.h>



namespace fluid_settings
{


constexpr time_t const      g_oldest_fluid_setting_date = snapdev::unix_timestamp(
                                          2022      // year
                                        , 7         // month
                                        , 21        // day
                                        , 0         // hour
                                        , 0         // minute
                                        , 0);       // second

timestamp_t const           g_oldest_fluid_setting(g_oldest_fluid_setting_date, 0);



void value::set_value(
      std::string const & v
    , priority_t priority
    , timestamp_t const & timestamp)
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
    if(timestamp < g_oldest_fluid_setting)
    {
        throw fluid_settings_parameter_error(
              "value timestamp from before fluid-settings' birth ("
            + timestamp.to_string()
            + " < "
            + g_oldest_fluid_setting.to_string()
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
