// Copyright (c) 2012-2024  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/eventdispatcher
// contact@m2osw.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#pragma once

/** \file
 * \brief Exceptions.
 *
 * The following are all the exceptions used by the Event Dispatcher
 * library.
 */


// libexcept
//
#include    "libexcept/exception.h"



namespace fluid_settings
{



DECLARE_LOGIC_ERROR(fluid_settings_parameter_error);
DECLARE_LOGIC_ERROR(fluid_settings_implementation_error);

DECLARE_MAIN_EXCEPTION(fluid_settings_exception);

DECLARE_EXCEPTION(fluid_settings_exception, invalid_value);
DECLARE_EXCEPTION(fluid_settings_exception, overflow);



} // namespace ed
// vim: ts=4 sw=4 et
