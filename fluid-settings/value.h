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
 * The fluid settings is an array of named values. Each value has multiple
 * parameters:
 *
 * * The name, which is used as the index to find values in memory.
 * * The actual value, these are verified with the f_opts in the
 *   `settings` class.
 * * The priority, which defines which values a simple GET returns
 *   (i.e. the value with the highest priority is returned by a
 *   GET which does not specify a priority; this is the current value
 *   of this named setting).
 * * The timestamp when that value was last set.
 *
 * The timestamp is specific to a value at a given priority. We need it
 * to make sure that we keep the last value being set.
 */
#pragma

// snapdev
//
#include    <snapdev/timespec_ex.h>


// C++
//
#include    <map>
#include    <set>



namespace fluid_settings
{



typedef snapdev::timespec_ex    timestamp_t;
typedef int                     priority_t;

// the priority of -1 is a special value to get the value with the highest
// priority (which is the default when doing a GET)
//
constexpr priority_t const      HIGHEST_PRIORITY = -1;

// the priority of 0 is for a service's defaults
//
constexpr priority_t const      DEFAULTS_PRIORITY = 0;

// ... 1 to 49 are reserved for apps to install their own defaults

constexpr priority_t const      ADMINISTRATOR_PRIORITY = 50;

// ... 51 to 99 are reserved for apps to overwrite even administrator defaults

constexpr priority_t const      MINIMUM_PRIORITY = 0;
constexpr priority_t const      MAXIMUM_PRIORITY = 99;



// one value class holds a value, its priority and timestamp
//
class value
{
public:
    typedef std::set<value>                     set_t;
    typedef std::map<std::string, value::set_t> map_t;

    void                    set_value(
                                  std::string const & v
                                , priority_t priority
                                , timestamp_t const & timestamp);
    std::string const &     get_value() const;
    priority_t              get_priority() const;
    timestamp_t const &     get_timestamp() const;

    bool                    operator < (value const & rhs) const;

private:
    std::string             f_value = std::string();
    int                     f_priority = ADMINISTRATOR_PRIORITY;
    timestamp_t             f_timestamp = timestamp_t();
};





} // namespace fluid_settings
// vim: ts=4 sw=4 et
